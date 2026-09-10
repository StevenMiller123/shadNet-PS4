// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <string_view>
#include <absl/container/flat_hash_map.h>
#include <magic_enum/magic_enum.hpp>
#include <orbis/Http.h>
#include <orbis/UserService.h>
#include <orbis/libkernel.h>
#include "common/elf_info.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler.h"
#include "core/libraries/np/np_web_api/np_web_api_internal.h"
#include "shadnet/config.h"

namespace Libraries::Np::NpWebApi {

static std::recursive_mutex g_global_mutex;
static absl::flat_hash_map<s32, OrbisNpWebApiContext*> g_contexts;
static s32 g_library_context_count = 0;

// Last WebApi error code parsed from an error response body
static thread_local s32 g_last_webapi_error = ORBIS_OK;

static s32 captureWebApiError(const char* body, u64 len);
static s32 g_user_context_count = 0;
static s32 g_handle_count = 0;
static s32 g_push_event_filter_count = 0;
static s32 g_service_push_event_filter_count = 0;
static s32 g_extended_push_event_filter_count = 0;
static s32 g_registered_callback_count = 0;
static s64 g_request_count = 0;
static u64 g_last_timeout_check = 0;
static s32 g_sdk_ver = 0;

s32 initializeLibrary() {
    return sceKernelGetCompiledSdkVersion(&g_sdk_ver);
}

s32 getCompiledSdkVersion() {
    return g_sdk_ver;
}

s32 createLibraryContext(s32 libHttpCtxId, u64 poolSize, const char* name, s32 type) {
    std::scoped_lock lk{g_global_mutex};

    g_library_context_count++;
    if (g_library_context_count >= 0x8000) {
        g_library_context_count = 1;
    }
    s32 ctx_id = g_library_context_count;
    while (g_contexts.find(ctx_id) != g_contexts.end()) {
        ctx_id--;
    }
    if (ctx_id <= 0) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_MAX;
    }

    // Create new context
    g_contexts[ctx_id] = new OrbisNpWebApiContext{};
    auto& new_context = g_contexts.at(ctx_id);
    new_context->libCtxId = ctx_id;
    new_context->libHttpCtxId = libHttpCtxId;
    new_context->type = type;
    new_context->userCount = 0;
    new_context->terminated = false;

