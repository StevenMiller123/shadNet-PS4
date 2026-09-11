// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <magic_enum/magic_enum.hpp>
#include <orbis/NpWebApi.h>
#include <orbis/UserService.h>
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_manager/np_manager.h"
#include "core/libraries/np/np_web_api/np_web_api.h"
#include "core/libraries/np/np_web_api/np_web_api_internal.h"

extern "C" {
void sceNpWebApiVshInitialize();
void sceNpWebApiInitializeForPresence();
void sceNpWebApiIntCreateRequest();
void sceNpWebApiIntRegisterServicePushEventCallback();
void sceNpWebApiIntRegisterServicePushEventCallbackA();

SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiCreateContext);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiCreatePushEventFilter);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiCreateServicePushEventFilter);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiDeletePushEventFilter);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiDeleteServicePushEventFilter);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiRegisterExtdPushEventCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiRegisterNotificationCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiRegisterPushEventCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiRegisterServicePushEventCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiUnregisterNotificationCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiUnregisterPushEventCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiUnregisterServicePushEventCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiAbortHandle);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiAbortRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiAddHttpRequestHeader);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiAddMultipartPart);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiCheckTimeout);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiClearAllUnusedConnection);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiClearUnusedConnection);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiCreateContextA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiCreateExtdPushEventFilter);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiCreateHandle);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiCreateMultipartRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiCreateRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiDeleteContext);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiDeleteExtdPushEventFilter);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiDeleteHandle);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiDeleteRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiGetConnectionStats);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiGetHttpResponseHeaderValue);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiGetHttpResponseHeaderValueLength);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiGetHttpStatusCode);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiGetMemoryPoolStats);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiInitialize);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiInitializeForPresence);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiIntCreateCtxIndExtdPushEventFilter);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiIntCreateRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiIntCreateServicePushEventFilter);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiIntInitialize);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiIntRegisterServicePushEventCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiIntRegisterServicePushEventCallbackA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiReadData);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiRegisterExtdPushEventCallbackA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiSendMultipartRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiSendMultipartRequest2);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiSendRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiSendRequest2);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiSetHandleTimeout);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiSetMaxConnection);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiSetMultipartContentType);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiSetRequestTimeout);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiTerminate);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiUnregisterExtdPushEventCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi, sceNpWebApiVshInitialize);

static void RegisterLibraryHooks() {
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiCreateContext);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiCreatePushEventFilter);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiCreateServicePushEventFilter);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiDeletePushEventFilter);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiDeleteServicePushEventFilter);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiRegisterExtdPushEventCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiRegisterNotificationCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiRegisterPushEventCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiRegisterServicePushEventCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiUnregisterNotificationCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiUnregisterPushEventCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiUnregisterServicePushEventCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiAbortHandle);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiAbortRequest);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiAddHttpRequestHeader);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiAddMultipartPart);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiCheckTimeout);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiClearAllUnusedConnection);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiClearUnusedConnection);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiCreateContext);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiCreateContextA);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiCreateExtdPushEventFilter);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiCreateHandle);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiCreateMultipartRequest);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiCreatePushEventFilter);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiCreateRequest);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiCreateServicePushEventFilter);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiDeleteContext);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiDeleteExtdPushEventFilter);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiDeleteHandle);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiDeletePushEventFilter);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiDeleteRequest);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiDeleteServicePushEventFilter);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiGetConnectionStats);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiGetHttpResponseHeaderValue);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiGetHttpResponseHeaderValueLength);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiGetHttpStatusCode);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiGetMemoryPoolStats);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiInitialize);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiInitializeForPresence);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiIntCreateCtxIndExtdPushEventFilter);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiIntCreateRequest);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiIntCreateServicePushEventFilter);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiIntInitialize);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiIntRegisterServicePushEventCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiIntRegisterServicePushEventCallbackA);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiReadData);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiRegisterExtdPushEventCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiRegisterExtdPushEventCallbackA);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiRegisterNotificationCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiRegisterPushEventCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiRegisterServicePushEventCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiSendMultipartRequest);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiSendMultipartRequest2);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiSendRequest);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiSendRequest2);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiSetHandleTimeout);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiSetMaxConnection);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiSetMultipartContentType);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiSetRequestTimeout);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiTerminate);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiUnregisterExtdPushEventCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiUnregisterNotificationCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiUnregisterPushEventCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiUnregisterServicePushEventCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiVshInitialize);
}
}

