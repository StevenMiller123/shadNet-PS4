// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <magic_enum/magic_enum.hpp>
#include <orbis/NpWebApi.h>
#include <orbis/UserService.h>
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_web_api/np_web_api.h"
#include "core/libraries/np/np_web_api/np_web_api_internal.h"

extern "C" {
void sceNpWebApiVshInitialize();
void sceNpWebApiInitializeForPresence();
void sceNpWebApiIntCreateRequest();
void sceNpWebApiIntRegisterServicePushEventCallback();
void sceNpWebApiIntRegisterServicePushEventCallbackA();
}

HOOK_INIT(sceNpWebApiCreateContext);
static s32 sceNpWebApiCreateContext_hook(s32 libCtxId, OrbisNpOnlineId* onlineId) {
    return Libraries::Np::NpWebApi::sceNpWebApiCreateContext(libCtxId, onlineId);
}

HOOK_INIT(sceNpWebApiCreatePushEventFilter);
static s32 sceNpWebApiCreatePushEventFilter_hook(
    s32 libCtxId,
    const Libraries::Np::NpWebApi::OrbisNpWebApiPushEventFilterParameter* pFilterParam,
    u64 filterParamNum) {
    return Libraries::Np::NpWebApi::sceNpWebApiCreatePushEventFilter(libCtxId, pFilterParam,
                                                                     filterParamNum);
}

HOOK_INIT(sceNpWebApiCreateServicePushEventFilter);
static s32 sceNpWebApiCreateServicePushEventFilter_hook(
    s32 libCtxId, s32 handleId, const char* pNpServiceName, u32 npServiceLabel,
    const Libraries::Np::NpWebApi::OrbisNpWebApiServicePushEventFilterParameter* pFilterParam,
    u64 filterParamNum) {
    return Libraries::Np::NpWebApi::sceNpWebApiCreateServicePushEventFilter(
        libCtxId, handleId, pNpServiceName, npServiceLabel, pFilterParam, filterParamNum);
}

HOOK_INIT(sceNpWebApiDeletePushEventFilter);
static s32 sceNpWebApiDeletePushEventFilter_hook(s32 libCtxId, s32 filterId) {
    return Libraries::Np::NpWebApi::sceNpWebApiDeletePushEventFilter(libCtxId, filterId);
}

HOOK_INIT(sceNpWebApiDeleteServicePushEventFilter);
static s32 sceNpWebApiDeleteServicePushEventFilter_hook(s32 libCtxId, s32 filterId) {
    return Libraries::Np::NpWebApi::sceNpWebApiDeleteServicePushEventFilter(libCtxId, filterId);
}

HOOK_INIT(sceNpWebApiRegisterExtdPushEventCallback);
static s32 sceNpWebApiRegisterExtdPushEventCallback_hook(s32 titleUserCtxId, s32 filterId,
                                                         OrbisNpWebApiExtdPushEventCallback cbFunc,
                                                         void* pUserArg) {
    return Libraries::Np::NpWebApi::sceNpWebApiRegisterExtdPushEventCallback(
        titleUserCtxId, filterId, cbFunc, pUserArg);
}

HOOK_INIT(sceNpWebApiRegisterNotificationCallback);
static s32 sceNpWebApiRegisterNotificationCallback_hook(
    s32 titleUserCtxId, Libraries::Np::NpWebApi::OrbisNpWebApiNotificationCallback cbFunc,
    void* pUserArg) {
    return Libraries::Np::NpWebApi::sceNpWebApiRegisterNotificationCallback(titleUserCtxId, cbFunc,
                                                                            pUserArg);
}

HOOK_INIT(sceNpWebApiRegisterPushEventCallback);
static s32 sceNpWebApiRegisterPushEventCallback_hook(s32 titleUserCtxId, s32 filterId,
                                                     OrbisNpWebApiPushEventCallback cbFunc,
                                                     void* pUserArg) {
    return Libraries::Np::NpWebApi::sceNpWebApiRegisterPushEventCallback(titleUserCtxId, filterId,
                                                                         cbFunc, pUserArg);
}

HOOK_INIT(sceNpWebApiRegisterServicePushEventCallback);
static s32 sceNpWebApiRegisterServicePushEventCallback_hook(
    s32 titleUserCtxId, s32 filterId, OrbisNpWebApiServicePushEventCallback cbFunc,
    void* pUserArg) {
    return Libraries::Np::NpWebApi::sceNpWebApiRegisterServicePushEventCallback(
        titleUserCtxId, filterId, cbFunc, pUserArg);
}