    // Manually init mutex to get around issues
    pthread_mutex_t* lock = new_context->contextLock.native_handle();
    LOG_INFO(Lib_NpWebApi, "manually initializing lock {:#x}", (u64)lock);
    pthread_mutexattr_t mtx_attr{};
    pthread_mutexattr_init(&mtx_attr);
    pthread_mutexattr_settype(&mtx_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(lock, &mtx_attr);
    pthread_mutexattr_destroy(&mtx_attr);

    if (name != nullptr) {
        new_context->name = std::string(name);
    }

    return ctx_id;
}

OrbisNpWebApiContext* findAndValidateContext(s32 libCtxId, s32 flag) {
    std::scoped_lock lk{g_global_mutex};
    if (libCtxId < 1 || libCtxId >= 0x8000) {
        return nullptr;
    }
    if (g_contexts.find(libCtxId) == g_contexts.end()) {
        return nullptr;
    }
    auto& context = g_contexts[libCtxId];
    std::scoped_lock lk2{context->contextLock};
    if (flag == 0 && context->terminated) {
        return nullptr;
    }
    context->userCount++;
    return context;
}

void releaseContext(OrbisNpWebApiContext* context) {
    std::scoped_lock lk{context->contextLock};
    context->userCount--;
}

bool isContextTerminated(OrbisNpWebApiContext* context) {
    std::scoped_lock lk{context->contextLock};
    return context->terminated;
}

bool isContextBusy(OrbisNpWebApiContext* context) {
    std::scoped_lock lk{context->contextLock};
    return context->userCount > 1;
}

bool areContextHandlesBusy(OrbisNpWebApiContext* context) {
    std::scoped_lock lk{context->contextLock};
    for (auto& handle : context->handles) {
        if (handle.second->userCount > 0) {
            return true;
        }
    }
    return false;
}

void lockContext(OrbisNpWebApiContext* context) {
    context->contextLock.lock();
}

void unlockContext(OrbisNpWebApiContext* context) {
    context->contextLock.unlock();
}

void markContextAsTerminated(OrbisNpWebApiContext* context) {
    std::scoped_lock lk{context->contextLock};
    context->terminated = true;
}

void checkContextTimeout(OrbisNpWebApiContext* context) {
    u64 time = sceKernelGetProcessTime();
    std::scoped_lock lk{context->contextLock};

    for (auto& user_context : context->userContexts) {
        checkUserContextTimeout(user_context.second);
    }

    for (auto& value : context->timerHandles) {
        auto& timer_handle = value.second;
        if (!timer_handle->timedOut && timer_handle->handleTimeout != 0 &&
            timer_handle->handleEndTime < time) {
            timer_handle->timedOut = true;
            abortHandle(context->libCtxId, timer_handle->handleId);
        }
    }
}

void checkTimeout() {
    u64 time = sceKernelGetProcessTime();
    if (time < g_last_timeout_check + 1000) {
        return;
    }
    g_last_timeout_check = time;
    std::scoped_lock lk{g_global_mutex};

    for (auto& context : g_contexts) {
        checkContextTimeout(context.second);
    }
}

s32 deleteContext(s32 libCtxId) {
    std::scoped_lock lk{g_global_mutex};
    if (g_contexts.find(libCtxId) == g_contexts.end()) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    auto& context = g_contexts[libCtxId];
    context->handles.clear();
    context->timerHandles.clear();
    context->pushEventFilters.clear();
    context->servicePushEventFilters.clear();
    context->extendedPushEventFilters.clear();

    g_contexts.erase(libCtxId);
    return ORBIS_OK;
}

s32 terminateContext(s32 libCtxId) {
    OrbisNpWebApiContext* ctx = findAndValidateContext(libCtxId);
    if (ctx == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    if (g_sdk_ver < Common::ElfInfo::FW_400 && isContextBusy(ctx)) {
        releaseContext(ctx);
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_BUSY;
    }

    std::vector<s32> user_context_ids;
    for (auto& user_context : ctx->userContexts) {
        user_context_ids.emplace_back(user_context.first);
    }
    for (s32 user_context_id : user_context_ids) {
        s32 result = deleteUserContext(user_context_id);
        if (result != ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND ||
            g_sdk_ver < Common::ElfInfo::FW_400) {
            return result;
        }
    }

    lockContext(ctx);
    if (g_sdk_ver >= Common::ElfInfo::FW_400) {
        for (auto& handle : ctx->handles) {
            abortHandle(libCtxId, handle.first);
        }
        if (isContextTerminated(ctx)) {
            unlockContext(ctx);
            releaseContext(ctx);
            return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
        }
        markContextAsTerminated(ctx);
        while (isContextBusy(ctx) || areContextHandlesBusy(ctx)) {
            unlockContext(ctx);
            sceKernelUsleep(50000);
            lockContext(ctx);
        }
    }

    unlockContext(ctx);
    releaseContext(ctx);
    return deleteContext(libCtxId);
}

OrbisNpWebApiUserContext* findUserContextByUserId(OrbisNpWebApiContext* context, s32 userId) {
    if (userId == ORBIS_USER_SERVICE_USER_ID_INVALID) {
        return nullptr;
    }

    std::scoped_lock lk{context->contextLock};
    for (auto& user_context : context->userContexts) {
        if (user_context.second->userId == userId) {
            user_context.second->userCount++;
            return user_context.second;
        }
    }
    return nullptr;
}

OrbisNpWebApiUserContext* findUserContext(OrbisNpWebApiContext* context, s32 titleUserCtxId) {
    std::scoped_lock lk{context->contextLock};
    if (context->userContexts.find(titleUserCtxId) == context->userContexts.end()) {
        return nullptr;
    }
    OrbisNpWebApiUserContext* user_context = context->userContexts[titleUserCtxId];
    if (user_context->deleted) {
        return nullptr;
    }
    user_context->userCount++;
    return user_context;
}

s32 createUserContextWithOnlineId(s32 libCtxId, OrbisNpOnlineId* onlineId) {
    LOG_WARNING(Lib_NpWebApi, "called libCtxId = {}", libCtxId);

    s32 user_id = 0;
    sceUserServiceGetInitialUser(&user_id);
    return createUserContext(libCtxId, user_id);
}

s32 createUserContext(s32 libCtxId, s32 userId) {
    LOG_INFO(Lib_NpWebApi, "libCtxId = {}, userId = {}", libCtxId, userId);
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContextByUserId(context, userId);
    if (user_context != nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_ALREADY_EXIST;
    }

    std::scoped_lock lk{context->contextLock};

    // Create new user context
    g_user_context_count++;
    if (g_user_context_count >= 0x10000) {
        g_user_context_count = 1;
    }
    s32 user_ctx_id = (libCtxId << 0x10) | g_user_context_count;
    while (context->userContexts.find(user_ctx_id) != context->userContexts.end()) {
        user_ctx_id--;
    }
    if (user_ctx_id <= (libCtxId << 0x10)) {
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_MAX;
    }

    context->userContexts[user_ctx_id] = new OrbisNpWebApiUserContext{};
    user_context = context->userContexts.at(user_ctx_id);
    user_context->userCount = 0;
    user_context->parentContext = context;
    user_context->userId = userId;
    user_context->userCtxId = user_ctx_id;
    user_context->deleted = false;

    // TODO: Internal structs related to libSceHttp use are initialized here.
    releaseContext(context);
    return user_ctx_id;
}

s32 registerNotificationCallback(s32 titleUserCtxId, OrbisNpWebApiNotificationCallback cbFunc,
                                 void* pUserArg) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    lockContext(context);
    user_context->notificationCallbackFunction = cbFunc;
    user_context->pNotificationCallbackUserArgs = pUserArg;
    unlockContext(context);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 unregisterNotificationCallback(s32 titleUserCtxId) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    lockContext(context);
    user_context->notificationCallbackFunction = nullptr;
    user_context->pNotificationCallbackUserArgs = nullptr;
    unlockContext(context);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

bool isUserContextBusy(OrbisNpWebApiUserContext* userContext) {
    std::scoped_lock lk{userContext->parentContext->contextLock};
    return userContext->userCount > 1;
}

bool areUserContextRequestsBusy(OrbisNpWebApiUserContext* userContext) {
    std::scoped_lock lk{userContext->parentContext->contextLock};
    for (auto& request : userContext->requests) {
        request.second->userCount++;
        bool req_busy = isRequestBusy(request.second);
        request.second->userCount--;
        if (req_busy) {
            return true;
        }
    }
    return false;
}

void releaseUserContext(OrbisNpWebApiUserContext* userContext) {
    std::scoped_lock lk{userContext->parentContext->contextLock};
    userContext->userCount--;
}

void checkUserContextTimeout(OrbisNpWebApiUserContext* userContext) {
    std::scoped_lock lk{userContext->parentContext->contextLock};
    for (auto& request : userContext->requests) {
        checkRequestTimeout(request.second);
    }
}

s32 deleteUserContext(s32 titleUserCtxId) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    lockContext(context);
    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        unlockContext(context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (g_sdk_ver < Common::ElfInfo::FW_400) {
        if (isUserContextBusy(user_context)) {
            releaseUserContext(user_context);
            unlockContext(context);
            releaseContext(context);
            return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_BUSY;
        }

        if (areUserContextRequestsBusy(user_context)) {
            releaseUserContext(user_context);
            unlockContext(context);
            releaseContext(context);
            return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_BUSY;
        }
    } else {
        for (auto& request : user_context->requests) {
            abortRequestInternal(context, user_context, request.second);
        }

        if (user_context->deleted) {
            releaseUserContext(user_context);
            unlockContext(context);
            releaseContext(context);
            return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
        }

        user_context->deleted = true;
        while (isUserContextBusy(user_context) || areUserContextRequestsBusy(user_context)) {
            unlockContext(context);
            sceKernelUsleep(50000);
            lockContext(context);
        }
    }

    user_context->extendedPushEventCallbacks.clear();
    user_context->servicePushEventCallbacks.clear();
    user_context->pushEventCallbacks.clear();
    user_context->requests.clear();
    context->userContexts.erase(titleUserCtxId);

    unlockContext(context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 createRequest(s32 titleUserCtxId, const char* pApiGroup, const char* pPath,
                  OrbisNpWebApiHttpMethod method,
                  const OrbisNpWebApiContentParameter* pContentParameter,
                  const OrbisNpWebApiIntCreateRequestExtraArgs* pInternalArgs, s64* pRequestId,
                  bool isMultipart) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    lockContext(user_context->parentContext);
    if (g_sdk_ver >= Common::ElfInfo::FW_400 && user_context->deleted) {
        unlockContext(user_context->parentContext);
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    g_request_count++;
    if (g_request_count >> 0x20 != 0) {
        g_request_count = 1;
    }

    s64 user_ctx_id = static_cast<s64>(titleUserCtxId);
    s64 request_id = (user_ctx_id << 0x20) | g_request_count;
    while (user_context->requests.find(request_id) != user_context->requests.end()) {
        request_id--;
    }
    user_context->requests[request_id] = new OrbisNpWebApiRequest{};

    auto& request = user_context->requests[request_id];
    request->parentContext = context;
    request->userCount = 0;
    request->requestId = request_id;
    request->userMethod = method;
    request->multipart = isMultipart;
    request->aborted = false;

    if (pApiGroup != nullptr) {
        request->userApiGroup = std::string(pApiGroup);
    }

    if (pPath != nullptr) {
        request->userPath = std::string(pPath);
    }

    if (pContentParameter != nullptr) {
        request->userContentLength = pContentParameter->content_length;
        if (pContentParameter->content_type != nullptr) {
            request->userContentType = std::string(pContentParameter->content_type);
        }
    }

    unlockContext(user_context->parentContext);

    if (pRequestId != nullptr) {
        *pRequestId = request->requestId;
    }

    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

OrbisNpWebApiRequest* findRequest(OrbisNpWebApiUserContext* userContext, s64 requestId) {
    std::scoped_lock lk{userContext->parentContext->contextLock};
    if (userContext->requests.find(requestId) != userContext->requests.end()) {
        return userContext->requests[requestId];
    }

    return nullptr;
}

OrbisNpWebApiRequest* findRequestAndMarkBusy(OrbisNpWebApiUserContext* userContext, s64 requestId) {
    std::scoped_lock lk{userContext->parentContext->contextLock};
    if (userContext->requests.find(requestId) != userContext->requests.end()) {
        auto& request = userContext->requests[requestId];
        request->userCount++;
        return request;
    }

    return nullptr;
}

bool isRequestBusy(OrbisNpWebApiRequest* request) {
    std::scoped_lock lk{request->parentContext->contextLock};
    return request->userCount > 1;
}

s32 setRequestTimeout(s64 requestId, u32 timeout) {
    OrbisNpWebApiContext* context = findAndValidateContext(requestId >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, requestId >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiRequest* request = findRequestAndMarkBusy(user_context, requestId);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }

    request->requestTimeout = timeout;

    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

void startRequestTimer(OrbisNpWebApiRequest* request) {
    if (request->requestTimeout != 0 && request->requestEndTime == 0) {
        request->requestEndTime = sceKernelGetProcessTime() + request->requestTimeout;
    }
}

void checkRequestTimeout(OrbisNpWebApiRequest* request) {
    u64 time = sceKernelGetProcessTime();
    if (!request->timedOut && request->requestEndTime != 0 && request->requestEndTime < time) {
        request->timedOut = true;
        abortRequest(request->requestId);
    }
}

s32 setMultipartContentType(s64 requestId, const char* pTypeName, const char* pBoundary) {
    OrbisNpWebApiContext* context = findAndValidateContext(requestId >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }
    OrbisNpWebApiUserContext* user_context = findUserContext(context, requestId >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }
    OrbisNpWebApiRequest* request = findRequest(user_context, requestId);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }
    request->multipartContentType = (pTypeName != nullptr) ? pTypeName : "";
    request->multipartBoundary = (pBoundary != nullptr) ? pBoundary : "";
    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 addMultipartPart(s64 requestId, const OrbisNpWebApiMultipartPartParameter* pParam,
                     s32* pIndex) {
    if (pParam == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    OrbisNpWebApiContext* context = findAndValidateContext(requestId >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }
    OrbisNpWebApiUserContext* user_context = findUserContext(context, requestId >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }
    OrbisNpWebApiRequest* request = findRequest(user_context, requestId);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }

    const auto isContentLength = [](const char* name) {
        const char* t = "content-length";
        for (; *name != '\0' && *t != '\0'; ++name, ++t) {
            char c = *name;
            if (c >= 'A' && c <= 'Z') {
                c = static_cast<char>(c - 'A' + 'a');
            }
            if (c != *t) {
                return false;
            }
        }
        return *name == '\0' && *t == '\0';
    };

    OrbisNpWebApiRequest::MultipartPart part;
    part.contentLength = pParam->contentLength;
    std::string headers;
    for (u64 i = 0; i < pParam->headerNum; ++i) {
        const OrbisNpWebApiHttpHeader& h = pParam->pHeaders[i];
        if (h.pName == nullptr || h.pValue == nullptr || isContentLength(h.pName)) {
            continue;
        }
        headers += h.pName;
        headers += ": ";
        headers += h.pValue;
        headers += "\r\n";
    }
    headers += "Content-Length: ";
    headers += std::to_string(pParam->contentLength);
    headers += "\r\n\r\n";
    part.rawHeaders = std::move(headers);

    request->multipartParts.push_back(std::move(part));
    if (pIndex != nullptr) {
        *pIndex = static_cast<s32>(request->multipartParts.size()); // 1-based part index
    }
    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 sendRequest(s64 requestId, s32 partIndex, const void* pData, u64 dataSize, s8 flag,
                OrbisNpWebApiResponseInformationOption* pRespInfoOption) {
    OrbisNpWebApiContext* context = findAndValidateContext(requestId >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, requestId >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiRequest* request = findRequestAndMarkBusy(user_context, requestId);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }

    startRequestTimer(request);

    std::string multipartBody;
    const void* sendData = pData;
    u64 sendSize = dataSize;
    if (request->multipart) {
        if (partIndex < 1 || partIndex > static_cast<s32>(request->multipartParts.size())) {
            releaseRequest(request);
            releaseUserContext(user_context);
            releaseContext(context);
            return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
        }
        auto& part = request->multipartParts[partIndex - 1];
        if (pData != nullptr && dataSize > 0) {
            part.data.append(static_cast<const char*>(pData), dataSize);
        }
        part.sentSize += dataSize;

        bool allComplete = true;
        for (const auto& p : request->multipartParts) {
            if (p.sentSize < p.contentLength) {
                allComplete = false;
                break;
            }
        }
        if (!allComplete) {
            releaseRequest(request);
            releaseUserContext(user_context);
            releaseContext(context);
            return ORBIS_OK;
        }

        std::string boundary = request->multipartBoundary;
        if (boundary.empty()) {
            static const char kHex[] = "0123456789abcdef";
            boundary = "----shadPS4Boundary";
            const u64 v = static_cast<u64>(requestId);
            for (int shift = 60; shift >= 0; shift -= 4) {
                boundary += kHex[(v >> shift) & 0xf];
            }
        }
        const std::string typeName = request->multipartContentType.empty()
                                         ? std::string("multipart/mixed")
                                         : request->multipartContentType;
        for (const auto& p : request->multipartParts) {
            multipartBody += "--";
            multipartBody += boundary;
            multipartBody += "\r\n";
            multipartBody += p.rawHeaders;
            multipartBody += p.data;
            multipartBody += "\r\n";
        }
        multipartBody += "--";
        multipartBody += boundary;
        multipartBody += "--\r\n";

        request->userContentType = typeName + "; boundary=" + boundary;
        request->userContentLength = multipartBody.size();
        sendData = multipartBody.data();
        sendSize = multipartBody.size();
    }

    if (g_sdk_ver >= Common::ElfInfo::FW_250 && !request->sent) {
        request->sent = true;
    }

    lockContext(context);
    if (!request->timedOut && request->aborted) {
        unlockContext(context);
        releaseRequest(request);
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_ABORTED;
    }

    unlockContext(context);

    // Stubbing sceNpManagerIntGetSigninState call with a config check.
    auto& np_handler = Libraries::Np::NpHandler::Instance();
    if (!np_handler.IsActive()) {
        releaseRequest(request);
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
    }

    auto& config = ShadNet::Settings::GetInstance();
    if (request->http_request_id == 0) {
        std::string base_url = config.GetWebApiServerUrl();
        const s32 tmpl_id = sceHttpCreateTemplate(context->libHttpCtxId, "libhttp", 2, 0);
        if (tmpl_id < 0) {
            LOG_ERROR(Lib_NpWebApi, "sceHttpCreateTemplate failed: {:#x}", tmpl_id);
            releaseRequest(request);
            releaseUserContext(user_context);
            releaseContext(context);
            return tmpl_id;
        }
        request->http_template_id = tmpl_id;
        const int conn_id = sceHttpCreateConnectionWithURL(tmpl_id, base_url.c_str(), true);
        if (conn_id < 0) {
            LOG_ERROR(Lib_NpWebApi, "sceHttpCreateConnectionWithURL failed: {:#x}", conn_id);
            sceHttpDeleteTemplate(tmpl_id);
            request->http_template_id = 0;
            releaseRequest(request);
            releaseUserContext(user_context);
            releaseContext(context);
            return conn_id;
        }
        request->http_connection_id = conn_id;
        s32 sceMethod;
        switch (request->userMethod) {
        case ORBIS_NP_WEBAPI_HTTP_METHOD_GET:
            sceMethod = 0;
            break;
        case ORBIS_NP_WEBAPI_HTTP_METHOD_POST:
            sceMethod = 1;
            break;
        case ORBIS_NP_WEBAPI_HTTP_METHOD_PUT:
            sceMethod = 4;
            break;
        case ORBIS_NP_WEBAPI_HTTP_METHOD_DELETE:
            sceMethod = 5;
            break;
        case ORBIS_NP_WEBAPI_HTTP_METHOD_PATCH:
            sceMethod = 8;
            break;
        default:
            LOG_ERROR(Lib_NpWebApi, "unknown method enum value {}",
                      static_cast<int>(request->userMethod));
            releaseRequest(request);
            releaseUserContext(user_context);
            releaseContext(context);
            return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
        }
        const std::string full_url = base_url + request->userPath;
        const int req_id = sceHttpCreateRequestWithURL(conn_id, sceMethod, full_url.c_str(),
                                                       request->userContentLength);
        if (req_id < 0) {
            LOG_ERROR(Lib_NpWebApi, "sceHttpCreateRequestWithURL failed: {:#x}", req_id);
            sceHttpDeleteConnection(conn_id);
            request->http_connection_id = 0;
            sceHttpDeleteTemplate(tmpl_id);
            request->http_template_id = 0;
            releaseRequest(request);
            releaseUserContext(user_context);
            releaseContext(context);
            return req_id;
        }
        request->http_request_id = req_id;

        if (!request->userContentType.empty()) {
            sceHttpAddRequestHeader(req_id, "Content-Type", request->userContentType.c_str(), 0);
        }

        auto& np_handler = NpHandler::Instance();
        const std::string bearer = np_handler.GetBearerToken(user_context->userId);
        if (!bearer.empty()) {
            const std::string auth_value = "Bearer " + bearer;
            sceHttpAddRequestHeader(req_id, "Authorization", auth_value.c_str(), 0);
        } else {
            LOG_WARNING(Lib_NpWebApi,
                        "no bearer token for user_id={}; request to '{}' will "
                        "be unauthenticated",
                        user_context->userId, request->userPath);
        }

        const auto isContentType = [](const std::string& name) {
            constexpr std::string_view target = "content-type";
            if (name.size() != target.size()) {
                return false;
            }
            for (size_t i = 0; i < name.size(); ++i) {
                char c = name[i];
                if (c >= 'A' && c <= 'Z') {
                    c = static_cast<char>(c - 'A' + 'a');
                }
                if (c != target[i]) {
                    return false;
                }
            }
            return true;
        };
        const bool haveContentType = !request->userContentType.empty();
        for (const auto& [hname, hvalue] : request->userHeaders) {
            if (haveContentType && isContentType(hname)) {
                continue;
            }
            sceHttpAddRequestHeader(req_id, hname.c_str(), hvalue.c_str(), 0);
        }
    }

    setRequestState(request, 4);

    const s32 send_err = sceHttpSendRequest(request->http_request_id, sendData, sendSize);
    if (send_err < 0) {
        LOG_ERROR(Lib_NpWebApi, "sceHttpSendRequest failed: {:#x}", send_err);
        releaseRequest(request);
        releaseUserContext(user_context);
        releaseContext(context);
        return send_err;
    }

    LOG_INFO(Lib_NpWebApi, "requestId={:#x} apiGroup='{}' path='{}' method={} httpReqId={}",
             requestId, request->userApiGroup, request->userPath,
             magic_enum::enum_name(request->userMethod), request->http_request_id);

    s32 sendResult = ORBIS_OK;
    if (flag != 0) {
        s32 status = 0;
        if (sceHttpGetStatusCode(request->http_request_id, &status) >= 0) {
            if (pRespInfoOption != nullptr) {
                pRespInfoOption->http_status = status;
            }
            if (status >= 400) {
                std::string errBody;
                char buf[256];
                for (;;) {
                    const s32 n = sceHttpReadData(request->http_request_id, buf, sizeof(buf));
                    if (n <= 0)
                        break;
                    errBody.append(buf, static_cast<size_t>(n));
                    if (errBody.size() > 64u * 1024u)
                        break; // sanity cap on a runaway error body
                }
                if (pRespInfoOption != nullptr) {
                    pRespInfoOption->response_data_size = errBody.size();
                    char* const dst = reinterpret_cast<char* const>(pRespInfoOption->error_object);
                    const u64 cap = pRespInfoOption->error_object_size;
                    if (dst != nullptr && cap > 0) {
                        const u64 n = std::min<u64>(errBody.size(), cap - 1);
                        if (n > 0)
                            std::memcpy(dst, errBody.data(), n);
                        dst[n] = '\0';
                    }
                }
                const s32 npErr = captureWebApiError(errBody.data(), errBody.size());
                if (npErr > 0 && npErr < 0xc00000) {
                    sendResult = static_cast<s32>(0x82000000u | static_cast<u32>(npErr));
                } else if (status >= 100 && status < 600) {
                    sendResult = static_cast<s32>(0x82f00000u | static_cast<u32>(status));
                } else {
                    sendResult = static_cast<s32>(0x82ffffffu);
                }
            }
        }
    }

    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return sendResult;
}

s32 abortRequestInternal(OrbisNpWebApiContext* context, OrbisNpWebApiUserContext* userContext,
                         OrbisNpWebApiRequest* request) {
    if (context == nullptr || userContext == nullptr || request == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{context->contextLock};
    if (request->aborted) {
        return ORBIS_OK;
    }

    request->aborted = true;
    if (request->http_request_id) {
        sceHttpAbortRequest(request->http_request_id);
    }
    return ORBIS_OK;
}

s32 abortRequest(s64 requestId) {
    OrbisNpWebApiContext* context = findAndValidateContext(requestId >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, requestId >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiRequest* request = findRequest(user_context, requestId);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }

    s32 result = abortRequestInternal(context, user_context, request);

    releaseUserContext(user_context);
    releaseContext(context);
    return result;
}

void releaseRequest(OrbisNpWebApiRequest* request) {
    std::scoped_lock lk{request->parentContext->contextLock};
    request->userCount--;
}

s32 deleteRequest(s64 requestId) {
    OrbisNpWebApiContext* context = findAndValidateContext(requestId >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    lockContext(context);
    OrbisNpWebApiUserContext* user_context = findUserContext(context, requestId >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiRequest* request = findRequestAndMarkBusy(user_context, requestId);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }

    if (g_sdk_ver < Common::ElfInfo::FW_400 && isRequestBusy(request)) {
        releaseRequest(request);
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_BUSY;
    }

    abortRequestInternal(context, user_context, request);
    while (isRequestBusy(request)) {
        unlockContext(context);
        sceKernelUsleep(50000);
        lockContext(context);
    }

    releaseRequest(request);
    if (request->http_request_id != 0) {
        sceHttpDeleteRequest(request->http_request_id);
        request->http_request_id = 0;
    }
    if (request->http_connection_id != 0) {
        sceHttpDeleteConnection(request->http_connection_id);
        request->http_connection_id = 0;
    }
    if (request->http_template_id != 0) {
        sceHttpDeleteTemplate(request->http_template_id);
        request->http_template_id = 0;
    }
    user_context->requests.erase(request->requestId);

    releaseUserContext(user_context);
    unlockContext(context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 createHandleInternal(OrbisNpWebApiContext* context) {
    g_handle_count++;
    if (g_handle_count >= 0xf0000000) {
        g_handle_count = 1;
    }

    std::scoped_lock lk{context->contextLock};

    s32 handle_id = g_handle_count;
    context->handles[handle_id] = new OrbisNpWebApiHandle{};
    auto& handle = context->handles[handle_id];
    handle->handleId = handle_id;
    handle->userCount = 0;
    handle->aborted = false;
    handle->deleted = false;

    if (g_sdk_ver >= Common::ElfInfo::FW_300) {
        context->timerHandles[handle_id] = new OrbisNpWebApiTimerHandle{};
        auto& timer_handle = context->timerHandles[handle_id];
        timer_handle->handleId = handle_id;
        timer_handle->timedOut = false;
        timer_handle->handleTimeout = 0;
        timer_handle->handleEndTime = 0;
    }

    return handle_id;
}

s32 createHandle(s32 libCtxId) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = createHandleInternal(context);
    releaseContext(context);
    return result;
}

s32 setHandleTimeoutInternal(OrbisNpWebApiContext* context, s32 handleId, u32 timeout) {
    std::scoped_lock lk{context->contextLock};
    if (context->timerHandles.find(handleId) == context->timerHandles.end()) {
        return ORBIS_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND;
    }
    auto& handle = context->handles[handleId];
    handle->userCount++;

    auto& timer_handle = context->timerHandles[handleId];
    timer_handle->handleTimeout = timeout;

    handle->userCount--;
    return ORBIS_OK;
}

s32 setHandleTimeout(s32 libCtxId, s32 handleId, u32 timeout) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = setHandleTimeoutInternal(context, handleId, timeout);
    releaseContext(context);
    return result;
}

void startHandleTimer(OrbisNpWebApiContext* context, s32 handleId) {
    std::scoped_lock lk{context->contextLock};
    if (context->timerHandles.find(handleId) == context->timerHandles.end()) {
        return;
    }
    auto& timer_handle = context->timerHandles[handleId];
    if (timer_handle->handleTimeout == 0) {
        return;
    }
    timer_handle->handleEndTime = sceKernelGetProcessTime() + timer_handle->handleTimeout;
}

void releaseHandle(OrbisNpWebApiContext* context, OrbisNpWebApiHandle* handle) {
    if (handle != nullptr) {
        std::scoped_lock lk{context->contextLock};
        handle->userCount--;
    }
}

s32 getHandle(OrbisNpWebApiContext* context, s32 handleId, OrbisNpWebApiHandle** handleOut) {
    std::scoped_lock lk{context->contextLock};
    if (context->handles.find(handleId) == context->handles.end()) {
        return ORBIS_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND;
    }
    auto& handle = context->handles[handleId];
    handle->userCount++;
    if (handleOut != nullptr) {
        *handleOut = handle;
    }
    return ORBIS_OK;
}

s32 abortHandle(s32 libCtxId, s32 handleId) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiHandle* handle;
    s32 result = getHandle(context, handleId, &handle);
    if (result == ORBIS_OK) {
        std::scoped_lock lk{context->contextLock};
        handle->aborted = true;
        // TODO: sceNpAsmClientAbortRequest call
        releaseHandle(context, handle);
    }

    releaseContext(context);
    return result;
}

s32 deleteHandleInternal(OrbisNpWebApiContext* context, s32 handleId) {
    lockContext(context);
    if (context->handles.find(handleId) == context->handles.end()) {
        return ORBIS_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND;
    }

    auto& handle = context->handles[handleId];
    if (g_sdk_ver >= Common::ElfInfo::FW_400) {
        if (handle->deleted) {
            unlockContext(context);
            return ORBIS_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND;
        }
        handle->deleted = true;
        unlockContext(context);
        abortHandle(context->libCtxId, handleId);
        lockContext(context);
        handle->userCount++;
        while (handle->userCount > 1) {
            handle->userCount--;
            unlockContext(context);
            sceKernelUsleep(50000);
            lockContext(context);
            handle->userCount++;
        }
        handle->userCount--;
    } else if (handle->userCount > 0) {
        unlockContext(context);
        return ORBIS_NP_WEBAPI_ERROR_HANDLE_BUSY;
    }

    context->handles.erase(handleId);

    if (g_sdk_ver >= Common::ElfInfo::FW_300 &&
        context->timerHandles.find(handleId) != context->timerHandles.end()) {
        context->timerHandles.erase(handleId);
    }

    unlockContext(context);
    return ORBIS_OK;
}

s32 deleteHandle(s32 libCtxId, s32 handleId) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = deleteHandleInternal(context, handleId);
    releaseContext(context);
    return result;
}

s32 createPushEventFilterInternal(OrbisNpWebApiContext* context,
                                  const OrbisNpWebApiPushEventFilterParameter* pFilterParam,
                                  u64 filterParamNum) {
    std::scoped_lock lk{context->contextLock};
    g_push_event_filter_count++;
    if (g_push_event_filter_count >= 0xf0000000) {
        g_push_event_filter_count = 1;
    }
    s32 filterId = g_push_event_filter_count;

    context->pushEventFilters[filterId] = new OrbisNpWebApiPushEventFilter{};
    auto& filter = context->pushEventFilters[filterId];
    filter->parentContext = context;
    filter->filterId = filterId;

    LOG_INFO(Lib_NpWebApi, "filterId={} dataTypeParams={}", filterId, filterParamNum);
    if (pFilterParam != nullptr && filterParamNum != 0) {
        for (u64 param_idx = 0; param_idx < filterParamNum; param_idx++) {
            OrbisNpWebApiPushEventFilterParameter copy = OrbisNpWebApiPushEventFilterParameter{};
            memcpy(&copy, &pFilterParam[param_idx], sizeof(OrbisNpWebApiPushEventFilterParameter));
            LOG_INFO(Lib_NpWebApi, "  filterParam[{}] dataType='{}'", param_idx,
                     copy.data_type.val);
            filter->filterParams.emplace_back(copy);
        }
    }
    return filterId;
}

s32 createPushEventFilter(s32 libCtxId, const OrbisNpWebApiPushEventFilterParameter* pFilterParam,
                          u64 filterParamNum) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = createPushEventFilterInternal(context, pFilterParam, filterParamNum);
    releaseContext(context);
    return result;
}

s32 deletePushEventFilterInternal(OrbisNpWebApiContext* context, s32 filterId) {
    std::scoped_lock lk{context->contextLock};
    if (context->pushEventFilters.find(filterId) == context->pushEventFilters.end()) {
        return ORBIS_NP_WEBAPI_ERROR_PUSH_EVENT_FILTER_NOT_FOUND;
    }

    context->pushEventFilters[filterId]->filterParams.clear();
    context->pushEventFilters.erase(filterId);
    return ORBIS_OK;
}

s32 deletePushEventFilter(s32 libCtxId, s32 filterId) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = deletePushEventFilterInternal(context, filterId);
    releaseContext(context);
    return result;
}

s32 registerPushEventCallbackInternal(OrbisNpWebApiUserContext* userContext, s32 filterId,
                                      OrbisNpWebApiPushEventCallback cbFunc, void* pUserArg) {
    std::scoped_lock lk{userContext->parentContext->contextLock};
    g_registered_callback_count++;
    if (g_registered_callback_count >= 0xf0000000) {
        g_registered_callback_count = 1;
    }
    s32 cbId = g_registered_callback_count;

    userContext->pushEventCallbacks[cbId] = new OrbisNpWebApiRegisteredPushEventCallback{};
    auto& cb = userContext->pushEventCallbacks[cbId];
    cb->callbackId = cbId;
    cb->filterId = filterId;
    cb->cbFunc = cbFunc;
    cb->pUserArg = pUserArg;

    return cbId;
}

s32 registerPushEventCallback(s32 titleUserCtxId, s32 filterId,
                              OrbisNpWebApiPushEventCallback cbFunc, void* pUserArg) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (g_sdk_ver >= Common::ElfInfo::FW_250 &&
        context->pushEventFilters.find(filterId) == context->pushEventFilters.end()) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_PUSH_EVENT_FILTER_NOT_FOUND;
    }

    s32 result = registerPushEventCallbackInternal(user_context, filterId, cbFunc, pUserArg);
    releaseUserContext(user_context);
    releaseContext(context);
    return result;
}

s32 unregisterPushEventCallback(s32 titleUserCtxId, s32 callbackId) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (user_context->pushEventCallbacks.find(callbackId) ==
        user_context->pushEventCallbacks.end()) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_PUSH_EVENT_CALLBACK_NOT_FOUND;
    }

    lockContext(context);
    user_context->pushEventCallbacks.erase(callbackId);
    unlockContext(context);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 createServicePushEventFilterInternal(
    OrbisNpWebApiContext* context, s32 handleId, const char* pNpServiceName, u32 npServiceLabel,
    const OrbisNpWebApiServicePushEventFilterParameter* pFilterParam, u64 filterParamNum) {
    std::scoped_lock lk{context->contextLock};
    if (context->handles.find(handleId) == context->handles.end()) {
        return ORBIS_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND;
    }
    auto& handle = context->handles[handleId];
    handle->userCount++;

    auto& np_handler = Libraries::Np::NpHandler::Instance();
    if (pNpServiceName != nullptr && !np_handler.IsActive()) {
        // Seems sceNpManagerIntGetUserList fails?
        LOG_DEBUG(Lib_NpWebApi, "Cannot create service push event while shadNet is disabled");
        handle->userCount--;
        return ORBIS_NP_WEBAPI_ERROR_SIGNED_IN_USER_NOT_FOUND;
    }

    g_service_push_event_filter_count++;
    if (g_service_push_event_filter_count >= 0xf0000000) {
        g_service_push_event_filter_count = 1;
    }
    s32 filterId = g_service_push_event_filter_count;

    context->servicePushEventFilters[filterId] = new OrbisNpWebApiServicePushEventFilter{};
    auto& filter = context->servicePushEventFilters[filterId];
    filter->parentContext = context;
    filter->filterId = filterId;

    if (pNpServiceName == nullptr) {
        filter->internal = true;
    } else {
        filter->npServiceName = std::string(pNpServiceName);
    }

    filter->npServiceLabel = npServiceLabel;
    LOG_INFO(Lib_NpWebApi, "filterId={} dataTypeParams={}", filterId, filterParamNum);
    if (pFilterParam != nullptr && filterParamNum != 0) {
        for (u64 param_idx = 0; param_idx < filterParamNum; param_idx++) {
            OrbisNpWebApiServicePushEventFilterParameter copy =
                OrbisNpWebApiServicePushEventFilterParameter{};
            memcpy(&copy, &pFilterParam[param_idx],
                   sizeof(OrbisNpWebApiServicePushEventFilterParameter));
            LOG_INFO(Lib_NpWebApi, "  filterParam[{}] data_type='{}'", param_idx,
                     copy.data_type.val);
            filter->filterParams.emplace_back(copy);
        }
    }

    handle->userCount--;
    return filterId;
}

s32 createServicePushEventFilter(s32 libCtxId, s32 handleId, const char* pNpServiceName,
                                 u32 npServiceLabel,
                                 const OrbisNpWebApiServicePushEventFilterParameter* pFilterParam,
                                 u64 filterParamNum) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    startHandleTimer(context, handleId);
    s32 result = createServicePushEventFilterInternal(context, handleId, pNpServiceName,
                                                      npServiceLabel, pFilterParam, filterParamNum);
    releaseContext(context);
    return result;
}

s32 deleteServicePushEventFilterInternal(OrbisNpWebApiContext* context, s32 filterId) {
    std::scoped_lock lk{context->contextLock};
    if (context->servicePushEventFilters.find(filterId) == context->servicePushEventFilters.end()) {
        return ORBIS_NP_WEBAPI_ERROR_SERVICE_PUSH_EVENT_FILTER_NOT_FOUND;
    }

    context->servicePushEventFilters[filterId]->filterParams.clear();
    context->servicePushEventFilters.erase(filterId);
    return ORBIS_OK;
}

s32 deleteServicePushEventFilter(s32 libCtxId, s32 filterId) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = deleteServicePushEventFilterInternal(context, filterId);
    releaseContext(context);
    return result;
}