namespace Libraries::Np::NpWebApi {

static bool g_is_initialized = false;
static s32 g_active_library_contexts = 0;

s32 sceNpWebApiCreateContext(s32 libCtxId, OrbisNpOnlineId* onlineId) {
    if (libCtxId >= 0x8000) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_LIB_CONTEXT_ID;
    }
    if (onlineId == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    return createUserContextWithOnlineId(libCtxId, onlineId);
}

s32 sceNpWebApiCreatePushEventFilter(s32 libCtxId,
                                     const OrbisNpWebApiPushEventFilterParameter* pFilterParam,
                                     u64 filterParamNum) {
    if (pFilterParam == nullptr || filterParamNum == 0) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_WARNING(Lib_NpWebApi, "called, libCtxId = {:#x}", libCtxId);
    return createPushEventFilter(libCtxId, pFilterParam, filterParamNum);
}

s32 sceNpWebApiCreateServicePushEventFilter(
    s32 libCtxId, s32 handleId, const char* pNpServiceName, u32 npServiceLabel,
    const OrbisNpWebApiServicePushEventFilterParameter* pFilterParam, u64 filterParamNum) {
    if (pNpServiceName == nullptr || pFilterParam == nullptr || filterParamNum == 0) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    if (getCompiledSdkVersion() >= Common::ElfInfo::FW_200 && npServiceLabel == -1) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_WARNING(Lib_NpWebApi,
                "called, libCtxId = {:#x}, handleId = {:#x}, pNpServiceName = '{}', "
                "npServiceLabel = {:#x}",
                libCtxId, handleId, pNpServiceName, npServiceLabel);
    return createServicePushEventFilter(libCtxId, handleId, pNpServiceName, npServiceLabel,
                                        pFilterParam, filterParamNum);
}

s32 sceNpWebApiDeletePushEventFilter(s32 libCtxId, s32 filterId) {
    LOG_INFO(Lib_NpWebApi, "called, libCtxId = {:#x}, filterId = {:#x}", libCtxId, filterId);
    return deletePushEventFilter(libCtxId, filterId);
}

s32 sceNpWebApiDeleteServicePushEventFilter(s32 libCtxId, s32 filterId) {
    LOG_INFO(Lib_NpWebApi, "called, libCtxId = {:#x}, filterId = {:#x}", libCtxId, filterId);
    return deleteServicePushEventFilter(libCtxId, filterId);
}

s32 sceNpWebApiRegisterExtdPushEventCallback(s32 titleUserCtxId, s32 filterId,
                                             OrbisNpWebApiExtdPushEventCallback cbFunc,
                                             void* pUserArg) {
    if (cbFunc == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi, "called, titleUserCtxId = {:#x}, filterId = {:#x}, cbFunc = {}",
             titleUserCtxId, filterId, reinterpret_cast<void*>(cbFunc));
    return registerExtdPushEventCallback(titleUserCtxId, filterId, cbFunc, nullptr, pUserArg);
}

s32 sceNpWebApiRegisterNotificationCallback(s32 titleUserCtxId,
                                            OrbisNpWebApiNotificationCallback cbFunc,
                                            void* pUserArg) {
    if (cbFunc == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi, "called, titleUserCtxId = {:#x}, cbFunc = {}", titleUserCtxId,
             reinterpret_cast<void*>(cbFunc));
    return registerNotificationCallback(titleUserCtxId, cbFunc, pUserArg);
}

s32 sceNpWebApiRegisterPushEventCallback(s32 titleUserCtxId, s32 filterId,
                                         OrbisNpWebApiPushEventCallback cbFunc, void* pUserArg) {
    if (getCompiledSdkVersion() >= Common::ElfInfo::FW_100 && cbFunc == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi, "called, titleUserCtxId = {:#x}, filterId = {:#x}, cbFunc = {}",
             titleUserCtxId, filterId, reinterpret_cast<void*>(cbFunc));
    return registerPushEventCallback(titleUserCtxId, filterId, cbFunc, pUserArg);
}

s32 sceNpWebApiRegisterServicePushEventCallback(s32 titleUserCtxId, s32 filterId,
                                                OrbisNpWebApiServicePushEventCallback cbFunc,
                                                void* pUserArg) {
    if (getCompiledSdkVersion() >= Common::ElfInfo::FW_100 && cbFunc == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi, "called, titleUserCtxId = {:#x}, filterId = {:#x}, cbFunc = {}",
             titleUserCtxId, filterId, reinterpret_cast<void*>(cbFunc));
    return registerServicePushEventCallback(titleUserCtxId, filterId, cbFunc, nullptr, nullptr,
                                            pUserArg);
}

s32 sceNpWebApiUnregisterNotificationCallback(s32 titleUserCtxId) {
    LOG_INFO(Lib_NpWebApi, "called, titleUserCtxId = {:#x}", titleUserCtxId);
    return unregisterNotificationCallback(titleUserCtxId);
}

s32 sceNpWebApiUnregisterPushEventCallback(s32 titleUserCtxId, s32 callbackId) {
    LOG_INFO(Lib_NpWebApi, "called, titleUserCtxId = {:#x}, callbackId = {:#x}", titleUserCtxId,
             callbackId);
    return unregisterPushEventCallback(titleUserCtxId, callbackId);
}

s32 sceNpWebApiUnregisterServicePushEventCallback(s32 titleUserCtxId, s32 callbackId) {
    LOG_INFO(Lib_NpWebApi, "called, titleUserCtxId = {:#x}, callbackId = {:#x}", titleUserCtxId,
             callbackId);
    return unregisterServicePushEventCallback(titleUserCtxId, callbackId);
}

s32 sceNpWebApiAbortHandle(s32 libCtxId, s32 handleId) {
    LOG_INFO(Lib_NpWebApi, "called, libCtxId = {:#x}, handleId = {:#x}", libCtxId, handleId);
    return abortHandle(libCtxId, handleId);
}

s32 sceNpWebApiAbortRequest(s64 requestId) {
    LOG_INFO(Lib_NpWebApi, "called, requestId = {:#x}", requestId);
    return abortRequest(requestId);
}

s32 sceNpWebApiAddHttpRequestHeader(s64 requestId, const char* pFieldName, const char* pValue) {
    LOG_INFO(Lib_NpWebApi, "called, requestId = {:#x}, pFieldName = '{}', pValue = '{}'", requestId,
             (pFieldName ? pFieldName : "null"), (pValue ? pValue : "null"));
    if (pFieldName == nullptr || pValue == nullptr)
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    return addHttpRequestHeaderInternal(requestId, pFieldName, pValue);
}

s32 sceNpWebApiAddMultipartPart(s64 requestId, const OrbisNpWebApiMultipartPartParameter* pParam,
                                s32* pIndex) {
    LOG_INFO(Lib_NpWebApi, "called, requestId = {:#x}, headerNum = {}, contentLength = {}",
             requestId, (pParam ? pParam->headerNum : 0), (pParam ? pParam->contentLength : 0));
    return addMultipartPart(requestId, pParam, pIndex);
}

void sceNpWebApiCheckTimeout() {
    LOG_TRACE(Lib_NpWebApi, "called");
    if (!g_is_initialized) {
        return;
    }
    return checkTimeout();
}

s32 sceNpWebApiClearAllUnusedConnection(s32 userCtxId, bool bRemainKeepAliveConnection) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED), userCtxId = {:#x}, "
              "bRemainKeepAliveConnection = {}",
              userCtxId, bRemainKeepAliveConnection);
    return ORBIS_OK;
}

