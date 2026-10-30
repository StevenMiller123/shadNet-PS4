// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <string_view>
#include <magic_enum/magic_enum.hpp>
#include <orbis/Http.h>
#include <orbis/UserService.h>
#include <orbis/libkernel.h>

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/np/np_web_api/np_web_api_internal.h"
#include "shadnet/config.h"

namespace Libraries::Np::NpWebApi {

static Common::RecursiveMutex g_global_mutex;
static std::map<s32, OrbisNpWebApiContext*> g_contexts;
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

s32 createLibraryContext(s32 lib_http_ctx_id, u64 pool_size, const char* name, s32 type) {
    std::scoped_lock lk{g_global_mutex};

    g_library_context_count++;
    if (g_library_context_count >= 0x8000) {
        g_library_context_count = 1;
    }
    s32 ctx_id = g_library_context_count;
    while (g_contexts.contains(ctx_id)) {
        ctx_id--;
    }
    if (ctx_id <= 0) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_MAX;
    }

    // Create new context
    g_contexts[ctx_id] = new OrbisNpWebApiContext{};
    auto& new_context = g_contexts.at(ctx_id);
    new_context->lib_ctx_id = ctx_id;
    new_context->lib_http_ctx_id = lib_http_ctx_id;
    new_context->type = type;
    new_context->user_count = 0;
    new_context->terminated = false;
    if (name != nullptr) {
        new_context->name = std::string(name);
    }

    return ctx_id;
}

OrbisNpWebApiContext* findAndValidateContext(s32 lib_ctx_id, s32 flag) {
    std::scoped_lock lk{g_global_mutex};
    if (lib_ctx_id < 1 || lib_ctx_id >= 0x8000 || !g_contexts.contains(lib_ctx_id)) {
        return nullptr;
    }
    auto& context = g_contexts[lib_ctx_id];
    std::scoped_lock lk2{context->context_lock};
    if (flag == 0 && context->terminated) {
        return nullptr;
    }
    context->user_count++;
    return context;
}

void releaseContext(OrbisNpWebApiContext* context) {
    std::scoped_lock lk{context->context_lock};
    context->user_count--;
}

bool isContextTerminated(OrbisNpWebApiContext* context) {
    std::scoped_lock lk{context->context_lock};
    return context->terminated;
}

bool isContextBusy(OrbisNpWebApiContext* context) {
    std::scoped_lock lk{context->context_lock};
    return context->user_count > 1;
}

bool areContextHandlesBusy(OrbisNpWebApiContext* context) {
    std::scoped_lock lk{context->context_lock};
    for (auto& handle : context->handles) {
        if (handle.second->user_count > 0) {
            return true;
        }
    }
    return false;
}

void lockContext(OrbisNpWebApiContext* context) {
    context->context_lock.lock();
}

void unlockContext(OrbisNpWebApiContext* context) {
    context->context_lock.unlock();
}

void markContextAsTerminated(OrbisNpWebApiContext* context) {
    std::scoped_lock lk{context->context_lock};
    context->terminated = true;
}