s32 registerServicePushEventCallbackInternal(
    OrbisNpWebApiUserContext* userContext, s32 filterId,
    OrbisNpWebApiServicePushEventCallback cbFunc,
    OrbisNpWebApiInternalServicePushEventCallback intCbFunc,
    OrbisNpWebApiInternalServicePushEventCallbackA intCbFuncA, void* pUserArg) {
    std::scoped_lock lk{userContext->parentContext->contextLock};
    if (cbFunc == nullptr && intCbFunc == nullptr && intCbFuncA == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    g_registered_callback_count++;
    if (g_registered_callback_count >= 0xf0000000) {
        g_registered_callback_count = 1;
    }
    s32 cbId = g_registered_callback_count;

    userContext->servicePushEventCallbacks[cbId] =
        new OrbisNpWebApiRegisteredServicePushEventCallback{};
    auto& cb = userContext->servicePushEventCallbacks[cbId];
    cb->callbackId = cbId;
    cb->filterId = filterId;
    cb->cbFunc = cbFunc;
    cb->internalCbFunc = intCbFunc;
    cb->internalCbFuncA = intCbFuncA;
    cb->pUserArg = pUserArg;

    return cbId;
}

s32 registerServicePushEventCallback(s32 titleUserCtxId, s32 filterId,
                                     OrbisNpWebApiServicePushEventCallback cbFunc,
                                     OrbisNpWebApiInternalServicePushEventCallback intCbFunc,
                                     OrbisNpWebApiInternalServicePushEventCallbackA intCbFuncA,
                                     void* pUserArg) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (g_sdk_ver >= Common::ElfInfo::FW_250 &&
        context->servicePushEventFilters.find(filterId) == context->servicePushEventFilters.end()) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_SERVICE_PUSH_EVENT_FILTER_NOT_FOUND;
    }

    s32 result = registerServicePushEventCallbackInternal(user_context, filterId, cbFunc, intCbFunc,
                                                          intCbFuncA, pUserArg);
    releaseUserContext(user_context);
    releaseContext(context);
    return result;
}