s32 sceNpWebApiClearUnusedConnection(s32 userCtxId, const char* pApiGroup,
                                     bool bRemainKeepAliveConnection) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED), userCtxId = {:#x}, "
              "pApiGroup = '{}', bRemainKeepAliveConnection = {}",
              userCtxId, (pApiGroup ? pApiGroup : "null"), bRemainKeepAliveConnection);
    return ORBIS_OK;
}

s32 sceNpWebApiCreateContextA(s32 libCtxId, s32 userId) {
    if (libCtxId >= 0x8000) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_LIB_CONTEXT_ID;
    }
    if (userId == ORBIS_USER_SERVICE_USER_ID_INVALID) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    return createUserContext(libCtxId, userId);
}

s32 sceNpWebApiCreateExtdPushEventFilter(
    s32 libCtxId, s32 handleId, const char* pNpServiceName, u32 npServiceLabel,
    const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam, u64 filterParamNum) {
    if ((pNpServiceName != nullptr && npServiceLabel == -1) || pFilterParam == nullptr ||
        filterParamNum == 0) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    LOG_INFO(
        Lib_NpWebApi,
        "called, libCtxId = {:#x}, handleId = {:#x}, pNpServiceName = '{}', npServiceLabel = {:#x}",
        libCtxId, handleId, (pNpServiceName ? pNpServiceName : "null"), npServiceLabel);
    return createExtendedPushEventFilter(libCtxId, handleId, pNpServiceName, npServiceLabel,
                                         pFilterParam, filterParamNum, false);
}