HOOK_INIT(sceNpWebApiUnregisterNotificationCallback);
static s32 sceNpWebApiUnregisterNotificationCallback_hook(s32 titleUserCtxId) {
    return Libraries::Np::NpWebApi::sceNpWebApiUnregisterNotificationCallback(titleUserCtxId);
}

HOOK_INIT(sceNpWebApiUnregisterPushEventCallback);
static s32 sceNpWebApiUnregisterPushEventCallback_hook(s32 titleUserCtxId, s32 callbackId) {
    return Libraries::Np::NpWebApi::sceNpWebApiUnregisterPushEventCallback(titleUserCtxId,
                                                                           callbackId);
}

HOOK_INIT(sceNpWebApiUnregisterServicePushEventCallback);
static s32 sceNpWebApiUnregisterServicePushEventCallback_hook(s32 titleUserCtxId, s32 callbackId) {
    return Libraries::Np::NpWebApi::sceNpWebApiUnregisterServicePushEventCallback(titleUserCtxId,
                                                                                  callbackId);
}

HOOK_INIT(sceNpWebApiAbortHandle);
static s32 sceNpWebApiAbortHandle_hook(s32 libCtxId, s32 handleId) {
    return Libraries::Np::NpWebApi::sceNpWebApiAbortHandle(libCtxId, handleId);
}

HOOK_INIT(sceNpWebApiAbortRequest);
static s32 sceNpWebApiAbortRequest_hook(s64 requestId) {
    return Libraries::Np::NpWebApi::sceNpWebApiAbortRequest(requestId);
}

HOOK_INIT(sceNpWebApiAddHttpRequestHeader);
static s32 sceNpWebApiAddHttpRequestHeader_hook(s64 requestId, const char* pFieldName,
                                                const char* pValue) {
    return Libraries::Np::NpWebApi::sceNpWebApiAddHttpRequestHeader(requestId, pFieldName, pValue);
}

HOOK_INIT(sceNpWebApiAddMultipartPart);
static s32 sceNpWebApiAddMultipartPart_hook(
    s64 requestId, const Libraries::Np::NpWebApi::OrbisNpWebApiMultipartPartParameter* pParam,
    s32* pIndex) {
    return Libraries::Np::NpWebApi::sceNpWebApiAddMultipartPart(requestId, pParam, pIndex);
}

HOOK_INIT(sceNpWebApiCheckTimeout);
static void sceNpWebApiCheckTimeout_hook() {
    return Libraries::Np::NpWebApi::sceNpWebApiCheckTimeout();
}

HOOK_INIT(sceNpWebApiClearAllUnusedConnection);
static s32 sceNpWebApiClearAllUnusedConnection_hook(s32 userCtxId,
                                                    bool bRemainKeepAliveConnection) {
    return Libraries::Np::NpWebApi::sceNpWebApiClearAllUnusedConnection(userCtxId,
                                                                        bRemainKeepAliveConnection);
}

HOOK_INIT(sceNpWebApiClearUnusedConnection);
static s32 sceNpWebApiClearUnusedConnection_hook(s32 userCtxId, const char* pApiGroup,
                                                 bool bRemainKeepAliveConnection) {
    return Libraries::Np::NpWebApi::sceNpWebApiClearUnusedConnection(userCtxId, pApiGroup,
                                                                     bRemainKeepAliveConnection);
}

HOOK_INIT(sceNpWebApiCreateContextA);
static s32 sceNpWebApiCreateContextA_hook(s32 libCtxId, s32 userId) {
    return Libraries::Np::NpWebApi::sceNpWebApiCreateContextA(libCtxId, userId);
}

HOOK_INIT(sceNpWebApiCreateExtdPushEventFilter);
static s32 sceNpWebApiCreateExtdPushEventFilter_hook(
    s32 libCtxId, s32 handleId, const char* pNpServiceName, u32 npServiceLabel,
    const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam, u64 filterParamNum) {
    return Libraries::Np::NpWebApi::sceNpWebApiCreateExtdPushEventFilter(
        libCtxId, handleId, pNpServiceName, npServiceLabel, pFilterParam, filterParamNum);
}