s32 unregisterServicePushEventCallback(s32 titleUserCtxId, s32 callbackId) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (user_context->servicePushEventCallbacks.find(callbackId) ==
        user_context->servicePushEventCallbacks.end()) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_SERVICE_PUSH_EVENT_CALLBACK_NOT_FOUND;
    }

    lockContext(context);
    user_context->servicePushEventCallbacks.erase(callbackId);
    unlockContext(context);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 createExtendedPushEventFilterInternal(
    OrbisNpWebApiContext* context, s32 handleId, const char* pNpServiceName, u32 npServiceLabel,
    const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam, u64 filterParamNum,
    bool internal) {
    std::scoped_lock lk{context->contextLock};
    if (context->handles.find(handleId) == context->handles.end()) {
        return ORBIS_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND;
    }
    auto& handle = context->handles[handleId];
    handle->userCount++;

    auto& np_handler = Libraries::Np::NpHandler::Instance();
    if (pNpServiceName != nullptr && !np_handler.IsActive()) {
        // Seems sceNpManagerIntGetUserList fails?
        LOG_DEBUG(Lib_NpWebApi, "Cannot create extended push event while shadNet is disabled");
        handle->userCount--;
        return ORBIS_NP_WEBAPI_ERROR_SIGNED_IN_USER_NOT_FOUND;
    }

    g_extended_push_event_filter_count++;
    if (g_extended_push_event_filter_count >= 0xf0000000) {
        g_extended_push_event_filter_count = 1;
    }
    s32 filterId = g_extended_push_event_filter_count;

    context->extendedPushEventFilters[filterId] = new OrbisNpWebApiExtendedPushEventFilter{};
    auto& filter = context->extendedPushEventFilters[filterId];
    filter->internal = internal;
    filter->parentContext = context;
    filter->filterId = filterId;

    if (pNpServiceName == nullptr) {
        npServiceLabel = -1;
    } else {
        filter->npServiceName = std::string(pNpServiceName);
    }

    filter->npServiceLabel = npServiceLabel;

    LOG_INFO(Lib_NpWebApi, "filterId={} service='{}' label={:#x} dataTypeParams={}", filterId,
             pNpServiceName ? pNpServiceName : "null", npServiceLabel, filterParamNum);
    if (pFilterParam != nullptr && filterParamNum != 0) {
        for (u64 param_idx = 0; param_idx < filterParamNum; param_idx++) {
            OrbisNpWebApiExtdPushEventFilterParameter copy =
                OrbisNpWebApiExtdPushEventFilterParameter{};
            memcpy(&copy, &pFilterParam[param_idx],
                   sizeof(OrbisNpWebApiExtdPushEventFilterParameter));
            LOG_INFO(Lib_NpWebApi, "  filterParam[{}] dataType='{}' extdKeys={}", param_idx,
                     copy.data_type.val, copy.extd_data_key_num);
            if (copy.extd_data_key != nullptr) {
                for (u64 k = 0; k < copy.extd_data_key_num; k++) {
                    LOG_INFO(Lib_NpWebApi, "    extdDataKey[{}]='{}'", k,
                             copy.extd_data_key[k].val);
                }
            }
            filter->filterParams.emplace_back(copy);
        }
    }

    handle->userCount--;
    return filterId;
}