s32 sceNpWebApiCreateHandle(s32 libCtxId) {
    return createHandle(libCtxId);
}

s32 sceNpWebApiCreateMultipartRequest(s32 titleUserCtxId, const char* pApiGroup, const char* pPath,
                                      OrbisNpWebApiHttpMethod method, s64* pRequestId) {
    if (pApiGroup == nullptr || pPath == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    if (getCompiledSdkVersion() >= Common::ElfInfo::FW_250 &&
        method > OrbisNpWebApiHttpMethod::ORBIS_NP_WEBAPI_HTTP_METHOD_DELETE) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    LOG_INFO(Lib_NpWebApi,
             "called, titleUserCtxId = {:#x}, pApiGroup = '{}', pPath = '{}', method = {}",
             titleUserCtxId, pApiGroup, pPath, magic_enum::enum_name(method));

    return createRequest(titleUserCtxId, pApiGroup, pPath, method, nullptr, nullptr, pRequestId,
                         true);
}

s32 sceNpWebApiCreateRequest(s32 titleUserCtxId, const char* pApiGroup, const char* pPath,
                             OrbisNpWebApiHttpMethod method,
                             const OrbisNpWebApiContentParameter* pContentParameter,
                             s64* pRequestId) {
    if (pApiGroup == nullptr || pPath == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    if (pContentParameter != nullptr && pContentParameter->content_length != 0 &&
        pContentParameter->content_type == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_CONTENT_PARAMETER;
    }

    if (getCompiledSdkVersion() >= Common::ElfInfo::FW_250 &&
        method > OrbisNpWebApiHttpMethod::ORBIS_NP_WEBAPI_HTTP_METHOD_DELETE) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    LOG_INFO(Lib_NpWebApi,
             "called, titleUserCtxId = {:#x}, pApiGroup = '{}', pPath = '{}', method = {}",
             titleUserCtxId, pApiGroup, pPath, magic_enum::enum_name(method));

    return createRequest(titleUserCtxId, pApiGroup, pPath, method, pContentParameter, nullptr,
                         pRequestId, false);
}

s32 sceNpWebApiDeleteContext(s32 titleUserCtxId) {
    LOG_INFO(Lib_NpWebApi, "called titleUserCtxId = {:#x}", titleUserCtxId);
    return deleteUserContext(titleUserCtxId);
}

s32 sceNpWebApiDeleteExtdPushEventFilter(s32 libCtxId, s32 filterId) {
    LOG_INFO(Lib_NpWebApi, "called libCtxId = {:#x}, filterId = {:#x}", libCtxId, filterId);
    return deleteExtendedPushEventFilter(libCtxId, filterId);
}

s32 sceNpWebApiDeleteHandle(s32 libCtxId, s32 handleId) {
    LOG_INFO(Lib_NpWebApi, "called libCtxId = {:#x}, handleId = {:#x}", libCtxId, handleId);
    return deleteHandle(libCtxId, handleId);
}

s32 sceNpWebApiDeleteRequest(s64 requestId) {
    LOG_INFO(Lib_NpWebApi, "called requestId = {:#x}", requestId);
    return deleteRequest(requestId);
}

s32 sceNpWebApiGetConnectionStats(s32 userCtxId, const char* pApiGroup,
                                  OrbisNpWebApiConnectionStats* pStats) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED), userCtxId = {:#x}, "
              "pApiGroup = '{}', pStats = {}",
              userCtxId, (pApiGroup ? pApiGroup : "null"), fmt::ptr(pStats));
    return ORBIS_OK;
}

s32 sceNpWebApiGetErrorCode() {
    const s32 code = getLastWebApiError();
    LOG_INFO(Lib_NpWebApi, "called, lastErrorCode = {:#x}", code);
    return code;
}

s32 sceNpWebApiGetHttpResponseHeaderValue(s64 requestId, const char* pFieldName, char* pValue,
                                          u64 valueSize) {
    LOG_INFO(Lib_NpWebApi, "called, requestId = {:#x}, pFieldName = '{}', valueSize = {}",
             requestId, (pFieldName ? pFieldName : "null"), valueSize);
    if (pFieldName == nullptr || pValue == nullptr || valueSize == 0)
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    return getHttpResponseHeaderValueInternal(requestId, pFieldName, pValue, valueSize, nullptr);
}