HOOK_INIT(sceNpWebApiCreateHandle);
static s32 sceNpWebApiCreateHandle_hook(s32 libCtxId) {
    return Libraries::Np::NpWebApi::sceNpWebApiCreateHandle(libCtxId);
}

HOOK_INIT(sceNpWebApiCreateMultipartRequest);
static s32 sceNpWebApiCreateMultipartRequest_hook(s32 titleUserCtxId, const char* pApiGroup,
                                                  const char* pPath, OrbisNpWebApiHttpMethod method,
                                                  s64* pRequestId) {
    return Libraries::Np::NpWebApi::sceNpWebApiCreateMultipartRequest(titleUserCtxId, pApiGroup,
                                                                      pPath, method, pRequestId);
}

HOOK_INIT(sceNpWebApiCreateRequest);
static s32 sceNpWebApiCreateRequest_hook(s32 titleUserCtxId, const char* pApiGroup,
                                         const char* pPath, OrbisNpWebApiHttpMethod method,
                                         const OrbisNpWebApiContentParameter* pContentParameter,
                                         s64* pRequestId) {
    return Libraries::Np::NpWebApi::sceNpWebApiCreateRequest(titleUserCtxId, pApiGroup, pPath,
                                                             method, pContentParameter, pRequestId);
}

HOOK_INIT(sceNpWebApiDeleteContext);
static s32 sceNpWebApiDeleteContext_hook(s32 titleUserCtxId) {
    return Libraries::Np::NpWebApi::sceNpWebApiDeleteContext(titleUserCtxId);
}

HOOK_INIT(sceNpWebApiDeleteExtdPushEventFilter);
static s32 sceNpWebApiDeleteExtdPushEventFilter_hook(s32 libCtxId, s32 filterId) {
    return Libraries::Np::NpWebApi::sceNpWebApiDeleteExtdPushEventFilter(libCtxId, filterId);
}

HOOK_INIT(sceNpWebApiDeleteHandle);
static s32 sceNpWebApiDeleteHandle_hook(s32 libCtxId, s32 handleId) {
    return Libraries::Np::NpWebApi::sceNpWebApiDeleteHandle(libCtxId, handleId);
}

HOOK_INIT(sceNpWebApiDeleteRequest);
static s32 sceNpWebApiDeleteRequest_hook(s64 requestId) {
    return Libraries::Np::NpWebApi::sceNpWebApiDeleteRequest(requestId);
}

HOOK_INIT(sceNpWebApiGetConnectionStats);
static s32 sceNpWebApiGetConnectionStats_hook(
    s32 userCtxId, const char* pApiGroup,
    Libraries::Np::NpWebApi::OrbisNpWebApiConnectionStats* pStats) {
    return Libraries::Np::NpWebApi::sceNpWebApiGetConnectionStats(userCtxId, pApiGroup, pStats);
}

HOOK_INIT(sceNpWebApiGetHttpResponseHeaderValue);
static s32 sceNpWebApiGetHttpResponseHeaderValue_hook(s64 requestId, const char* pFieldName,
                                                      char* pValue, u64 valueSize) {
    return Libraries::Np::NpWebApi::sceNpWebApiGetHttpResponseHeaderValue(requestId, pFieldName,
                                                                          pValue, valueSize);
}

HOOK_INIT(sceNpWebApiGetHttpResponseHeaderValueLength);
static s32 sceNpWebApiGetHttpResponseHeaderValueLength_hook(s64 requestId, const char* pFieldName,
                                                            u64* pValueLength) {
    return Libraries::Np::NpWebApi::sceNpWebApiGetHttpResponseHeaderValueLength(
        requestId, pFieldName, pValueLength);
}

HOOK_INIT(sceNpWebApiGetHttpStatusCode);
static s32 sceNpWebApiGetHttpStatusCode_hook(s64 requestId, s32* out_status_code) {
    return Libraries::Np::NpWebApi::sceNpWebApiGetHttpStatusCode(requestId, out_status_code);
}

HOOK_INIT(sceNpWebApiGetMemoryPoolStats);
static s32 sceNpWebApiGetMemoryPoolStats_hook(
    s32 libCtxId, Libraries::Np::NpWebApi::OrbisNpWebApiMemoryPoolStats* pCurrentStat) {
    return Libraries::Np::NpWebApi::sceNpWebApiGetMemoryPoolStats(libCtxId, pCurrentStat);
}