s32 createExtendedPushEventFilter(s32 libCtxId, s32 handleId, const char* pNpServiceName,
                                  u32 npServiceLabel,
                                  const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam,
                                  u64 filterParamNum, bool internal) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    startHandleTimer(context, handleId);
    s32 result = createExtendedPushEventFilterInternal(
        context, handleId, pNpServiceName, npServiceLabel, pFilterParam, filterParamNum, internal);
    releaseContext(context);
    return result;
}

s32 deleteExtendedPushEventFilterInternal(OrbisNpWebApiContext* context, s32 filterId) {
    std::scoped_lock lk{context->contextLock};
    if (context->extendedPushEventFilters.find(filterId) ==
        context->extendedPushEventFilters.end()) {
        return ORBIS_NP_WEBAPI_ERROR_EXTD_PUSH_EVENT_FILTER_NOT_FOUND;
    }

    context->extendedPushEventFilters[filterId]->filterParams.clear();
    context->extendedPushEventFilters.erase(filterId);
    return ORBIS_OK;
}

s32 deleteExtendedPushEventFilter(s32 libCtxId, s32 filterId) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = deleteExtendedPushEventFilterInternal(context, filterId);
    releaseContext(context);
    return result;
}

s32 registerExtdPushEventCallbackInternal(OrbisNpWebApiUserContext* userContext, s32 filterId,
                                          OrbisNpWebApiExtdPushEventCallback cbFunc,
                                          OrbisNpWebApiExtdPushEventCallbackA cbFuncA,
                                          void* pUserArg) {
    std::scoped_lock lk{userContext->parentContext->contextLock};

    g_registered_callback_count++;
    if (g_registered_callback_count >= 0xf0000000) {
        g_registered_callback_count = 1;
    }

    if (cbFunc == nullptr && cbFuncA == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    s32 cbId = g_registered_callback_count;

    userContext->extendedPushEventCallbacks[cbId] =
        new OrbisNpWebApiRegisteredExtendedPushEventCallback{};
    auto& cb = userContext->extendedPushEventCallbacks[cbId];
    cb->callbackId = cbId;
    cb->filterId = filterId;
    cb->cbFunc = cbFunc;
    cb->cbFuncA = cbFuncA;
    cb->pUserArg = pUserArg;

    return cbId;
}

s32 registerExtdPushEventCallback(s32 titleUserCtxId, s32 filterId,
                                  OrbisNpWebApiExtdPushEventCallback cbFunc,
                                  OrbisNpWebApiExtdPushEventCallbackA cbFuncA, void* pUserArg) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (context->extendedPushEventFilters.find(filterId) ==
        context->extendedPushEventFilters.end()) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_EXTD_PUSH_EVENT_FILTER_NOT_FOUND;
    }

    s32 result =
        registerExtdPushEventCallbackInternal(user_context, filterId, cbFunc, cbFuncA, pUserArg);
    releaseUserContext(user_context);
    releaseContext(context);
    return result;
}