void checkContextTimeout(OrbisNpWebApiContext* context) {
    u64 time = sceKernelGetProcessTime();
    std::scoped_lock lk{context->context_lock};

    for (auto& user_context : context->user_contexts) {
        checkUserContextTimeout(user_context.second);
    }

    for (auto& value : context->timer_handles) {
        auto& timer_handle = value.second;
        if (!timer_handle->timed_out && timer_handle->handle_timeout != 0 &&
            timer_handle->handle_end_time < time) {
            timer_handle->timed_out = true;
            abortHandle(context->lib_ctx_id, timer_handle->handle_id);
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

s32 deleteContext(s32 lib_ctx_id) {
    std::scoped_lock lk{g_global_mutex};
    if (!g_contexts.contains(lib_ctx_id)) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    auto& context = g_contexts[lib_ctx_id];
    context->handles.clear();
    context->timer_handles.clear();
    context->push_event_filters.clear();
    context->service_push_event_filters.clear();
    context->extended_push_event_filters.clear();

    g_contexts.erase(lib_ctx_id);
    return ORBIS_OK;
}

s32 terminateContext(s32 lib_ctx_id) {
    OrbisNpWebApiContext* ctx = findAndValidateContext(lib_ctx_id);
    if (ctx == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    if (g_sdk_ver < Common::ElfInfo::FW_400 && isContextBusy(ctx)) {
        releaseContext(ctx);
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_BUSY;
    }

    std::vector<s32> user_context_ids;
    for (auto& user_context : ctx->user_contexts) {
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
            abortHandle(lib_ctx_id, handle.first);
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
    return deleteContext(lib_ctx_id);
}

OrbisNpWebApiUserContext* findUserContextByUserId(OrbisNpWebApiContext* context, s32 user_id) {
    if (user_id == ORBIS_USER_SERVICE_USER_ID_INVALID) {
        return nullptr;
    }

    std::scoped_lock lk{context->context_lock};
    for (auto& user_context : context->user_contexts) {
        if (user_context.second->user_id == user_id) {
            OrbisNpWebApiUserContext* user_ctx = user_context.second;
            user_ctx->user_count++;
            return user_ctx;
        }
    }
    return nullptr;
}

OrbisNpWebApiUserContext* findUserContext(OrbisNpWebApiContext* context, s32 title_user_ctx_id) {
    std::scoped_lock lk{context->context_lock};
    if (!context->user_contexts.contains(title_user_ctx_id)) {
        return nullptr;
    }
    OrbisNpWebApiUserContext* user_context = context->user_contexts[title_user_ctx_id];
    if (user_context->deleted) {
        return nullptr;
    }
    user_context->user_count++;
    return user_context;
}

s32 createUserContextWithOnlineId(s32 lib_ctx_id, OrbisNpOnlineId* online_id) {
    LOG_WARNING(Lib_NpWebApi, "called lib_ctx_id = {}", lib_ctx_id);

    s32 user_id = 0;
    sceUserServiceGetInitialUser(&user_id);
    return createUserContext(lib_ctx_id, user_id);
}

s32 createUserContext(s32 lib_ctx_id, s32 user_id) {
    LOG_INFO(Lib_NpWebApi, "lib_ctx_id = {}, user_id = {}", lib_ctx_id, user_id);
    OrbisNpWebApiContext* context = findAndValidateContext(lib_ctx_id);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContextByUserId(context, user_id);
    if (user_context != nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_ALREADY_EXIST;
    }

    std::scoped_lock lk{context->context_lock};

    // Create new user context
    g_user_context_count++;
    if (g_user_context_count >= 0x10000) {
        g_user_context_count = 1;
    }
    s32 user_ctx_id = (lib_ctx_id << 0x10) | g_user_context_count;
    while (context->user_contexts.contains(user_ctx_id)) {
        user_ctx_id--;
    }
    if (user_ctx_id <= (lib_ctx_id << 0x10)) {
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_MAX;
    }

    context->user_contexts[user_ctx_id] = new OrbisNpWebApiUserContext{};
    user_context = context->user_contexts.at(user_ctx_id);
    user_context->user_count = 0;
    user_context->parent_context = context;
    user_context->user_id = user_id;
    user_context->user_ctx_id = user_ctx_id;
    user_context->deleted = false;

    // TODO: Internal structs related to libSceHttp use are initialized here.
    releaseContext(context);
    return user_ctx_id;
}

s32 registerNotificationCallback(s32 title_user_ctx_id, OrbisNpWebApiNotificationCallback cb_func,
                                 void* user_arg) {
    OrbisNpWebApiContext* context = findAndValidateContext(title_user_ctx_id >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, title_user_ctx_id);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    lockContext(context);
    user_context->notification_callback_function = cb_func;
    user_context->notification_callback_user_args = user_arg;
    unlockContext(context);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 unregisterNotificationCallback(s32 title_user_ctx_id) {
    OrbisNpWebApiContext* context = findAndValidateContext(title_user_ctx_id >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, title_user_ctx_id);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    lockContext(context);
    user_context->notification_callback_function = nullptr;
    user_context->notification_callback_user_args = nullptr;
    unlockContext(context);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

bool isUserContextBusy(OrbisNpWebApiUserContext* user_context) {
    std::scoped_lock lk{user_context->parent_context->context_lock};
    return user_context->user_count > 1;
}

bool areUserContextRequestsBusy(OrbisNpWebApiUserContext* user_context) {
    std::scoped_lock lk{user_context->parent_context->context_lock};
    for (auto& request : user_context->requests) {
        request.second->user_count++;
        bool req_busy = isRequestBusy(request.second);
        request.second->user_count--;
        if (req_busy) {
            return true;
        }
    }
    return false;
}

void releaseUserContext(OrbisNpWebApiUserContext* user_context) {
    std::scoped_lock lk{user_context->parent_context->context_lock};
    user_context->user_count--;
}

void checkUserContextTimeout(OrbisNpWebApiUserContext* user_context) {
    std::scoped_lock lk{user_context->parent_context->context_lock};
    for (auto& request : user_context->requests) {
        checkRequestTimeout(request.second);
    }
}

s32 deleteUserContext(s32 title_user_ctx_id) {
    OrbisNpWebApiContext* context = findAndValidateContext(title_user_ctx_id >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    lockContext(context);
    OrbisNpWebApiUserContext* user_context = findUserContext(context, title_user_ctx_id);
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

    user_context->extended_push_event_callbacks.clear();
    user_context->service_push_event_callbacks.clear();
    user_context->push_event_callbacks.clear();
    user_context->requests.clear();
    context->user_contexts.erase(title_user_ctx_id);

    unlockContext(context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 createRequest(s32 title_user_ctx_id, const char* api_group, const char* path,
                  OrbisNpWebApiHttpMethod method,
                  const OrbisNpWebApiContentParameter* content_parameter,
                  const OrbisNpWebApiIntCreateRequestExtraArgs* internal_args, s64* out_request_id,
                  bool is_multipart) {
    OrbisNpWebApiContext* context = findAndValidateContext(title_user_ctx_id >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, title_user_ctx_id);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    lockContext(user_context->parent_context);
    if (g_sdk_ver >= Common::ElfInfo::FW_400 && user_context->deleted) {
        unlockContext(user_context->parent_context);
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    g_request_count++;
    if (g_request_count >> 0x20 != 0) {
        g_request_count = 1;
    }

    s64 user_ctx_id = static_cast<s64>(title_user_ctx_id);
    s64 request_id = (user_ctx_id << 0x20) | g_request_count;
    while (user_context->requests.contains(request_id)) {
        request_id--;
    }
    user_context->requests[request_id] = new OrbisNpWebApiRequest{};

    auto& request = user_context->requests[request_id];
    request->parent_context = context;
    request->user_count = 0;
    request->request_id = request_id;
    request->user_method = method;
    request->multipart = is_multipart;
    request->aborted = false;

    if (api_group != nullptr) {
        request->user_api_group = std::string(api_group);
    }

    if (path != nullptr) {
        request->user_path = std::string(path);
    }

    if (content_parameter != nullptr) {
        request->user_content_length = content_parameter->content_length;
        if (content_parameter->content_type != nullptr) {
            request->user_content_type = std::string(content_parameter->content_type);
        }
    }

    unlockContext(user_context->parent_context);

    if (out_request_id != nullptr) {
        *out_request_id = request->request_id;
    }

    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

OrbisNpWebApiRequest* findRequest(OrbisNpWebApiUserContext* user_context, s64 request_id) {
    std::scoped_lock lk{user_context->parent_context->context_lock};
    if (user_context->requests.contains(request_id)) {
        return user_context->requests[request_id];
    }

    return nullptr;
}

OrbisNpWebApiRequest* findRequestAndMarkBusy(OrbisNpWebApiUserContext* user_context,
                                             s64 request_id) {
    std::scoped_lock lk{user_context->parent_context->context_lock};
    if (user_context->requests.contains(request_id)) {
        auto& request = user_context->requests[request_id];
        request->user_count++;
        return request;
    }
    return nullptr;
}

bool isRequestBusy(OrbisNpWebApiRequest* request) {
    std::scoped_lock lk{request->parent_context->context_lock};
    return request->user_count > 1;
}

s32 setRequestTimeout(s64 request_id, u32 timeout) {
    OrbisNpWebApiContext* context = findAndValidateContext(request_id >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, request_id >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiRequest* request = findRequestAndMarkBusy(user_context, request_id);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }

    request->request_timeout = timeout;

    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

void startRequestTimer(OrbisNpWebApiRequest* request) {
    if (request->request_timeout != 0 && request->request_end_time == 0) {
        request->request_end_time = sceKernelGetProcessTime() + request->request_timeout;
    }
}

void checkRequestTimeout(OrbisNpWebApiRequest* request) {
    u64 time = sceKernelGetProcessTime();
    if (!request->timed_out && request->request_end_time != 0 && request->request_end_time < time) {
        request->timed_out = true;
        abortRequest(request->request_id);
    }
}

s32 setMultipartContentType(s64 request_id, const char* type_name, const char* boundary) {
    OrbisNpWebApiContext* context = findAndValidateContext(request_id >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }
    OrbisNpWebApiUserContext* user_context = findUserContext(context, request_id >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }
    OrbisNpWebApiRequest* request = findRequest(user_context, request_id);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }
    request->multipart_content_type = (type_name != nullptr) ? type_name : "";
    request->multipart_boundary = (boundary != nullptr) ? boundary : "";
    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 addMultipartPart(s64 request_id, const OrbisNpWebApiMultipartPartParameter* param, s32* index) {
    if (param == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    OrbisNpWebApiContext* context = findAndValidateContext(request_id >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }
    OrbisNpWebApiUserContext* user_context = findUserContext(context, request_id >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }
    OrbisNpWebApiRequest* request = findRequest(user_context, request_id);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }

    const auto is_content_length = [](const char* name) {
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
    part.content_length = param->content_length;
    std::string headers;
    for (u64 i = 0; i < param->header_num; ++i) {
        const OrbisNpWebApiHttpHeader& h = param->headers[i];
        if (h.name == nullptr || h.value == nullptr || is_content_length(h.name)) {
            continue;
        }
        headers += h.name;
        headers += ": ";
        headers += h.value;
        headers += "\r\n";
    }
    headers += "Content-Length: ";
    headers += std::to_string(param->content_length);
    headers += "\r\n\r\n";
    part.raw_headers = std::move(headers);

    request->multipart_parts.push_back(std::move(part));
    if (index != nullptr) {
        *index = static_cast<s32>(request->multipart_parts.size()); // 1-based part index
    }
    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 sendRequest(s64 request_id, s32 part_index, const void* data, u64 data_size, s8 flag,
                OrbisNpWebApiResponseInformationOption* resp_info_option) {
    OrbisNpWebApiContext* context = findAndValidateContext(request_id >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, request_id >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiRequest* request = findRequestAndMarkBusy(user_context, request_id);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }

    startRequestTimer(request);

    std::string multipart_body;
    const void* send_data = data;
    u64 send_size = data_size;
    if (request->multipart) {
        if (part_index < 1 || part_index > static_cast<s32>(request->multipart_parts.size())) {
            releaseRequest(request);
            releaseUserContext(user_context);
            releaseContext(context);
            return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
        }
        auto& part = request->multipart_parts[part_index - 1];
        if (data != nullptr && data_size > 0) {
            part.data.append(static_cast<const char*>(data), data_size);
        }
        part.sent_size += data_size;

        bool all_complete = true;
        for (const auto& p : request->multipart_parts) {
            if (p.sent_size < p.content_length) {
                all_complete = false;
                break;
            }
        }
        if (!all_complete) {
            releaseRequest(request);
            releaseUserContext(user_context);
            releaseContext(context);
            return ORBIS_OK;
        }

        std::string boundary = request->multipart_boundary;
        if (boundary.empty()) {
            static const char kHex[] = "0123456789abcdef";
            boundary = "----shadPS4Boundary";
            const u64 v = static_cast<u64>(request_id);
            for (s32 shift = 60; shift >= 0; shift -= 4) {
                boundary += kHex[(v >> shift) & 0xf];
            }
        }
        const std::string type_name = request->multipart_content_type.empty()
                                          ? std::string("multipart/mixed")
                                          : request->multipart_content_type;
        for (const auto& p : request->multipart_parts) {
            multipart_body += "--";
            multipart_body += boundary;
            multipart_body += "\r\n";
            multipart_body += p.raw_headers;
            multipart_body += p.data;
            multipart_body += "\r\n";
        }
        multipart_body += "--";
        multipart_body += boundary;
        multipart_body += "--\r\n";

        request->user_content_type = type_name + "; boundary=" + boundary;
        request->user_content_length = multipart_body.size();
        send_data = multipart_body.data();
        send_size = multipart_body.size();
    }

    if (g_sdk_ver >= Common::ElfInfo::FW_250 && !request->sent) {
        request->sent = true;
    }

    lockContext(context);
    if (!request->timed_out && request->aborted) {
        unlockContext(context);
        releaseRequest(request);
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_ABORTED;
    }

    unlockContext(context);

    // Stubbing sceNpManagerIntGetSigninState call with a config check.
    auto& np_handler = NpHandler::Instance();
    if (!np_handler.IsSignedIn(user_context->user_id)) {
        releaseRequest(request);
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
    }

    auto& config = ShadNet::Settings::GetInstance();
    if (request->http_request_id == 0) {
        std::string base_url = config.GetWebApiServerUrl();
        const s32 tmpl_id = sceHttpCreateTemplate(context->lib_http_ctx_id, "libhttp", 2, 0);
        if (tmpl_id < 0) {
            LOG_ERROR(Lib_NpWebApi, "sceHttpCreateTemplate failed: {:#x}", tmpl_id);
            releaseRequest(request);
            releaseUserContext(user_context);
            releaseContext(context);
            return tmpl_id;
        }
        request->http_template_id = tmpl_id;
        const s32 conn_id = sceHttpCreateConnectionWithURL(tmpl_id, base_url.c_str(), true);
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
        s32 sce_method;
        switch (request->user_method) {
        case ORBIS_NP_WEBAPI_HTTP_METHOD_GET:
            sce_method = 0;
            break;
        case ORBIS_NP_WEBAPI_HTTP_METHOD_POST:
            sce_method = 1;
            break;
        case ORBIS_NP_WEBAPI_HTTP_METHOD_PUT:
            sce_method = 4;
            break;
        case ORBIS_NP_WEBAPI_HTTP_METHOD_DELETE:
            sce_method = 5;
            break;
        case ORBIS_NP_WEBAPI_HTTP_METHOD_PATCH:
            sce_method = 8;
            break;
        default:
            LOG_ERROR(Lib_NpWebApi, "invalid method {}", static_cast<s32>(request->user_method));
            releaseRequest(request);
            releaseUserContext(user_context);
            releaseContext(context);
            return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
        }
        const std::string full_url = base_url + request->user_path;
        const s32 req_id = sceHttpCreateRequestWithURL(conn_id, sce_method, full_url.c_str(),
                                                       request->user_content_length);
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

        if (!request->user_content_type.empty()) {
            sceHttpAddRequestHeader(req_id, "Content-Type", request->user_content_type.c_str(), 0);
        }

        const std::string bearer = np_handler.GetBearerToken(user_context->user_id);
        if (!bearer.empty()) {
            const std::string auth_value = "Bearer " + bearer;
            sceHttpAddRequestHeader(req_id, "Authorization", auth_value.c_str(), 0);
        } else {
            LOG_WARNING(Lib_NpWebApi,
                        "no bearer token for user_id={}; request to '{}' will "
                        "be unauthenticated",
                        user_context->user_id, request->user_path);
        }

        const auto is_content_type = [](const std::string& name) {
            constexpr std::string_view target = "content-type";
            if (name.size() != target.size()) {
                return false;
            }
            for (u64 i = 0; i < name.size(); ++i) {
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
        const bool have_content_type = !request->user_content_type.empty();
        for (const auto& [hname, hvalue] : request->user_headers) {
            if (have_content_type && is_content_type(hname)) {
                continue;
            }
            sceHttpAddRequestHeader(req_id, hname.c_str(), hvalue.c_str(), 0);
        }
    }

    setRequestState(request, 4);

    const s32 send_err = sceHttpSendRequest(request->http_request_id, send_data, send_size);
    if (send_err < 0) {
        LOG_ERROR(Lib_NpWebApi, "sceHttpSendRequest failed: {:#x}", send_err);
        releaseRequest(request);
        releaseUserContext(user_context);
        releaseContext(context);
        return send_err;
    }

    LOG_INFO(Lib_NpWebApi, "request_id={:#x} api_group='{}' path='{}' method={} http_req_id={}",
             request_id, request->user_api_group, request->user_path,
             magic_enum::enum_name(request->user_method), request->http_request_id);

    s32 send_result = ORBIS_OK;
    if (flag != 0) {
        s32 status = 0;
        if (sceHttpGetStatusCode(request->http_request_id, &status) >= 0) {
            if (resp_info_option != nullptr) {
                resp_info_option->http_status = status;
            }
            if (status >= 400) {
                std::string err_body;
                char buf[256];
                for (;;) {
                    const s32 n = sceHttpReadData(request->http_request_id, buf, sizeof(buf));
                    if (n <= 0)
                        break;
                    err_body.append(buf, static_cast<u64>(n));
                    if (err_body.size() > 64u * 1024u)
                        break; // sanity cap on a runaway error body
                }
                if (resp_info_option != nullptr) {
                    resp_info_option->response_data_size = err_body.size();
                    char* const dst = reinterpret_cast<char* const>(resp_info_option->error_object);
                    const u64 cap = resp_info_option->error_object_size;
                    if (dst != nullptr && cap > 0) {
                        const u64 n = std::min<u64>(err_body.size(), cap - 1);
                        if (n > 0)
                            std::memcpy(dst, err_body.data(), n);
                        dst[n] = '\0';
                    }
                }
                const s32 np_err = captureWebApiError(err_body.data(), err_body.size());
                if (np_err > 0 && np_err < 0xc00000) {
                    send_result = static_cast<s32>(0x82000000u | static_cast<u32>(np_err));
                } else if (status >= 100 && status < 600) {
                    send_result = static_cast<s32>(0x82f00000u | static_cast<u32>(status));
                } else {
                    send_result = static_cast<s32>(0x82ffffffu);
                }
            }
        }
    }

    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return send_result;
}

s32 abortRequestInternal(OrbisNpWebApiContext* context, OrbisNpWebApiUserContext* user_context,
                         OrbisNpWebApiRequest* request) {
    if (context == nullptr || user_context == nullptr || request == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{context->context_lock};
    if (request->aborted) {
        return ORBIS_OK;
    }

    request->aborted = true;
    if (request->http_request_id) {
        sceHttpAbortRequest(request->http_request_id);
    }
    return ORBIS_OK;
}

s32 abortRequest(s64 request_id) {
    OrbisNpWebApiContext* context = findAndValidateContext(request_id >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, request_id >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiRequest* request = findRequest(user_context, request_id);
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
    std::scoped_lock lk{request->parent_context->context_lock};
    request->user_count--;
}

s32 deleteRequest(s64 request_id) {
    OrbisNpWebApiContext* context = findAndValidateContext(request_id >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    lockContext(context);
    OrbisNpWebApiUserContext* user_context = findUserContext(context, request_id >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiRequest* request = findRequestAndMarkBusy(user_context, request_id);
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
    user_context->requests.erase(request->request_id);

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

    std::scoped_lock lk{context->context_lock};

    s32 handle_id = g_handle_count;
    context->handles[handle_id] = new OrbisNpWebApiHandle{};
    auto& handle = context->handles[handle_id];
    handle->handle_id = handle_id;
    handle->user_count = 0;
    handle->aborted = false;
    handle->deleted = false;

    if (g_sdk_ver >= Common::ElfInfo::FW_300) {
        context->timer_handles[handle_id] = new OrbisNpWebApiTimerHandle{};
        auto& timer_handle = context->timer_handles[handle_id];
        timer_handle->handle_id = handle_id;
        timer_handle->timed_out = false;
        timer_handle->handle_timeout = 0;
        timer_handle->handle_end_time = 0;
    }

    return handle_id;
}

s32 createHandle(s32 lib_ctx_id) {
    OrbisNpWebApiContext* context = findAndValidateContext(lib_ctx_id);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = createHandleInternal(context);
    releaseContext(context);
    return result;
}

s32 setHandleTimeoutInternal(OrbisNpWebApiContext* context, s32 handle_id, u32 timeout) {
    std::scoped_lock lk{context->context_lock};
    if (!context->timer_handles.contains(handle_id)) {
        return ORBIS_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND;
    }
    auto& handle = context->handles[handle_id];
    handle->user_count++;

    auto& timer_handle = context->timer_handles[handle_id];
    timer_handle->handle_timeout = timeout;

    handle->user_count--;
    return ORBIS_OK;
}

s32 setHandleTimeout(s32 lib_ctx_id, s32 handle_id, u32 timeout) {
    OrbisNpWebApiContext* context = findAndValidateContext(lib_ctx_id);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = setHandleTimeoutInternal(context, handle_id, timeout);
    releaseContext(context);
    return result;
}

void startHandleTimer(OrbisNpWebApiContext* context, s32 handle_id) {
    std::scoped_lock lk{context->context_lock};
    if (!context->timer_handles.contains(handle_id)) {
        return;
    }
    auto& timer_handle = context->timer_handles[handle_id];
    if (timer_handle->handle_timeout == 0) {
        return;
    }
    timer_handle->handle_end_time = sceKernelGetProcessTime() + timer_handle->handle_timeout;
}

void releaseHandle(OrbisNpWebApiContext* context, OrbisNpWebApiHandle* handle) {
    if (handle != nullptr) {
        std::scoped_lock lk{context->context_lock};
        handle->user_count--;
    }
}

s32 getHandle(OrbisNpWebApiContext* context, s32 handle_id, OrbisNpWebApiHandle** handle_out) {
    std::scoped_lock lk{context->context_lock};
    if (!context->handles.contains(handle_id)) {
        return ORBIS_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND;
    }
    auto& handle = context->handles[handle_id];
    handle->user_count++;
    if (handle_out != nullptr) {
        *handle_out = handle;
    }
    return ORBIS_OK;
}

s32 abortHandle(s32 lib_ctx_id, s32 handle_id) {
    OrbisNpWebApiContext* context = findAndValidateContext(lib_ctx_id);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiHandle* handle;
    s32 result = getHandle(context, handle_id, &handle);
    if (result == ORBIS_OK) {
        std::scoped_lock lk{context->context_lock};
        handle->aborted = true;
        // TODO: sceNpAsmClientAbortRequest call
        releaseHandle(context, handle);
    }

    releaseContext(context);
    return result;
}

s32 deleteHandleInternal(OrbisNpWebApiContext* context, s32 handle_id) {
    lockContext(context);
    if (!context->handles.contains(handle_id)) {
        unlockContext(context);
        return ORBIS_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND;
    }

    auto& handle = context->handles[handle_id];
    if (g_sdk_ver >= Common::ElfInfo::FW_400) {
        if (handle->deleted) {
            unlockContext(context);
            return ORBIS_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND;
        }
        handle->deleted = true;
        unlockContext(context);
        abortHandle(context->lib_ctx_id, handle_id);
        lockContext(context);
        handle->user_count++;
        while (handle->user_count > 1) {
            handle->user_count--;
            unlockContext(context);
            sceKernelUsleep(50000);
            lockContext(context);
            handle->user_count++;
        }
        handle->user_count--;
    } else if (handle->user_count > 0) {
        unlockContext(context);
        return ORBIS_NP_WEBAPI_ERROR_HANDLE_BUSY;
    }

    context->handles.erase(handle_id);

    if (g_sdk_ver >= Common::ElfInfo::FW_300 && context->timer_handles.contains(handle_id)) {
        context->timer_handles.erase(handle_id);
    }

    unlockContext(context);
    return ORBIS_OK;
}

s32 deleteHandle(s32 lib_ctx_id, s32 handle_id) {
    OrbisNpWebApiContext* context = findAndValidateContext(lib_ctx_id);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = deleteHandleInternal(context, handle_id);
    releaseContext(context);
    return result;
}

s32 createPushEventFilterInternal(OrbisNpWebApiContext* context,
                                  const OrbisNpWebApiPushEventFilterParameter* filter_param,
                                  u64 filter_param_num) {
    std::scoped_lock lk{context->context_lock};
    g_push_event_filter_count++;
    if (g_push_event_filter_count >= 0xf0000000) {
        g_push_event_filter_count = 1;
    }
    s32 filter_id = g_push_event_filter_count;

    context->push_event_filters[filter_id] = new OrbisNpWebApiPushEventFilter{};
    auto& filter = context->push_event_filters[filter_id];
    filter->parent_context = context;
    filter->filter_id = filter_id;

    LOG_INFO(Lib_NpWebApi, "filter_id={} data_type_params={}", filter_id, filter_param_num);
    if (filter_param != nullptr && filter_param_num != 0) {
        for (u64 param_idx = 0; param_idx < filter_param_num; param_idx++) {
            OrbisNpWebApiPushEventFilterParameter copy = OrbisNpWebApiPushEventFilterParameter{};
            std::memcpy(&copy, &filter_param[param_idx],
                        sizeof(OrbisNpWebApiPushEventFilterParameter));
            LOG_INFO(Lib_NpWebApi, "  filter_param[{}] data_type='{}'", param_idx,
                     copy.data_type.val);
            filter->filter_params.emplace_back(copy);
        }
    }
    return filter_id;
}

s32 createPushEventFilter(s32 lib_ctx_id, const OrbisNpWebApiPushEventFilterParameter* filter_param,
                          u64 filter_param_num) {
    OrbisNpWebApiContext* context = findAndValidateContext(lib_ctx_id);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = createPushEventFilterInternal(context, filter_param, filter_param_num);
    releaseContext(context);
    return result;
}

s32 deletePushEventFilterInternal(OrbisNpWebApiContext* context, s32 filter_id) {
    std::scoped_lock lk{context->context_lock};
    if (!context->push_event_filters.contains(filter_id)) {
        return ORBIS_NP_WEBAPI_ERROR_PUSH_EVENT_FILTER_NOT_FOUND;
    }

    context->push_event_filters[filter_id]->filter_params.clear();
    context->push_event_filters.erase(filter_id);
    return ORBIS_OK;
}

s32 deletePushEventFilter(s32 lib_ctx_id, s32 filter_id) {
    OrbisNpWebApiContext* context = findAndValidateContext(lib_ctx_id);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = deletePushEventFilterInternal(context, filter_id);
    releaseContext(context);
    return result;
}

s32 registerPushEventCallbackInternal(OrbisNpWebApiUserContext* user_context, s32 filter_id,
                                      OrbisNpWebApiPushEventCallback cb_func, void* user_arg) {
    std::scoped_lock lk{user_context->parent_context->context_lock};
    g_registered_callback_count++;
    if (g_registered_callback_count >= 0xf0000000) {
        g_registered_callback_count = 1;
    }
    s32 cb_id = g_registered_callback_count;

    user_context->push_event_callbacks[cb_id] = new OrbisNpWebApiRegisteredPushEventCallback{};
    auto& cb = user_context->push_event_callbacks[cb_id];
    cb->callback_id = cb_id;
    cb->filter_id = filter_id;
    cb->cb_func = cb_func;
    cb->user_arg = user_arg;
    return cb_id;
}

s32 registerPushEventCallback(s32 title_user_ctx_id, s32 filter_id,
                              OrbisNpWebApiPushEventCallback cb_func, void* user_arg) {
    OrbisNpWebApiContext* context = findAndValidateContext(title_user_ctx_id >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, title_user_ctx_id);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (g_sdk_ver >= Common::ElfInfo::FW_250 && !context->push_event_filters.contains(filter_id)) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_PUSH_EVENT_FILTER_NOT_FOUND;
    }

    s32 result = registerPushEventCallbackInternal(user_context, filter_id, cb_func, user_arg);
    releaseUserContext(user_context);
    releaseContext(context);
    return result;
}

s32 unregisterPushEventCallback(s32 title_user_ctx_id, s32 callback_id) {
    OrbisNpWebApiContext* context = findAndValidateContext(title_user_ctx_id >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, title_user_ctx_id);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (!user_context->push_event_callbacks.contains(callback_id)) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_PUSH_EVENT_CALLBACK_NOT_FOUND;
    }

    lockContext(context);
    user_context->push_event_callbacks.erase(callback_id);
    unlockContext(context);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 createServicePushEventFilterInternal(
    OrbisNpWebApiContext* context, s32 handle_id, const char* np_service_name, u32 np_service_label,
    const OrbisNpWebApiServicePushEventFilterParameter* filter_param, u64 filter_param_num) {
    std::scoped_lock lk{context->context_lock};
    if (!context->handles.contains(handle_id)) {
        return ORBIS_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND;
    }
    auto& handle = context->handles[handle_id];
    handle->user_count++;

    auto& np_handler = NpHandler::Instance();
    if (np_service_name != nullptr && !np_handler.IsActive()) {
        // Seems sceNpManagerIntGetUserList fails?
        LOG_DEBUG(Lib_NpWebApi, "Cannot create service push event while shadNet is disabled");
        handle->user_count--;
        return ORBIS_NP_WEBAPI_ERROR_SIGNED_IN_USER_NOT_FOUND;
    }

    g_service_push_event_filter_count++;
    if (g_service_push_event_filter_count >= 0xf0000000) {
        g_service_push_event_filter_count = 1;
    }
    s32 filter_id = g_service_push_event_filter_count;

    context->service_push_event_filters[filter_id] = new OrbisNpWebApiServicePushEventFilter{};
    auto& filter = context->service_push_event_filters[filter_id];
    filter->parent_context = context;
    filter->filter_id = filter_id;

    if (np_service_name == nullptr) {
        filter->internal = true;
    } else {
        filter->np_service_name = std::string(np_service_name);
    }

    filter->np_service_label = np_service_label;
    LOG_INFO(Lib_NpWebApi, "filter_id={} data_type_params={}", filter_id, filter_param_num);
    if (filter_param != nullptr && filter_param_num != 0) {
        for (u64 param_idx = 0; param_idx < filter_param_num; param_idx++) {
            OrbisNpWebApiServicePushEventFilterParameter copy =
                OrbisNpWebApiServicePushEventFilterParameter{};
            std::memcpy(&copy, &filter_param[param_idx],
                        sizeof(OrbisNpWebApiServicePushEventFilterParameter));
            LOG_INFO(Lib_NpWebApi, "  filter_param[{}] data_type='{}'", param_idx,
                     copy.data_type.val);
            filter->filter_params.emplace_back(copy);
        }
    }

    handle->user_count--;
    return filter_id;
}

s32 createServicePushEventFilter(s32 lib_ctx_id, s32 handle_id, const char* np_service_name,
                                 u32 np_service_label,
                                 const OrbisNpWebApiServicePushEventFilterParameter* filter_param,
                                 u64 filter_param_num) {
    OrbisNpWebApiContext* context = findAndValidateContext(lib_ctx_id);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    startHandleTimer(context, handle_id);
    s32 result = createServicePushEventFilterInternal(
        context, handle_id, np_service_name, np_service_label, filter_param, filter_param_num);
    releaseContext(context);
    return result;
}

s32 deleteServicePushEventFilterInternal(OrbisNpWebApiContext* context, s32 filter_id) {
    std::scoped_lock lk{context->context_lock};
    if (!context->service_push_event_filters.contains(filter_id)) {
        return ORBIS_NP_WEBAPI_ERROR_SERVICE_PUSH_EVENT_FILTER_NOT_FOUND;
    }

    context->service_push_event_filters[filter_id]->filter_params.clear();
    context->service_push_event_filters.erase(filter_id);
    return ORBIS_OK;
}

s32 deleteServicePushEventFilter(s32 lib_ctx_id, s32 filter_id) {
    OrbisNpWebApiContext* context = findAndValidateContext(lib_ctx_id);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = deleteServicePushEventFilterInternal(context, filter_id);
    releaseContext(context);
    return result;
}

s32 registerServicePushEventCallbackInternal(
    OrbisNpWebApiUserContext* user_context, s32 filter_id,
    OrbisNpWebApiServicePushEventCallback cb_func,
    OrbisNpWebApiInternalServicePushEventCallback int_cb_func,
    OrbisNpWebApiInternalServicePushEventCallbackA int_cb_func_a, void* user_arg) {
    std::scoped_lock lk{user_context->parent_context->context_lock};
    if (cb_func == nullptr && int_cb_func == nullptr && int_cb_func_a == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    g_registered_callback_count++;
    if (g_registered_callback_count >= 0xf0000000) {
        g_registered_callback_count = 1;
    }
    s32 cb_id = g_registered_callback_count;

    user_context->service_push_event_callbacks[cb_id] =
        new OrbisNpWebApiRegisteredServicePushEventCallback{};
    auto& cb = user_context->service_push_event_callbacks[cb_id];
    cb->callback_id = cb_id;
    cb->filter_id = filter_id;
    cb->cb_func = cb_func;
    cb->internal_cb_func = int_cb_func;
    cb->internal_cb_func_a = int_cb_func_a;
    cb->user_arg = user_arg;
    return cb_id;
}

s32 registerServicePushEventCallback(s32 title_user_ctx_id, s32 filter_id,
                                     OrbisNpWebApiServicePushEventCallback cb_func,
                                     OrbisNpWebApiInternalServicePushEventCallback int_cb_func,
                                     OrbisNpWebApiInternalServicePushEventCallbackA int_cb_func_a,
                                     void* user_arg) {
    OrbisNpWebApiContext* context = findAndValidateContext(title_user_ctx_id >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, title_user_ctx_id);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (g_sdk_ver >= Common::ElfInfo::FW_250 &&
        !context->service_push_event_filters.contains(filter_id)) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_SERVICE_PUSH_EVENT_FILTER_NOT_FOUND;
    }

    s32 result = registerServicePushEventCallbackInternal(user_context, filter_id, cb_func,
                                                          int_cb_func, int_cb_func_a, user_arg);
    releaseUserContext(user_context);
    releaseContext(context);
    return result;
}

s32 unregisterServicePushEventCallback(s32 title_user_ctx_id, s32 callback_id) {
    OrbisNpWebApiContext* context = findAndValidateContext(title_user_ctx_id >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, title_user_ctx_id);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (!user_context->service_push_event_callbacks.contains(callback_id)) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_SERVICE_PUSH_EVENT_CALLBACK_NOT_FOUND;
    }

    lockContext(context);
    user_context->service_push_event_callbacks.erase(callback_id);
    unlockContext(context);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 createExtendedPushEventFilterInternal(
    OrbisNpWebApiContext* context, s32 handle_id, const char* np_service_name, u32 np_service_label,
    const OrbisNpWebApiExtdPushEventFilterParameter* filter_param, u64 filter_param_num,
    bool internal) {
    std::scoped_lock lk{context->context_lock};
    if (!context->handles.contains(handle_id)) {
        return ORBIS_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND;
    }
    auto& handle = context->handles[handle_id];
    handle->user_count++;

    auto& np_handler = NpHandler::Instance();
    if (np_service_name != nullptr && !np_handler.IsActive()) {
        // Seems sceNpManagerIntGetUserList fails?
        LOG_DEBUG(Lib_NpWebApi, "Cannot create extended push event while shadNet is disabled");
        handle->user_count--;
        return ORBIS_NP_WEBAPI_ERROR_SIGNED_IN_USER_NOT_FOUND;
    }

    g_extended_push_event_filter_count++;
    if (g_extended_push_event_filter_count >= 0xf0000000) {
        g_extended_push_event_filter_count = 1;
    }
    s32 filter_id = g_extended_push_event_filter_count;

    context->extended_push_event_filters[filter_id] = new OrbisNpWebApiExtendedPushEventFilter{};
    auto& filter = context->extended_push_event_filters[filter_id];
    filter->internal = internal;
    filter->parent_context = context;
    filter->filter_id = filter_id;

    if (np_service_name == nullptr) {
        np_service_label = -1;
    } else {
        filter->np_service_name = std::string(np_service_name);
    }

    filter->np_service_label = np_service_label;

    LOG_INFO(Lib_NpWebApi, "filter_id={} service='{}' label={:#x} data_type_params={}", filter_id,
             np_service_name ? np_service_name : "null", np_service_label, filter_param_num);
    if (filter_param != nullptr && filter_param_num != 0) {
        for (u64 param_idx = 0; param_idx < filter_param_num; param_idx++) {
            OrbisNpWebApiExtdPushEventFilterParameter copy =
                OrbisNpWebApiExtdPushEventFilterParameter{};
            std::memcpy(&copy, &filter_param[param_idx],
                        sizeof(OrbisNpWebApiExtdPushEventFilterParameter));
            LOG_INFO(Lib_NpWebApi, "  filter_param[{}] data_type='{}' extd_keys={}", param_idx,
                     copy.data_type.val, copy.extd_data_key_num);
            std::vector<std::string> keys;
            if (copy.extd_data_key != nullptr) {
                for (u64 k = 0; k < copy.extd_data_key_num; k++) {
                    LOG_INFO(Lib_NpWebApi, "    extd_data_key[{}]='{}'", k,
                             copy.extd_data_key[k].val);
                    keys.emplace_back(
                        copy.extd_data_key[k].val,
                        strnlen(copy.extd_data_key[k].val, sizeof(copy.extd_data_key[k].val)));
                }
            }
            copy.extd_data_key = nullptr;
            filter->filter_params.emplace_back(copy);
            filter->extd_data_keys.emplace_back(std::move(keys));
        }
    }

    handle->user_count--;
    return filter_id;
}

s32 createExtendedPushEventFilter(s32 lib_ctx_id, s32 handle_id, const char* np_service_name,
                                  u32 np_service_label,
                                  const OrbisNpWebApiExtdPushEventFilterParameter* filter_param,
                                  u64 filter_param_num, bool internal) {
    OrbisNpWebApiContext* context = findAndValidateContext(lib_ctx_id);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    startHandleTimer(context, handle_id);
    s32 result =
        createExtendedPushEventFilterInternal(context, handle_id, np_service_name, np_service_label,
                                              filter_param, filter_param_num, internal);
    releaseContext(context);
    return result;
}

s32 deleteExtendedPushEventFilterInternal(OrbisNpWebApiContext* context, s32 filter_id) {
    std::scoped_lock lk{context->context_lock};
    if (!context->extended_push_event_filters.contains(filter_id)) {
        return ORBIS_NP_WEBAPI_ERROR_EXTD_PUSH_EVENT_FILTER_NOT_FOUND;
    }

    context->extended_push_event_filters[filter_id]->filter_params.clear();
    context->extended_push_event_filters.erase(filter_id);
    return ORBIS_OK;
}

s32 deleteExtendedPushEventFilter(s32 lib_ctx_id, s32 filter_id) {
    OrbisNpWebApiContext* context = findAndValidateContext(lib_ctx_id);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = deleteExtendedPushEventFilterInternal(context, filter_id);
    releaseContext(context);
    return result;
}

s32 registerExtdPushEventCallbackInternal(OrbisNpWebApiUserContext* user_context, s32 filter_id,
                                          OrbisNpWebApiExtdPushEventCallback cb_func,
                                          OrbisNpWebApiExtdPushEventCallbackA cb_func_a,
                                          void* user_arg) {
    std::scoped_lock lk{user_context->parent_context->context_lock};

    g_registered_callback_count++;
    if (g_registered_callback_count >= 0xf0000000) {
        g_registered_callback_count = 1;
    }

    if (cb_func == nullptr && cb_func_a == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    s32 cb_id = g_registered_callback_count;

    user_context->extended_push_event_callbacks[cb_id] =
        new OrbisNpWebApiRegisteredExtendedPushEventCallback{};
    auto& cb = user_context->extended_push_event_callbacks[cb_id];
    cb->callback_id = cb_id;
    cb->filter_id = filter_id;
    cb->cb_func = cb_func;
    cb->cb_func_a = cb_func_a;
    cb->user_arg = user_arg;
    return cb_id;
}

s32 registerExtdPushEventCallback(s32 title_user_ctx_id, s32 filter_id,
                                  OrbisNpWebApiExtdPushEventCallback cb_func,
                                  OrbisNpWebApiExtdPushEventCallbackA cb_func_a, void* user_arg) {
    OrbisNpWebApiContext* context = findAndValidateContext(title_user_ctx_id >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, title_user_ctx_id);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (!context->extended_push_event_filters.contains(filter_id)) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_EXTD_PUSH_EVENT_FILTER_NOT_FOUND;
    }

    s32 result = registerExtdPushEventCallbackInternal(user_context, filter_id, cb_func, cb_func_a,
                                                       user_arg);
    releaseUserContext(user_context);
    releaseContext(context);
    return result;
}

s32 registerExtdPushEventCallbackA(s32 title_user_ctx_id, s32 filter_id,
                                   OrbisNpWebApiExtdPushEventCallbackA cb_func, void* user_arg) {
    return registerExtdPushEventCallback(title_user_ctx_id, filter_id, nullptr, cb_func, user_arg);
}

s32 unregisterExtdPushEventCallback(s32 title_user_ctx_id, s32 callback_id) {
    OrbisNpWebApiContext* context = findAndValidateContext(title_user_ctx_id >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, title_user_ctx_id);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (!user_context->extended_push_event_callbacks.contains(callback_id)) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_EXTD_PUSH_EVENT_CALLBACK_NOT_FOUND;
    }

    lockContext(context);
    user_context->extended_push_event_callbacks.erase(callback_id);
    unlockContext(context);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI getHttpRequestIdFromRequest(OrbisNpWebApiRequest* request) {
    return request->http_request_id;
}

// Request/response header support
bool iEquals(const char* a, u64 a_len, const char* b) {
    u64 i = 0;
    for (; i < a_len && b[i]; ++i) {
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i]))
            return false;
    }
    return i == a_len && b[i] == '\0';
}

bool findHeaderValue(const char* block, u64 block_len, const char* field, std::string& out_value) {
    u64 i = 0;
    while (i < block_len) {
        u64 line_start = i;
        while (i < block_len && block[i] != '\n')
            ++i;
        u64 line_end = i;
        if (i < block_len)
            ++i;
        if (line_end > line_start && block[line_end - 1] == '\r')
            --line_end;
        u64 colon = line_start;
        while (colon < line_end && block[colon] != ':')
            ++colon;
        if (colon >= line_end)
            continue;
        u64 name_len = colon - line_start;
        if (!iEquals(block + line_start, name_len, field))
            continue;
        u64 vs = colon + 1;
        while (vs < line_end && (block[vs] == ' ' || block[vs] == '\t'))
            ++vs;
        out_value.assign(block + vs, line_end - vs);
        return true;
    }
    return false;
}

bool parseErrorCode(const char* body, u64 len, s32& out) {
    std::string_view sv(body, len);
    u64 k = sv.find("\"code\"");
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

s32 addHttpRequestHeaderInternal(s64 request_id, const char* field_name, const char* value) {
    OrbisNpWebApiContext* context = findAndValidateContext(request_id >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }
    OrbisNpWebApiUserContext* user_context = findUserContext(context, request_id >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }
    OrbisNpWebApiRequest* request = findRequestAndMarkBusy(user_context, request_id);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }
    request->user_headers.emplace_back(field_name, value);
    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 getHttpResponseHeaderValueInternal(s64 request_id, const char* field_name, char* out_value,
                                       u64 value_size, u64* value_length) {
    OrbisNpWebApiContext* context = findAndValidateContext(request_id >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }
    OrbisNpWebApiUserContext* user_context = findUserContext(context, request_id >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }
    OrbisNpWebApiRequest* request = findRequestAndMarkBusy(user_context, request_id);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }

    char* block = nullptr;
    u64 block_size = 0;
    const s32 http_req_id = getHttpRequestIdFromRequest(request);
    const s32 err = sceHttpGetAllResponseHeaders(http_req_id, &block, &block_size);

    s32 result = ORBIS_OK;
    if (err < 0) {
        result = err;
    } else {
        std::string value;
        const bool found =
            block != nullptr && findHeaderValue(block, block_size, field_name, value);
        if (value_length != nullptr)
            *value_length = found ? value.size() : 0;
        if (out_value != nullptr && value_size > 0) {
            const u64 n = found ? std::min<u64>(value.size(), value_size - 1) : 0;
            if (n > 0)
                std::memcpy(out_value, value.data(), n);
            out_value[n] = '\0';
        }
    }

    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return result;
}

s32 PS4_SYSV_ABI getHttpStatusCodeInternal(s64 request_id, s32* out_status_code) {
    s32 status_code;
    OrbisNpWebApiContext* context = findAndValidateContext(request_id >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, request_id >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiRequest* request = findRequestAndMarkBusy(user_context, request_id);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }

    // Query HTTP layer
    {
        s32 http_req_id = getHttpRequestIdFromRequest(request);
        s32 err = sceHttpGetStatusCode(http_req_id, &status_code);

        if (out_status_code != nullptr) {
            *out_status_code = status_code;
        }

        releaseRequest(request);
        releaseUserContext(user_context);
        releaseContext(context);

        return err;
    }
}

void PS4_SYSV_ABI setRequestEndTime(OrbisNpWebApiRequest* request) {
    u64 time;
    if ((request->request_timeout != 0) && (request->request_end_time == 0)) {
        time = sceKernelGetProcessTime();
        request->request_end_time = (u64)request->request_timeout + time;
    }
}

void PS4_SYSV_ABI clearRequestEndTime(OrbisNpWebApiRequest* req) {
    req->request_end_time = 0;
    return;
}

bool PS4_SYSV_ABI hasRequestTimedOut(OrbisNpWebApiRequest* request) {
    return request->timed_out;
}

bool PS4_SYSV_ABI isRequestAborted(OrbisNpWebApiRequest* request) {
    return request->aborted;
}

void PS4_SYSV_ABI setRequestState(OrbisNpWebApiRequest* request, u8 state) {
    request->request_state = state;
}

u64 PS4_SYSV_ABI copyRequestData(OrbisNpWebApiRequest* request, void* data, u64 size) {
    u64 read_size = 0;
    if (request->remaining_data != 0) {
        u64 remaining_size = request->remaining_data - request->read_offset;
        if (remaining_size != 0) {
            if (remaining_size < size) {
                size = remaining_size;
            }
            std::memcpy(data, request->data + request->read_offset, size);
            request->read_offset += static_cast<u32>(size);
            read_size = size;
        }
    }
    return read_size;
}

s32 PS4_SYSV_ABI readDataInternal(s64 request_id, void* data, u64 size) {
    u32 offset;
    s32 result;
    u64 remaining_size;
    u64 bytes_copied;

    OrbisNpWebApiContext* context = findAndValidateContext(request_id >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, request_id >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiRequest* request = findRequestAndMarkBusy(user_context, request_id);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }

    setRequestEndTime(request);

    bytes_copied = copyRequestData(request, data, size);
    offset = (u32)bytes_copied;
    remaining_size = size - offset;

    if (remaining_size != 0) {
        lockContext(context);
        setRequestState(request, 5);

        if (!hasRequestTimedOut(request) && isRequestAborted(request)) {
            unlockContext(context);
            offset = ORBIS_NP_WEBAPI_ERROR_ABORTED;
        } else {
            unlockContext(context);

            s32 http_req_id = getHttpRequestIdFromRequest(request);
            s32 http_read =
                sceHttpReadData(http_req_id, reinterpret_cast<u8*>(data) + offset, remaining_size);

            if (http_read < 0) {
                http_read = 0;
            }

            offset += http_read;
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
                captureWebApiError(reinterpret_cast<const char*>(data), offset);
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

std::mutex g_push_mutex;
std::deque<PushEventInput> g_push_queue;

template <typename Filter>
bool filterMatches(const Filter* flt, const std::string& ev_service_name, bool ev_has_service_name,
                   const std::string& ev_data_type) {
    const bool catch_all = flt->np_service_name.empty() && flt->np_service_label == 0xffffffffu;
    if (catch_all) {
        if (ev_has_service_name) {
            return false;
        }
    } else if (!ev_has_service_name || flt->np_service_name != ev_service_name) {
        return false;
    }
    for (const auto& p : flt->filter_params) {
        if (ev_data_type == p.data_type.val) {
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
        const bool ev_has_service = !ev.np_service_name.empty();
        OrbisNpWebApiPushEventDataType dt{};
        std::snprintf(dt.val, sizeof(dt.val), "%s", ev.data_type.c_str());
        const OrbisNpOnlineId* from_p = ev.has_from ? &ev.from_online_id : nullptr;
        const OrbisNpOnlineId* to_p = ev.has_to ? &ev.to_online_id : nullptr;
        OrbisNpPeerAddressA to_addr{};
        OrbisNpPeerAddressA from_addr{};
        to_addr.account_id = ev.to_account_id;
        to_addr.platform = OrbisNpPlatformType::PS4;
        from_addr.account_id = ev.from_account_id;
        from_addr.platform = OrbisNpPlatformType::PS4;
        const OrbisNpPeerAddressA* to_addr_p = ev.to_account_id != 0 ? &to_addr : nullptr;
        const OrbisNpPeerAddressA* from_addr_p = ev.from_account_id != 0 ? &from_addr : nullptr;

        std::scoped_lock gl{g_global_mutex};
        for (auto& [libId, context] : g_contexts) {
            if (context == nullptr) {
                continue;
            }
            std::scoped_lock cl{context->context_lock};
            for (auto& [ucKey, uc] : context->user_contexts) {
                if (uc == nullptr) {
                    continue;
                }
                if (uc->user_id != ev.target_user_id) {
                    continue;
                }
                const s32 title_user_ctx_id = ucKey;

                for (auto& [cbId, cb] : uc->extended_push_event_callbacks) {
                    if (cb == nullptr) {
                        continue;
                    }
                    void (*raw)() = cb->cb_func_a ? reinterpret_cast<void (*)()>(cb->cb_func_a)
                                    : cb->cb_func ? reinterpret_cast<void (*)()>(cb->cb_func)
                                                  : nullptr;
                    if (raw == nullptr) {
                        continue;
                    }
                    auto fit = context->extended_push_event_filters.find(cb->filter_id);
                    if (fit == context->extended_push_event_filters.end()) {
                        continue;
                    }
                    const OrbisNpWebApiExtendedPushEventFilter* flt = fit->second;
                    if (!filterMatches(flt, ev.np_service_name, ev_has_service, ev.data_type)) {
                        continue;
                    }
                    const std::vector<std::string>* wanted_keys = nullptr;
                    for (u64 i = 0; i < flt->filter_params.size(); ++i) {
                        if (ev.data_type == flt->filter_params[i].data_type.val) {
                            if (i < flt->extd_data_keys.size()) {
                                wanted_keys = &flt->extd_data_keys[i];
                            }
                            break;
                        }
                    }
                    auto key_requested = [wanted_keys](const std::string& k) {
                        return wanted_keys != nullptr &&
                               std::find(wanted_keys->begin(), wanted_keys->end(), k) !=
                                   wanted_keys->end();
                    };
                    std::vector<OrbisNpWebApiExtdPushEventExtdData> exarr;
                    exarr.reserve(ev.extd_data.size());
                    for (auto& [k, v] : ev.extd_data) {
                        if (!key_requested(k)) {
                            continue;
                        }
                        OrbisNpWebApiExtdPushEventExtdData e{};
                        std::snprintf(e.extd_data_key.val, sizeof(e.extd_data_key.val), "%s",
                                      k.c_str());
                        e.data = const_cast<char*>(v.data());
                        e.data_len = v.size();
                        exarr.push_back(e);
                    }
                    const char* svc =
                        flt->np_service_name.empty() ? nullptr : flt->np_service_name.c_str();
                    const char* ext_data = ev.data.empty() ? nullptr : ev.data.data();
                    const OrbisNpWebApiExtdPushEventExtdData* ext_arr =
                        exarr.empty() ? nullptr : exarr.data();

                    reinterpret_cast<OrbisNpWebApiExtdPushEventCallbackA>(raw)(
                        title_user_ctx_id, cbId, svc, flt->np_service_label, to_addr_p, to_p,
                        from_addr_p, from_p, &dt, ext_data, ev.data.size(), ext_arr, exarr.size(),
                        cb->user_arg);
                }

                // Service push
                for (auto& [cbId, cb] : uc->service_push_event_callbacks) {
                    if (cb == nullptr || cb->cb_func == nullptr) {
                        continue;
                    }
                    auto fit = context->service_push_event_filters.find(cb->filter_id);
                    if (fit == context->service_push_event_filters.end()) {
                        continue;
                    }
                    const OrbisNpWebApiServicePushEventFilter* flt = fit->second;
                    if (!filterMatches(flt, ev.np_service_name, ev_has_service, ev.data_type)) {
                        continue;
                    }
                    const char* svc =
                        flt->np_service_name.empty() ? nullptr : flt->np_service_name.c_str();
                    const char* svc_data = ev.data.empty() ? nullptr : ev.data.data();
                    OrbisNpPeerAddress to_peer{};
                    OrbisNpPeerAddress from_peer{};
                    if (ev.has_to) {
                        to_peer.online_id = ev.to_online_id;
                    }
                    if (ev.has_from) {
                        from_peer.online_id = ev.from_online_id;
                    }
                    reinterpret_cast<OrbisNpWebApiServicePushEventCallback>(
                        reinterpret_cast<void (*)()>(cb->cb_func))(
                        title_user_ctx_id, cbId, svc, flt->np_service_label, &to_peer, &from_peer,
                        &dt, svc_data, ev.data.size(), cb->user_arg);
                }

                // Basic push
                if (!ev_has_service) {
                    for (auto& [cbId, cb] : uc->push_event_callbacks) {
                        if (cb == nullptr || cb->cb_func == nullptr) {
                            continue;
                        }
                        auto fit = context->push_event_filters.find(cb->filter_id);
                        if (fit == context->push_event_filters.end()) {
                            continue;
                        }
                        const OrbisNpWebApiPushEventFilter* flt = fit->second;
                        bool matched = false;
                        for (const auto& p : flt->filter_params) {
                            if (ev.data_type == p.data_type.val) {
                                matched = true;
                                break;
                            }
                        }
                        if (!matched) {
                            continue;
                        }
                        OrbisNpPeerAddress to_peer{};
                        OrbisNpPeerAddress from_peer{};
                        if (ev.has_to) {
                            to_peer.online_id = ev.to_online_id;
                        }
                        if (ev.has_from) {
                            from_peer.online_id = ev.from_online_id;
                        }
                        const char* data = ev.data.empty() ? nullptr : ev.data.data();
                        reinterpret_cast<OrbisNpWebApiPushEventCallback>(
                            reinterpret_cast<void (*)()>(cb->cb_func))(
                            title_user_ctx_id, cbId, &to_peer, &from_peer, &dt, data,
                            ev.data.size(), cb->user_arg);
                    }
                }
            }
        }
    }
}
}; // namespace Libraries::Np::NpWebApi