HOOK_INIT(sceNpWebApiInitialize);
static s32 sceNpWebApiInitialize_hook(s32 libHttpCtxId, u64 poolSize) {
    return Libraries::Np::NpWebApi::sceNpWebApiInitialize(libHttpCtxId, poolSize);
}

HOOK_INIT(sceNpWebApiInitializeForPresence);
static s32 sceNpWebApiInitializeForPresence_hook(s32 libHttpCtxId, u64 poolSize) {
    return Libraries::Np::NpWebApi::sceNpWebApiInitializeForPresence(libHttpCtxId, poolSize);
}

HOOK_INIT(sceNpWebApiIntCreateCtxIndExtdPushEventFilter);
static s32 sceNpWebApiIntCreateCtxIndExtdPushEventFilter_hook(
    s32 libCtxId, s32 handleId, const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam,
    u64 filterParamNum) {
    return Libraries::Np::NpWebApi::sceNpWebApiIntCreateCtxIndExtdPushEventFilter(
        libCtxId, handleId, pFilterParam, filterParamNum);
}

HOOK_INIT(sceNpWebApiIntCreateRequest);
static s32 sceNpWebApiIntCreateRequest_hook(
    s32 titleUserCtxId, const char* pApiGroup, const char* pPath, OrbisNpWebApiHttpMethod method,
    const OrbisNpWebApiContentParameter* pContentParameter,
    const Libraries::Np::NpWebApi::OrbisNpWebApiIntCreateRequestExtraArgs* pInternalArgs,
    s64* pRequestId) {
    return Libraries::Np::NpWebApi::sceNpWebApiIntCreateRequest(
        titleUserCtxId, pApiGroup, pPath, method, pContentParameter, pInternalArgs, pRequestId);
}

HOOK_INIT(sceNpWebApiIntCreateServicePushEventFilter);
static s32 sceNpWebApiIntCreateServicePushEventFilter_hook(
    s32 libCtxId, s32 handleId, const char* pNpServiceName, u32 npServiceLabel,
    const Libraries::Np::NpWebApi::OrbisNpWebApiServicePushEventFilterParameter* pFilterParam,
    u64 filterParamNum) {
    return Libraries::Np::NpWebApi::sceNpWebApiIntCreateServicePushEventFilter(
        libCtxId, handleId, pNpServiceName, npServiceLabel, pFilterParam, filterParamNum);
}

HOOK_INIT(sceNpWebApiIntInitialize);
static s32 sceNpWebApiIntInitialize_hook(const OrbisNpWebApiInitializeParameter* args) {
    return Libraries::Np::NpWebApi::sceNpWebApiIntInitialize(args);
}

HOOK_INIT(sceNpWebApiIntRegisterServicePushEventCallback);
static s32 sceNpWebApiIntRegisterServicePushEventCallback_hook(
    s32 titleUserCtxId, s32 filterId,
    Libraries::Np::NpWebApi::OrbisNpWebApiInternalServicePushEventCallback cbFunc, void* pUserArg) {
    return Libraries::Np::NpWebApi::sceNpWebApiIntRegisterServicePushEventCallback(
        titleUserCtxId, filterId, cbFunc, pUserArg);
}

HOOK_INIT(sceNpWebApiIntRegisterServicePushEventCallbackA);
static s32 sceNpWebApiIntRegisterServicePushEventCallbackA_hook(
    s32 titleUserCtxId, s32 filterId,
    Libraries::Np::NpWebApi::OrbisNpWebApiInternalServicePushEventCallbackA cbFunc,
    void* pUserArg) {
    return Libraries::Np::NpWebApi::sceNpWebApiIntRegisterServicePushEventCallbackA(
        titleUserCtxId, filterId, cbFunc, pUserArg);
}

HOOK_INIT(sceNpWebApiReadData);
static s32 sceNpWebApiReadData_hook(s64 requestId, void* pData, u64 size) {
    return Libraries::Np::NpWebApi::sceNpWebApiReadData(requestId, pData, size);
}