s32 registerExtdPushEventCallbackA(s32 titleUserCtxId, s32 filterId,
                                   OrbisNpWebApiExtdPushEventCallbackA cbFunc, void* pUserArg) {
    return registerExtdPushEventCallback(titleUserCtxId, filterId, nullptr, cbFunc, pUserArg);
}

s32 unregisterExtdPushEventCallback(s32 titleUserCtxId, s32 callbackId) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (user_context->extendedPushEventCallbacks.find(callbackId) ==
        user_context->extendedPushEventCallbacks.end()) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_EXTD_PUSH_EVENT_CALLBACK_NOT_FOUND;
    }

    lockContext(context);
    user_context->extendedPushEventCallbacks.erase(callbackId);
    unlockContext(context);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI getHttpRequestIdFromRequest(OrbisNpWebApiRequest* request) {
    return request->http_request_id;
}

// Request/response header support
bool iEquals(const char* a, u64 aLen, const char* b) {
    u64 i = 0;
    for (; i < aLen && b[i]; ++i) {
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i]))
            return false;
    }
    return i == aLen && b[i] == '\0';
}

bool findHeaderValue(const char* block, u64 blockLen, const char* field, std::string& outValue) {
    u64 i = 0;
    while (i < blockLen) {
        u64 lineStart = i;
        while (i < blockLen && block[i] != '\n')
            ++i;
        u64 lineEnd = i;
        if (i < blockLen)
            ++i;
        if (lineEnd > lineStart && block[lineEnd - 1] == '\r')
            --lineEnd;
        u64 colon = lineStart;
        while (colon < lineEnd && block[colon] != ':')
            ++colon;
        if (colon >= lineEnd)
            continue;
        u64 nameLen = colon - lineStart;
        if (!iEquals(block + lineStart, nameLen, field))
            continue;
        u64 vs = colon + 1;
        while (vs < lineEnd && (block[vs] == ' ' || block[vs] == '\t'))
            ++vs;
        outValue.assign(block + vs, lineEnd - vs);
        return true;
    }
    return false;
}