s32 sceNpWebApiGetHttpResponseHeaderValueLength(s64 requestId, const char* pFieldName,
                                                u64* pValueLength) {
    LOG_INFO(Lib_NpWebApi, "called, requestId = {:#x}, pFieldName = '{}'", requestId,
             (pFieldName ? pFieldName : "null"));
    if (pFieldName == nullptr || pValueLength == nullptr)
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    return getHttpResponseHeaderValueInternal(requestId, pFieldName, nullptr, 0, pValueLength);
}

s32 sceNpWebApiGetHttpStatusCode(s64 requestId, s32* out_status_code) {
    LOG_INFO(Lib_NpWebApi, "called, requestId = {:#x}", requestId);
    // On newer SDKs, NULL output pointer is invalid
    if (getCompiledSdkVersion() > Common::ElfInfo::FW_100 && out_status_code == nullptr)
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    s32 returncode = getHttpStatusCodeInternal(requestId, out_status_code);
    return returncode;
}

s32 sceNpWebApiGetMemoryPoolStats(s32 libCtxId, OrbisNpWebApiMemoryPoolStats* pCurrentStat) {
    LOG_ERROR(Lib_NpWebApi, "called (STUBBED), libCtxId = {:#x}, pCurrentStat = {}", libCtxId,
              fmt::ptr(pCurrentStat));
    return ORBIS_OK;
}

s32 sceNpWebApiInitialize(s32 libHttpCtxId, u64 poolSize) {
    LOG_INFO(Lib_NpWebApi, "called, libHttpCtxId = {:#x}, poolSize = {:#x} bytes", libHttpCtxId,
             poolSize);
    if (!g_is_initialized) {
        g_is_initialized = true;
        s32 result = initializeLibrary();
        if (result < ORBIS_OK) {
            return result;
        }
    }

    s32 result = createLibraryContext(libHttpCtxId, poolSize, nullptr, 0);
    if (result >= ORBIS_OK) {
        g_active_library_contexts++;
    }
    return result;
}

s32 sceNpWebApiInitializeForPresence(s32 libHttpCtxId, u64 poolSize) {
    LOG_INFO(Lib_NpWebApi, "called, libHttpCtxId = {:#x}, poolSize = {:#x} bytes", libHttpCtxId,
             poolSize);
    if (!g_is_initialized) {
        g_is_initialized = true;
        s32 result = initializeLibrary();
        if (result < ORBIS_OK) {
            return result;
        }
    }

    s32 result = createLibraryContext(libHttpCtxId, poolSize, nullptr, 3);
    if (result >= ORBIS_OK) {
        g_active_library_contexts++;
    }
    return result;
}

s32 sceNpWebApiIntCreateCtxIndExtdPushEventFilter(
    s32 libCtxId, s32 handleId, const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam,
    u64 filterParamNum) {
    if (pFilterParam == nullptr || filterParamNum == 0) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    LOG_INFO(Lib_NpWebApi, "called, libCtxId = {:#x}, handleId = {:#x}", libCtxId, handleId);
    return createExtendedPushEventFilter(libCtxId, handleId, nullptr, -1, pFilterParam,
                                         filterParamNum, true);
}

s32 sceNpWebApiIntCreateRequest(s32 titleUserCtxId, const char* pApiGroup, const char* pPath,
                                OrbisNpWebApiHttpMethod method,
                                const OrbisNpWebApiContentParameter* pContentParameter,
                                const OrbisNpWebApiIntCreateRequestExtraArgs* pInternalArgs,
                                s64* pRequestId) {
    LOG_INFO(Lib_NpWebApi, "called");
    if (pApiGroup == nullptr || pPath == nullptr ||
        method > OrbisNpWebApiHttpMethod::ORBIS_NP_WEBAPI_HTTP_METHOD_PATCH) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    if (pContentParameter != nullptr && pContentParameter->content_length != 0 &&
        pContentParameter->content_type == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_CONTENT_PARAMETER;
    }

    LOG_INFO(Lib_NpWebApi,
             "called, titleUserCtxId = {:#x}, pApiGroup = '{}', pPath = '{}', method = {}",
             titleUserCtxId, pApiGroup, pPath, magic_enum::enum_name(method));

    return createRequest(titleUserCtxId, pApiGroup, pPath, method, pContentParameter, pInternalArgs,
                         pRequestId, false);
}