HOOK_INIT(sceNpWebApiRegisterExtdPushEventCallbackA);
static s32 sceNpWebApiRegisterExtdPushEventCallbackA_hook(
    s32 titleUserCtxId, s32 filterId, OrbisNpWebApiExtdPushEventCallbackA cbFunc, void* pUserArg) {
    return Libraries::Np::NpWebApi::sceNpWebApiRegisterExtdPushEventCallbackA(
        titleUserCtxId, filterId, cbFunc, pUserArg);
}

HOOK_INIT(sceNpWebApiSendMultipartRequest);
static s32 sceNpWebApiSendMultipartRequest_hook(s64 requestId, s32 partIndex, const void* pData,
                                                u64 dataSize) {
    return Libraries::Np::NpWebApi::sceNpWebApiSendMultipartRequest(requestId, partIndex, pData,
                                                                    dataSize);
}

HOOK_INIT(sceNpWebApiSendMultipartRequest2);
static s32 sceNpWebApiSendMultipartRequest2_hook(
    s64 requestId, s32 partIndex, const void* pData, u64 dataSize,
    OrbisNpWebApiResponseInformationOption* pRespInfoOption) {
    return Libraries::Np::NpWebApi::sceNpWebApiSendMultipartRequest2(requestId, partIndex, pData,
                                                                     dataSize, pRespInfoOption);
}

HOOK_INIT(sceNpWebApiSendRequest);
static s32 sceNpWebApiSendRequest_hook(s64 requestId, const void* pData, u64 dataSize) {
    return Libraries::Np::NpWebApi::sceNpWebApiSendRequest(requestId, pData, dataSize);
}

HOOK_INIT(sceNpWebApiSendRequest2);
static s32 sceNpWebApiSendRequest2_hook(s64 requestId, const void* pData, u64 dataSize,
                                        OrbisNpWebApiResponseInformationOption* pRespInfoOption) {
    return Libraries::Np::NpWebApi::sceNpWebApiSendRequest2(requestId, pData, dataSize,
                                                            pRespInfoOption);
}

HOOK_INIT(sceNpWebApiSetHandleTimeout);
static s32 sceNpWebApiSetHandleTimeout_hook(s32 libCtxId, s32 handleId, u32 timeout) {
    return Libraries::Np::NpWebApi::sceNpWebApiSetHandleTimeout(libCtxId, handleId, timeout);
}

HOOK_INIT(sceNpWebApiSetMaxConnection);
static s32 sceNpWebApiSetMaxConnection_hook(s32 libCtxId, s32 maxConnection) {
    return Libraries::Np::NpWebApi::sceNpWebApiSetMaxConnection(libCtxId, maxConnection);
}

HOOK_INIT(sceNpWebApiSetMultipartContentType);
static s32 sceNpWebApiSetMultipartContentType_hook(s64 requestId, const char* pTypeName,
                                                   const char* pBoundary) {
    return Libraries::Np::NpWebApi::sceNpWebApiSetMultipartContentType(requestId, pTypeName,
                                                                       pBoundary);
}

HOOK_INIT(sceNpWebApiSetRequestTimeout);
static s32 sceNpWebApiSetRequestTimeout_hook(s64 requestId, u32 timeout) {
    return Libraries::Np::NpWebApi::sceNpWebApiSetRequestTimeout(requestId, timeout);
}

HOOK_INIT(sceNpWebApiTerminate);
static s32 sceNpWebApiTerminate_hook(s32 libCtxId) {
    return Libraries::Np::NpWebApi::sceNpWebApiTerminate(libCtxId);
}

HOOK_INIT(sceNpWebApiUnregisterExtdPushEventCallback);
static s32 sceNpWebApiUnregisterExtdPushEventCallback_hook(s32 titleUserCtxId, s32 callbackId) {
    return Libraries::Np::NpWebApi::sceNpWebApiUnregisterExtdPushEventCallback(titleUserCtxId,
                                                                               callbackId);
}

HOOK_INIT(sceNpWebApiVshInitialize);
static s32 sceNpWebApiVshInitialize_hook(s32 libHttpCtxId, u64 poolSize) {
    return Libraries::Np::NpWebApi::sceNpWebApiVshInitialize(libHttpCtxId, poolSize);
}