bool parseErrorCode(const char* body, u64 len, s32& out) {
    std::string_view sv(body, len);
    size_t k = sv.find("\"code\"");
    if (k == std::string_view::npos)
        return false;
    k += 6;
    while (k < sv.size() && (sv[k] == ' ' || sv[k] == ':' || sv[k] == '"'))
        ++k;
    bool neg = false;
    if (k < sv.size() && (sv[k] == '-' || sv[k] == '+')) {
        neg = sv[k] == '-';
        ++k;
    }
    if (k >= sv.size() || !std::isdigit((unsigned char)sv[k]))
        return false;
    long long v = 0;
    while (k < sv.size() && std::isdigit((unsigned char)sv[k])) {
        v = v * 10 + (sv[k] - '0');
        ++k;
    }
    out = static_cast<s32>(neg ? -v : v);
    return true;
}

static s32 captureWebApiError(const char* body, u64 len) {
    s32 code = 0;
    if (body != nullptr && len > 0 && parseErrorCode(body, len, code)) {
        g_last_webapi_error = code;
        return code;
    }
    return 0;
}

s32 getLastWebApiError() {
    return g_last_webapi_error;
}

s32 addHttpRequestHeaderInternal(s64 requestId, const char* pFieldName, const char* pValue) {
    OrbisNpWebApiContext* context = findAndValidateContext(requestId >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }
    OrbisNpWebApiUserContext* user_context = findUserContext(context, requestId >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }
    OrbisNpWebApiRequest* request = findRequestAndMarkBusy(user_context, requestId);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }
    request->userHeaders.emplace_back(pFieldName, pValue);
    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 getHttpResponseHeaderValueInternal(s64 requestId, const char* pFieldName, char* pValue,
                                       u64 valueSize, u64* pValueLength) {
    OrbisNpWebApiContext* context = findAndValidateContext(requestId >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }
    OrbisNpWebApiUserContext* user_context = findUserContext(context, requestId >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }
    OrbisNpWebApiRequest* request = findRequestAndMarkBusy(user_context, requestId);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }

    char* block = nullptr;
    u64 blockSize = 0;
    const s32 httpReqId = getHttpRequestIdFromRequest(request);
    const s32 err = sceHttpGetAllResponseHeaders(httpReqId, &block, &blockSize);

    s32 result = ORBIS_OK;
    if (err < 0) {
        result = err;
    } else {
        std::string value;
        const bool found = block != nullptr && findHeaderValue(block, blockSize, pFieldName, value);
        if (pValueLength != nullptr)
            *pValueLength = found ? value.size() : 0;
        if (pValue != nullptr && valueSize > 0) {
            const u64 n = found ? std::min<u64>(value.size(), valueSize - 1) : 0;
            if (n > 0)
                std::memcpy(pValue, value.data(), n);
            pValue[n] = '\0';
        }
    }

    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return result;
}

s32 PS4_SYSV_ABI getHttpStatusCodeInternal(s64 requestId, s32* out_status_code) {
    s32 status_code;
    OrbisNpWebApiContext* context = findAndValidateContext(requestId >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, requestId >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiRequest* request = findRequestAndMarkBusy(user_context, requestId);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }

    // Query HTTP layer
    {
        int32_t httpReqId = getHttpRequestIdFromRequest(request);
        s32 err = sceHttpGetStatusCode(httpReqId, &status_code);

        if (out_status_code != nullptr)
            *out_status_code = status_code;

        releaseRequest(request);
        releaseUserContext(user_context);
        releaseContext(context);

        return err;
    }
}

void PS4_SYSV_ABI setRequestEndTime(OrbisNpWebApiRequest* request) {
    u64 time;
    if ((request->requestTimeout != 0) && (request->requestEndTime == 0)) {
        time = sceKernelGetProcessTime();
        request->requestEndTime = (u64)request->requestTimeout + time;
    }
}

void PS4_SYSV_ABI clearRequestEndTime(OrbisNpWebApiRequest* req) {
    req->requestEndTime = 0;
    return;
}

bool PS4_SYSV_ABI hasRequestTimedOut(OrbisNpWebApiRequest* request) {
    return request->timedOut;
}

bool PS4_SYSV_ABI isRequestAborted(OrbisNpWebApiRequest* request) {
    return request->aborted;
}

void PS4_SYSV_ABI setRequestState(OrbisNpWebApiRequest* request, u8 state) {
    request->requestState = state;
}

u64 PS4_SYSV_ABI copyRequestData(OrbisNpWebApiRequest* request, void* data, u64 size) {
    u64 readSize = 0;

    if (request->remainingData != 0) {
        u64 remainingSize = request->remainingData - request->readOffset;

        if (remainingSize != 0) {
            if (remainingSize < size) {
                size = remainingSize;
            }
            memcpy(data, request->data + request->readOffset, size);
            request->readOffset += static_cast<u32>(size);
            readSize = size;
        }
    }
    return readSize;
}

s32 PS4_SYSV_ABI readDataInternal(s64 requestId, void* pData, u64 size) {
    u32 offset;
    s32 result;
    u64 remainingSize;
    u64 bytesCopied;

    OrbisNpWebApiContext* context = findAndValidateContext(requestId >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, requestId >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiRequest* request = findRequestAndMarkBusy(user_context, requestId);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }

    setRequestEndTime(request);

    bytesCopied = copyRequestData(request, pData, size);
    offset = (u32)bytesCopied;
    remainingSize = size - offset;

    if (remainingSize != 0) {
        lockContext(context);
        setRequestState(request, 5);

        if (!hasRequestTimedOut(request) && isRequestAborted(request)) {
            unlockContext(context);
            offset = ORBIS_NP_WEBAPI_ERROR_ABORTED;
        } else {
            unlockContext(context);

            s32 httpReqId = getHttpRequestIdFromRequest(request);
            s32 httpRead =
                sceHttpReadData(httpReqId, reinterpret_cast<u8*>(pData) + offset, remainingSize);

            if (httpRead < 0)
                httpRead = 0;

            offset += httpRead;
        }
    }

    // Final state resolution
    lockContext(context);
    setRequestState(request, 0);

    if (hasRequestTimedOut(request)) {
        result = ORBIS_NP_WEBAPI_ERROR_TIMEOUT;
    } else if (isRequestAborted(request)) {
        result = ORBIS_NP_WEBAPI_ERROR_ABORTED;
    } else {
        result = offset;
        if (offset > 0) {
            s32 sc = 0;
            if (sceHttpGetStatusCode(getHttpRequestIdFromRequest(request), &sc) >= 0 && sc >= 400) {
                captureWebApiError(reinterpret_cast<const char*>(pData), offset);
            }
        }
    }

    unlockContext(context);

    // Cleanup
    clearRequestEndTime(request);
    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);

    return result;
}