s32 sceNpWebApiIntCreateServicePushEventFilter(
    s32 libCtxId, s32 handleId, const char* pNpServiceName, u32 npServiceLabel,
    const OrbisNpWebApiServicePushEventFilterParameter* pFilterParam, u64 filterParamNum) {
    if (pFilterParam == nullptr || filterParamNum == 0) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_WARNING(Lib_NpWebApi,
                "called, libCtxId = {:#x}, handleId = {:#x}, pNpServiceName = '{}', "
                "npServiceLabel = {:#x}",
                libCtxId, handleId, (pNpServiceName ? pNpServiceName : "null"), npServiceLabel);
    return createServicePushEventFilter(libCtxId, handleId, pNpServiceName, npServiceLabel,
                                        pFilterParam, filterParamNum);
}

s32 sceNpWebApiIntInitialize(const OrbisNpWebApiInitializeParameter* args) {
    LOG_INFO(Lib_NpWebApi, "called");
    if (args == nullptr || args->size != sizeof(OrbisNpWebApiInitializeParameter)) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    if (!g_is_initialized) {
        g_is_initialized = true;
        s32 result = initializeLibrary();
        if (result < ORBIS_OK) {
            return result;
        }
    }

    s32 result = createLibraryContext(args->libhttp_ctx_id, args->pool_size, args->name, 2);
    if (result >= ORBIS_OK) {
        g_active_library_contexts++;
    }
    return result;
}

s32 sceNpWebApiIntRegisterServicePushEventCallback(
    s32 titleUserCtxId, s32 filterId, OrbisNpWebApiInternalServicePushEventCallback cbFunc,
    void* pUserArg) {
    if (cbFunc == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi, "called, titleUserCtxId = {:#x}, cbFunc = {}", titleUserCtxId,
             reinterpret_cast<void*>(cbFunc));
    return registerServicePushEventCallback(titleUserCtxId, filterId, nullptr, cbFunc, nullptr,
                                            pUserArg);
}

s32 sceNpWebApiIntRegisterServicePushEventCallbackA(
    s32 titleUserCtxId, s32 filterId, OrbisNpWebApiInternalServicePushEventCallbackA cbFunc,
    void* pUserArg) {
    if (cbFunc == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi, "called, titleUserCtxId = {:#x}, cbFunc = {}", titleUserCtxId,
             reinterpret_cast<void*>(cbFunc));
    return registerServicePushEventCallback(titleUserCtxId, filterId, nullptr, nullptr, cbFunc,
                                            pUserArg);
}

s32 sceNpWebApiReadData(s64 requestId, void* pData, u64 size) {
    LOG_INFO(Lib_NpWebApi, "called, requestId = {:#x}, pData = {}, size = {:#x}", requestId,
             fmt::ptr(pData), size);
    if (pData == nullptr || size == 0)
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;

    return readDataInternal(requestId, pData, size);
}

s32 sceNpWebApiRegisterExtdPushEventCallbackA(s32 titleUserCtxId, s32 filterId,
                                              OrbisNpWebApiExtdPushEventCallbackA cbFunc,
                                              void* pUserArg) {
    if (cbFunc == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi, "called, titleUserCtxId = {:#x}, cbFunc = {}", titleUserCtxId,
             reinterpret_cast<void*>(cbFunc));
    return registerExtdPushEventCallbackA(titleUserCtxId, filterId, cbFunc, pUserArg);
}

s32 sceNpWebApiSendMultipartRequest(s64 requestId, s32 partIndex, const void* pData, u64 dataSize) {
    if (partIndex <= 0 || pData == nullptr || dataSize == 0) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    LOG_INFO(Lib_NpWebApi,
             "called, requestId = {:#x}, "
             "partIndex = {:#x}, pData = {}, dataSize = {:#x}",
             requestId, partIndex, fmt::ptr(pData), dataSize);
    return sendRequest(requestId, partIndex, pData, dataSize, 0, nullptr);
}