void RegisterNpWebApiHooks() {
    HOOK(sceNpWebApiCreateContext);
    HOOK(sceNpWebApiCreatePushEventFilter);
    HOOK(sceNpWebApiCreateServicePushEventFilter);
    HOOK(sceNpWebApiDeletePushEventFilter);
    HOOK(sceNpWebApiDeleteServicePushEventFilter);
    HOOK(sceNpWebApiRegisterExtdPushEventCallback);
    HOOK(sceNpWebApiRegisterNotificationCallback);
    HOOK(sceNpWebApiRegisterPushEventCallback);
    HOOK(sceNpWebApiRegisterServicePushEventCallback);
    HOOK(sceNpWebApiUnregisterNotificationCallback);
    HOOK(sceNpWebApiUnregisterPushEventCallback);
    HOOK(sceNpWebApiUnregisterServicePushEventCallback);
    HOOK(sceNpWebApiAbortHandle);
    HOOK(sceNpWebApiAbortRequest);
    HOOK(sceNpWebApiAddHttpRequestHeader);
    HOOK(sceNpWebApiAddMultipartPart);
    HOOK(sceNpWebApiCheckTimeout);
    HOOK(sceNpWebApiClearAllUnusedConnection);
    HOOK(sceNpWebApiClearUnusedConnection);
    HOOK(sceNpWebApiCreateContext);
    HOOK(sceNpWebApiCreateContextA);
    HOOK(sceNpWebApiCreateExtdPushEventFilter);
    HOOK(sceNpWebApiCreateHandle);
    HOOK(sceNpWebApiCreateMultipartRequest);
    HOOK(sceNpWebApiCreatePushEventFilter);
    HOOK(sceNpWebApiCreateRequest);
    HOOK(sceNpWebApiCreateServicePushEventFilter);
    HOOK(sceNpWebApiDeleteContext);
    HOOK(sceNpWebApiDeleteExtdPushEventFilter);
    HOOK(sceNpWebApiDeleteHandle);
    HOOK(sceNpWebApiDeletePushEventFilter);
    HOOK(sceNpWebApiDeleteRequest);
    HOOK(sceNpWebApiDeleteServicePushEventFilter);
    HOOK(sceNpWebApiGetConnectionStats);
    HOOK(sceNpWebApiGetHttpResponseHeaderValue);
    HOOK(sceNpWebApiGetHttpResponseHeaderValueLength);
    HOOK(sceNpWebApiGetHttpStatusCode);
    HOOK(sceNpWebApiGetMemoryPoolStats);
    HOOK(sceNpWebApiInitialize);
    HOOK(sceNpWebApiInitializeForPresence);
    HOOK(sceNpWebApiIntCreateCtxIndExtdPushEventFilter);
    HOOK(sceNpWebApiIntCreateRequest);
    HOOK(sceNpWebApiIntCreateServicePushEventFilter);
    HOOK(sceNpWebApiIntInitialize);
    HOOK(sceNpWebApiIntRegisterServicePushEventCallback);
    HOOK(sceNpWebApiIntRegisterServicePushEventCallbackA);
    HOOK(sceNpWebApiReadData);
    HOOK(sceNpWebApiRegisterExtdPushEventCallback);
    HOOK(sceNpWebApiRegisterExtdPushEventCallbackA);
    HOOK(sceNpWebApiRegisterNotificationCallback);
    HOOK(sceNpWebApiRegisterPushEventCallback);
    HOOK(sceNpWebApiRegisterServicePushEventCallback);
    HOOK(sceNpWebApiSendMultipartRequest);
    HOOK(sceNpWebApiSendMultipartRequest2);
    HOOK(sceNpWebApiSendRequest);
    HOOK(sceNpWebApiSendRequest2);
    HOOK(sceNpWebApiSetHandleTimeout);
    HOOK(sceNpWebApiSetMaxConnection);
    HOOK(sceNpWebApiSetMultipartContentType);
    HOOK(sceNpWebApiSetRequestTimeout);
    HOOK(sceNpWebApiTerminate);
    HOOK(sceNpWebApiUnregisterExtdPushEventCallback);
    HOOK(sceNpWebApiUnregisterNotificationCallback);
    HOOK(sceNpWebApiUnregisterPushEventCallback);
    HOOK(sceNpWebApiUnregisterServicePushEventCallback);
    HOOK(sceNpWebApiVshInitialize);
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
    RegisterNpWebApiHooks();
    Libraries::Np::NpManager::RegisterNpCallback("npwebapi_push", DrainPushEvents);
};

} // namespace Libraries::Np::NpWebApi