using ServiceCb = PS4_SYSV_ABI void (*)(s32, s32, const char*, u32, const OrbisNpPeerAddress*,
                                        const OrbisNpPeerAddress*,
                                        const OrbisNpWebApiPushEventDataType*, const char*, u64,
                                        void*);
using BasicCb = PS4_SYSV_ABI void (*)(s32, s32, const OrbisNpPeerAddress*,
                                      const OrbisNpPeerAddress*,
                                      const OrbisNpWebApiPushEventDataType*, const char*, u64,
                                      void*);
using ExtdCbA = PS4_SYSV_ABI void (*)(s32, s32, const char*, u32, const OrbisNpPeerAddressA*,
                                      const OrbisNpOnlineId*, const OrbisNpPeerAddressA*,
                                      const OrbisNpOnlineId*, const OrbisNpWebApiPushEventDataType*,
                                      const char*, u64, const OrbisNpWebApiExtdPushEventExtdData*,
                                      u64, void*);

std::mutex g_push_mutex;
std::deque<PushEventInput> g_push_queue;

template <typename Filter>
bool filterMatches(const Filter* flt, const std::string& evServiceName, bool evHasServiceName,
                   const std::string& evDataType) {
    const bool catch_all = flt->npServiceName.empty() && flt->npServiceLabel == 0xffffffffu;
    if (catch_all) {
        if (evHasServiceName) {
            return false;
        }
    } else if (!evHasServiceName || flt->npServiceName != evServiceName) {
        return false;
    }
    for (const auto& p : flt->filterParams) {
        if (evDataType == p.data_type.val) {
            return true;
        }
    }
    return false;
}

// Network-thread hand-off only,the event is dispatched later from DrainPushEvents.
void EnqueuePushEvent(const PushEventInput& ev) {
    std::scoped_lock lk{g_push_mutex};
    g_push_queue.push_back(ev);
}

// Runs on the game thread from sceNpCheckCallback
void DrainPushEvents() {
    std::deque<PushEventInput> local;
    {
        std::scoped_lock lk{g_push_mutex};
        if (g_push_queue.empty()) {
            return;
        }
        local.swap(g_push_queue);
    }

    for (const PushEventInput& ev : local) {
        const bool ev_has_service = !ev.npServiceName.empty();
        OrbisNpWebApiPushEventDataType dt{};
        std::snprintf(dt.val, sizeof(dt.val), "%s", ev.dataType.c_str());
        const OrbisNpOnlineId* from_p = ev.hasFrom ? &ev.fromOnlineId : nullptr;
        const OrbisNpOnlineId* to_p = ev.hasTo ? &ev.toOnlineId : nullptr;

        std::scoped_lock gl{g_global_mutex};
        for (auto& [libId, context] : g_contexts) {
            if (context == nullptr) {
                continue;
            }
            std::scoped_lock cl{context->contextLock};
            for (auto& [ucKey, uc] : context->userContexts) {
                if (uc == nullptr) {
                    continue;
                }
                if (uc->userId != ev.targetUserId) {
                    continue;
                }
                const s32 title_user_ctx_id = ucKey;

                for (auto& [cbId, cb] : uc->extendedPushEventCallbacks) {
                    if (cb == nullptr) {
                        continue;
                    }
                    void (*raw)() = cb->cbFuncA  ? reinterpret_cast<void (*)()>(cb->cbFuncA)
                                    : cb->cbFunc ? reinterpret_cast<void (*)()>(cb->cbFunc)
                                                 : nullptr;
                    if (raw == nullptr) {
                        continue;
                    }
                    auto fit = context->extendedPushEventFilters.find(cb->filterId);
                    if (fit == context->extendedPushEventFilters.end()) {
                        continue;
                    }
                    const OrbisNpWebApiExtendedPushEventFilter* flt = fit->second;
                    if (!filterMatches(flt, ev.npServiceName, ev_has_service, ev.dataType)) {
                        continue;
                    }
                    std::vector<OrbisNpWebApiExtdPushEventExtdData> exarr;
                    exarr.reserve(ev.extdData.size());
                    for (auto& [k, v] : ev.extdData) {
                        OrbisNpWebApiExtdPushEventExtdData e{};
                        std::snprintf(e.extdDataKey.val, sizeof(e.extdDataKey.val), "%s",
                                      k.c_str());
                        e.pData = const_cast<char*>(v.data());
                        e.dataLen = v.size();
                        exarr.push_back(e);
                    }
                    const char* svc =
                        flt->npServiceName.empty() ? nullptr : flt->npServiceName.c_str();
                    const char* ext_data = ev.data.empty() ? nullptr : ev.data.data();
                    const OrbisNpWebApiExtdPushEventExtdData* ext_arr =
                        exarr.empty() ? nullptr : exarr.data();

                    reinterpret_cast<ExtdCbA>(raw)(
                        title_user_ctx_id, cbId, svc, flt->npServiceLabel, nullptr, to_p, nullptr,
                        from_p, &dt, ext_data, ev.data.size(), ext_arr, exarr.size(), cb->pUserArg);
                }

                // Service push
                for (auto& [cbId, cb] : uc->servicePushEventCallbacks) {
                    if (cb == nullptr || cb->cbFunc == nullptr) {
                        continue;
                    }
                    auto fit = context->servicePushEventFilters.find(cb->filterId);
                    if (fit == context->servicePushEventFilters.end()) {
                        continue;
                    }
                    const OrbisNpWebApiServicePushEventFilter* flt = fit->second;
                    if (!filterMatches(flt, ev.npServiceName, ev_has_service, ev.dataType)) {
                        continue;
                    }
                    const char* svc =
                        flt->npServiceName.empty() ? nullptr : flt->npServiceName.c_str();
                    const char* svc_data = ev.data.empty() ? nullptr : ev.data.data();
                    OrbisNpPeerAddress to_peer{};
                    OrbisNpPeerAddress from_peer{};
                    if (ev.hasTo) {
                        to_peer.online_id = ev.toOnlineId;
                    }
                    if (ev.hasFrom) {
                        from_peer.online_id = ev.fromOnlineId;
                    }
                    reinterpret_cast<ServiceCb>(reinterpret_cast<void (*)()>(cb->cbFunc))(
                        title_user_ctx_id, cbId, svc, flt->npServiceLabel, &to_peer, &from_peer,
                        &dt, svc_data, ev.data.size(), cb->pUserArg);
                }

                // Basic push
                if (!ev_has_service) {
                    for (auto& [cbId, cb] : uc->pushEventCallbacks) {
                        if (cb == nullptr || cb->cbFunc == nullptr) {
                            continue;
                        }
                        auto fit = context->pushEventFilters.find(cb->filterId);
                        if (fit == context->pushEventFilters.end()) {
                            continue;
                        }
                        const OrbisNpWebApiPushEventFilter* flt = fit->second;
                        bool matched = false;
                        for (const auto& p : flt->filterParams) {
                            if (ev.dataType == p.data_type.val) {
                                matched = true;
                                break;
                            }
                        }
                        if (!matched) {
                            continue;
                        }
                        OrbisNpPeerAddress to_peer{};
                        OrbisNpPeerAddress from_peer{};
                        if (ev.hasTo) {
                            to_peer.online_id = ev.toOnlineId;
                        }
                        if (ev.hasFrom) {
                            from_peer.online_id = ev.fromOnlineId;
                        }
                        const char* p_data = ev.data.empty() ? nullptr : ev.data.data();
                        reinterpret_cast<BasicCb>(reinterpret_cast<void (*)()>(cb->cbFunc))(
                            title_user_ctx_id, cbId, &to_peer, &from_peer, &dt, p_data,
                            ev.data.size(), cb->pUserArg);
                    }
                }
            }
        }
    }
}
}; // namespace Libraries::Np::NpWebApi