s32 sceNpWebApiSendMultipartRequest2(s64 requestId, s32 partIndex, const void* pData, u64 dataSize,
                                     OrbisNpWebApiResponseInformationOption* pRespInfoOption) {
    if (partIndex <= 0 || pData == nullptr || dataSize == 0) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    LOG_INFO(Lib_NpWebApi,
             "called, requestId = {:#x}, "
             "partIndex = {:#x}, pData = {}, dataSize = {:#x}, pRespInfoOption = {}",
             requestId, partIndex, fmt::ptr(pData), dataSize, fmt::ptr(pRespInfoOption));
    return sendRequest(requestId, partIndex, pData, dataSize, 1, pRespInfoOption);
}

s32 sceNpWebApiSendRequest(s64 requestId, const void* pData, u64 dataSize) {
    LOG_INFO(Lib_NpWebApi, "called, requestId = {:#x}, pData = {}, dataSize = {:#x}", requestId,
             fmt::ptr(pData), dataSize);
    return sendRequest(requestId, 0, pData, dataSize, 0, nullptr);
}

s32 sceNpWebApiSendRequest2(s64 requestId, const void* pData, u64 dataSize,
                            OrbisNpWebApiResponseInformationOption* pRespInfoOption) {
    LOG_INFO(Lib_NpWebApi,
             "called, requestId = {:#x}, "
             "pData = {}, dataSize = {:#x}, pRespInfoOption = {}",
             requestId, fmt::ptr(pData), dataSize, fmt::ptr(pRespInfoOption));
    return sendRequest(requestId, 0, pData, dataSize, 1, pRespInfoOption);
}

s32 sceNpWebApiSetHandleTimeout(s32 libCtxId, s32 handleId, u32 timeout) {
    LOG_INFO(Lib_NpWebApi, "called, libCtxId = {:#x}, handleId = {:#x}, timeout = {} ms", libCtxId,
             handleId, timeout);
    return setHandleTimeout(libCtxId, handleId, timeout);
}

s32 sceNpWebApiSetMaxConnection(s32 libCtxId, s32 maxConnection) {
    LOG_ERROR(Lib_NpWebApi, "called (STUBBED), libCtxId = {:#x}, maxConnection = {}", libCtxId,
              maxConnection);
    return ORBIS_OK;
}

s32 sceNpWebApiSetMultipartContentType(s64 requestId, const char* pTypeName,
                                       const char* pBoundary) {
    LOG_INFO(Lib_NpWebApi, "called, requestId = {:#x}, pTypeName = '{}', pBoundary = '{}'",
             requestId, (pTypeName ? pTypeName : "null"), (pBoundary ? pBoundary : "null"));
    return setMultipartContentType(requestId, pTypeName, pBoundary);
}

s32 sceNpWebApiSetRequestTimeout(s64 requestId, u32 timeout) {
    LOG_INFO(Lib_NpWebApi, "called, requestId = {:#x}, timeout = {} ms", requestId, timeout);
    return setRequestTimeout(requestId, timeout);
}

s32 sceNpWebApiTerminate(s32 libCtxId) {
    LOG_INFO(Lib_NpWebApi, "called, libCtxId = {:#x}", libCtxId);
    s32 result = terminateContext(libCtxId);
    if (result != ORBIS_OK) {
        return result;
    }

    g_active_library_contexts--;
    if (g_active_library_contexts == 0) {
        g_is_initialized = false;
    }
    return ORBIS_OK;
}

s32 sceNpWebApiUnregisterExtdPushEventCallback(s32 titleUserCtxId, s32 callbackId) {
    LOG_INFO(Lib_NpWebApi, "called, titleUserCtxId = {:#x}, callbackId = {:#x}", titleUserCtxId,
             callbackId);
    return unregisterExtdPushEventCallback(titleUserCtxId, callbackId);
}

s32 sceNpWebApiVshInitialize(s32 libHttpCtxId, u64 poolSize) {
    LOG_INFO(Lib_NpWebApi, "called, libHttpCtxId = {:#x}, poolSize = {:#x} bytes", libHttpCtxId,
             poolSize);
    if (!g_is_initialized) {
        g_is_initialized = true;
        s32 result = initializeLibrary();
        if (result < ORBIS_OK) {
            return result;
        }
    }

    s32 result = createLibraryContext(libHttpCtxId, poolSize, nullptr, 4);
    if (result >= ORBIS_OK) {
        g_active_library_contexts++;
    }
    return result;
}

void RegisterHooks() {
    RegisterLibraryHooks();
    NpManager::RegisterNpCallback("npwebapi_push", DrainPushEvents);
};

} // namespace Libraries::Np::NpWebApi