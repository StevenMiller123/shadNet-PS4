// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <utility>
#include <vector>
#include <orbis/NpWebApi.h>
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpWebApi {

struct OrbisNpWebApiPushEventFilterParameter {
    OrbisNpWebApiPushEventDataType data_type;
};

struct OrbisNpWebApiServicePushEventFilterParameter {
    OrbisNpWebApiPushEventDataType data_type;
};

struct OrbisNpWebApiExtdPushEventExtdData {
    OrbisNpWebApiExtdPushEventExtdDataKey extdDataKey;
    char* pData;
    u64 dataLen;
};

struct OrbisNpWebApiHttpHeader {
    char* pName;
    char* pValue;
};

struct OrbisNpWebApiMultipartPartParameter {
    OrbisNpWebApiHttpHeader* pHeaders;
    u64 headerNum;
    u64 contentLength;
};

struct OrbisNpWebApiMemoryPoolStats {
    u64 poolSize;
    u64 maxInuseSize;
    u64 currentInuseSize;
    s32 reserved;
};

struct OrbisNpWebApiConnectionStats {
    u32 max;
    u32 used;
    u32 unused;
    u32 keepAlive;
    u64 reserved;
};

// Needs reversing
struct OrbisNpWebApiIntCreateRequestExtraArgs {
    void* unk_0;
    void* unk_1;
    void* unk_2;
};
using OrbisNpWebApiInternalServicePushEventCallback = PS4_SYSV_ABI void (*)();
using OrbisNpWebApiInternalServicePushEventCallbackA = PS4_SYSV_ABI void (*)();
using OrbisNpWebApiNotificationCallback = PS4_SYSV_ABI void (*)();

struct PushEventInput {
    s32 targetUserId = 0;
    std::string npServiceName;
    u32 npServiceLabel = 0;
    std::string dataType;
    std::string data;
    OrbisNpOnlineId fromOnlineId{};
    bool hasFrom = false;
    OrbisNpOnlineId toOnlineId{};
    bool hasTo = false;
    std::vector<std::pair<std::string, std::string>> extdData;
};

s32 sceNpWebApiCreateContext(s32 libCtxId, OrbisNpOnlineId* onlineId);
s32 sceNpWebApiCreatePushEventFilter(s32 libCtxId,
                                     const OrbisNpWebApiPushEventFilterParameter* pFilterParam,
                                     u64 filterParamNum);
s32 sceNpWebApiCreateServicePushEventFilter(
    s32 libCtxId, s32 handleId, const char* pNpServiceName, u32 npServiceLabel,
    const OrbisNpWebApiServicePushEventFilterParameter* pFilterParam, u64 filterParamNum);
s32 sceNpWebApiDeletePushEventFilter(s32 libCtxId, s32 filterId);
s32 sceNpWebApiDeleteServicePushEventFilter(s32 libCtxId, s32 filterId);
s32 sceNpWebApiRegisterExtdPushEventCallback(s32 titleUserCtxId, s32 filterId,
                                             OrbisNpWebApiExtdPushEventCallback cbFunc,
                                             void* pUserArg);
s32 sceNpWebApiRegisterNotificationCallback(s32 titleUserCtxId,
                                            OrbisNpWebApiNotificationCallback cbFunc,
                                            void* pUserArg);
s32 sceNpWebApiRegisterPushEventCallback(s32 titleUserCtxId, s32 filterId,
                                         OrbisNpWebApiPushEventCallback cbFunc, void* pUserArg);
s32 sceNpWebApiRegisterServicePushEventCallback(s32 titleUserCtxId, s32 filterId,
                                                OrbisNpWebApiServicePushEventCallback cbFunc,
                                                void* pUserArg);
s32 sceNpWebApiUnregisterNotificationCallback(s32 titleUserCtxId);
s32 sceNpWebApiUnregisterPushEventCallback(s32 titleUserCtxId, s32 callbackId);
s32 sceNpWebApiUnregisterServicePushEventCallback(s32 titleUserCtxId, s32 callbackId);
s32 sceNpWebApiAbortHandle(s32 libCtxId, s32 handleId);
s32 sceNpWebApiAbortRequest(s64 requestId);
s32 sceNpWebApiAddHttpRequestHeader(s64 requestId, const char* pFieldName, const char* pValue);
s32 sceNpWebApiAddMultipartPart(s64 requestId, const OrbisNpWebApiMultipartPartParameter* pParam,
                                s32* pIndex);
void sceNpWebApiCheckTimeout();
s32 sceNpWebApiClearAllUnusedConnection(s32 userCtxId, bool bRemainKeepAliveConnection);
s32 sceNpWebApiClearUnusedConnection(s32 userCtxId, const char* pApiGroup,
                                     bool bRemainKeepAliveConnection);
s32 sceNpWebApiCreateContextA(s32 libCtxId, s32 userId);
s32 sceNpWebApiCreateExtdPushEventFilter(
    s32 libCtxId, s32 handleId, const char* pNpServiceName, u32 npServiceLabel,
    const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam, u64 filterParamNum);
s32 sceNpWebApiCreateHandle(s32 libCtxId);
s32 sceNpWebApiCreateMultipartRequest(s32 titleUserCtxId, const char* pApiGroup, const char* pPath,
                                      OrbisNpWebApiHttpMethod method, s64* pRequestId);
s32 sceNpWebApiCreateRequest(s32 titleUserCtxId, const char* pApiGroup, const char* pPath,
                             OrbisNpWebApiHttpMethod method,
                             const OrbisNpWebApiContentParameter* pContentParameter,
                             s64* pRequestId);
s32 sceNpWebApiDeleteContext(s32 titleUserCtxId);
s32 sceNpWebApiDeleteExtdPushEventFilter(s32 libCtxId, s32 filterId);
s32 sceNpWebApiDeleteHandle(s32 libCtxId, s32 handleId);
s32 sceNpWebApiDeleteRequest(s64 requestId);
s32 sceNpWebApiGetConnectionStats(s32 userCtxId, const char* pApiGroup,
                                  OrbisNpWebApiConnectionStats* pStats);
s32 sceNpWebApiGetHttpResponseHeaderValue(s64 requestId, const char* pFieldName, char* pValue,
                                          u64 valueSize);
s32 sceNpWebApiGetHttpResponseHeaderValueLength(s64 requestId, const char* pFieldName,
                                                u64* pValueLength);
s32 sceNpWebApiGetHttpStatusCode(s64 requestId, s32* out_status_code);
s32 sceNpWebApiGetMemoryPoolStats(s32 libCtxId, OrbisNpWebApiMemoryPoolStats* pCurrentStat);
s32 sceNpWebApiInitialize(s32 libHttpCtxId, u64 poolSize);
s32 sceNpWebApiInitializeForPresence(s32 libHttpCtxId, u64 poolSize);
s32 sceNpWebApiIntCreateCtxIndExtdPushEventFilter(
    s32 libCtxId, s32 handleId, const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam,
    u64 filterParamNum);
s32 sceNpWebApiIntCreateRequest(s32 titleUserCtxId, const char* pApiGroup, const char* pPath,
                                OrbisNpWebApiHttpMethod method,
                                const OrbisNpWebApiContentParameter* pContentParameter,
                                const OrbisNpWebApiIntCreateRequestExtraArgs* pInternalArgs,
                                s64* pRequestId);
s32 sceNpWebApiIntCreateServicePushEventFilter(
    s32 libCtxId, s32 handleId, const char* pNpServiceName, u32 npServiceLabel,
    const OrbisNpWebApiServicePushEventFilterParameter* pFilterParam, u64 filterParamNum);
s32 sceNpWebApiIntInitialize(const OrbisNpWebApiInitializeParameter* args);
s32 sceNpWebApiIntRegisterServicePushEventCallback(
    s32 titleUserCtxId, s32 filterId, OrbisNpWebApiInternalServicePushEventCallback cbFunc,
    void* pUserArg);
s32 sceNpWebApiIntRegisterServicePushEventCallbackA(
    s32 titleUserCtxId, s32 filterId, OrbisNpWebApiInternalServicePushEventCallbackA cbFunc,
    void* pUserArg);
s32 sceNpWebApiReadData(s64 requestId, void* pData, u64 size);
s32 sceNpWebApiRegisterExtdPushEventCallbackA(s32 titleUserCtxId, s32 filterId,
                                              OrbisNpWebApiExtdPushEventCallbackA cbFunc,
                                              void* pUserArg);
s32 sceNpWebApiSendMultipartRequest(s64 requestId, s32 partIndex, const void* pData, u64 dataSize);
s32 sceNpWebApiSendMultipartRequest2(s64 requestId, s32 partIndex, const void* pData, u64 dataSize,
                                     OrbisNpWebApiResponseInformationOption* pRespInfoOption);
s32 sceNpWebApiSendRequest(s64 requestId, const void* pData, u64 dataSize);
s32 sceNpWebApiSendRequest2(s64 requestId, const void* pData, u64 dataSize,
                            OrbisNpWebApiResponseInformationOption* pRespInfoOption);
s32 sceNpWebApiSetHandleTimeout(s32 libCtxId, s32 handleId, u32 timeout);
s32 sceNpWebApiSetMaxConnection(s32 libCtxId, s32 maxConnection);
s32 sceNpWebApiSetMultipartContentType(s64 requestId, const char* pTypeName, const char* pBoundary);
s32 sceNpWebApiSetRequestTimeout(s64 requestId, u32 timeout);
s32 sceNpWebApiTerminate(s32 libCtxId);
s32 sceNpWebApiUnregisterExtdPushEventCallback(s32 titleUserCtxId, s32 callbackId);
s32 sceNpWebApiVshInitialize(s32 libHttpCtxId, u64 poolSize);

void EnqueuePushEvent(const PushEventInput& ev);
void RegisterHooks();
} // namespace Libraries::Np::NpWebApi