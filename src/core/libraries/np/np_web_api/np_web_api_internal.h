// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include <absl/container/flat_hash_map.h>
#include "common/logging/log.h"
#include "core/libraries/np/np_web_api/np_web_api.h"

namespace Libraries::Np::NpWebApi {

// Structs reference each other, so declare them before their contents.
struct OrbisNpWebApiContext;
struct OrbisNpWebApiUserContext;
struct OrbisNpWebApiRequest;
struct OrbisNpWebApiHandle;
struct OrbisNpWebApiTimerHandle;
struct OrbisNpWebApiPushEventFilter;
struct OrbisNpWebApiServicePushEventFilter;
struct OrbisNpWebApiExtendedPushEventFilter;
struct OrbisNpWebApiRegisteredPushEventCallback;
struct OrbisNpWebApiRegisteredServicePushEventCallback;
struct OrbisNpWebApiRegisteredExtendedPushEventCallback;

struct OrbisNpWebApiContext {
    s32 type;
    s32 userCount;
    s32 libCtxId;
    s32 libHttpCtxId;
    std::recursive_mutex contextLock;
    absl::flat_hash_map<s32, OrbisNpWebApiUserContext*> userContexts;
    absl::flat_hash_map<s32, OrbisNpWebApiHandle*> handles;
    absl::flat_hash_map<s32, OrbisNpWebApiTimerHandle*> timerHandles;
    absl::flat_hash_map<s32, OrbisNpWebApiPushEventFilter*> pushEventFilters;
    absl::flat_hash_map<s32, OrbisNpWebApiServicePushEventFilter*> servicePushEventFilters;
    absl::flat_hash_map<s32, OrbisNpWebApiExtendedPushEventFilter*> extendedPushEventFilters;
    std::string name;
    bool terminated;
};

struct OrbisNpWebApiUserContext {
    OrbisNpWebApiContext* parentContext;
    s32 userCount;
    s32 userCtxId;
    s32 userId;
    absl::flat_hash_map<s64, OrbisNpWebApiRequest*> requests;
    absl::flat_hash_map<s32, OrbisNpWebApiRegisteredPushEventCallback*> pushEventCallbacks;
    absl::flat_hash_map<s32, OrbisNpWebApiRegisteredServicePushEventCallback*>
        servicePushEventCallbacks;
    absl::flat_hash_map<s32, OrbisNpWebApiRegisteredExtendedPushEventCallback*>
        extendedPushEventCallbacks;
    bool deleted;
    OrbisNpWebApiNotificationCallback notificationCallbackFunction;
    void* pNotificationCallbackUserArgs;
};

struct OrbisNpWebApiRequest {
    OrbisNpWebApiContext* parentContext;
    s32 userCount;
    s64 requestId;
    std::string userApiGroup;
    std::string userPath;
    OrbisNpWebApiHttpMethod userMethod;
    u64 userContentLength;
    std::string userContentType;
    std::vector<std::pair<std::string, std::string>> userHeaders;
    bool multipart;
    struct MultipartPart {
        std::string rawHeaders;
        u64 contentLength = 0;
        std::string data;
        u64 sentSize = 0;
    };
    std::string multipartContentType;
    std::string multipartBoundary;
    std::vector<MultipartPart> multipartParts;
    bool aborted;
    bool sent;
    u32 requestTimeout;
    u64 requestEndTime;
    bool timedOut;
    u8 requestState;
    u64 remainingData;
    u32 readOffset;
    char data[64];
    s32 http_connection_id = 0;
    s32 http_request_id = 0;
    s32 http_template_id = 0;
};

struct OrbisNpWebApiHandle {
    s32 handleId;
    bool aborted;
    bool deleted;
    s32 userCount;
};

struct OrbisNpWebApiTimerHandle {
    s32 handleId;
    u32 handleTimeout;
    u64 handleEndTime;
    bool timedOut;
};

struct OrbisNpWebApiPushEventFilter {
    s32 filterId;
    std::vector<OrbisNpWebApiPushEventFilterParameter> filterParams;
    OrbisNpWebApiContext* parentContext;
};

struct OrbisNpWebApiServicePushEventFilter {
    s32 filterId;
    bool internal;
    std::vector<OrbisNpWebApiServicePushEventFilterParameter> filterParams;
    std::string npServiceName;
    u32 npServiceLabel;
    OrbisNpWebApiContext* parentContext;
};

struct OrbisNpWebApiExtendedPushEventFilter {
    s32 filterId;
    bool internal;
    std::vector<OrbisNpWebApiExtdPushEventFilterParameter> filterParams;
    std::string npServiceName;
    u32 npServiceLabel;
    OrbisNpWebApiContext* parentContext;
};

struct OrbisNpWebApiRegisteredPushEventCallback {
    s32 callbackId;
    s32 filterId;
    OrbisNpWebApiPushEventCallback cbFunc;
    void* pUserArg;
};

struct OrbisNpWebApiRegisteredServicePushEventCallback {
    s32 callbackId;
    s32 filterId;
    OrbisNpWebApiServicePushEventCallback cbFunc;
    OrbisNpWebApiInternalServicePushEventCallback internalCbFunc;
    // Note: real struct stores both internal callbacks in one field
    OrbisNpWebApiInternalServicePushEventCallbackA internalCbFuncA;
    void* pUserArg;
};

struct OrbisNpWebApiRegisteredExtendedPushEventCallback {
    s32 callbackId;
    s32 filterId;
    OrbisNpWebApiExtdPushEventCallback cbFunc;
    // Note: real struct stores both callbacks in one field
    OrbisNpWebApiExtdPushEventCallbackA cbFuncA;
    void* pUserArg;
};

// General functions
s32 initializeLibrary();
s32 getCompiledSdkVersion();

// Library context functions
s32 createLibraryContext(s32 libHttpCtxId, u64 poolSize, const char* name, s32 type);
OrbisNpWebApiContext* findAndValidateContext(s32 libCtxId, s32 flag = 0);
void releaseContext(OrbisNpWebApiContext* context);
bool isContextTerminated(OrbisNpWebApiContext* context);
bool isContextBusy(OrbisNpWebApiContext* context);
bool areContextHandlesBusy(OrbisNpWebApiContext* context);
void lockContext(OrbisNpWebApiContext* context);
void unlockContext(OrbisNpWebApiContext* context);
void markContextAsTerminated(OrbisNpWebApiContext* context);
void checkContextTimeout(OrbisNpWebApiContext* context);
void checkTimeout();
s32 deleteContext(s32 libCtxId);
s32 terminateContext(s32 libCtxId);

// User context functions
OrbisNpWebApiUserContext* findUserContextByUserId(OrbisNpWebApiContext* context, s32 userId);
OrbisNpWebApiUserContext* findUserContext(OrbisNpWebApiContext* context, s32 userCtxId);
s32 createUserContextWithOnlineId(s32 libCtxId, OrbisNpOnlineId* onlineId);
s32 createUserContext(s32 libCtxId, s32 userId);
s32 registerNotificationCallback(s32 titleUserCtxId, OrbisNpWebApiNotificationCallback cbFunc,
                                 void* pUserArg);
s32 unregisterNotificationCallback(s32 titleUserCtxId);
bool isUserContextBusy(OrbisNpWebApiUserContext* userContext);
bool areUserContextRequestsBusy(OrbisNpWebApiUserContext* userContext);
void releaseUserContext(OrbisNpWebApiUserContext* userContext);
void checkUserContextTimeout(OrbisNpWebApiUserContext* userContext);
s32 deleteUserContext(s32 userCtxId);

// Request functions
s32 createRequest(s32 titleUserCtxId, const char* pApiGroup, const char* pPath,
                  OrbisNpWebApiHttpMethod method,
                  const OrbisNpWebApiContentParameter* pContentParameter,
                  const OrbisNpWebApiIntCreateRequestExtraArgs* pInternalArgs, s64* pRequestId,
                  bool isMultipart);
OrbisNpWebApiRequest* findRequest(OrbisNpWebApiUserContext* userContext, s64 requestId);
OrbisNpWebApiRequest* findRequestAndMarkBusy(OrbisNpWebApiUserContext* userContext, s64 requestId);
bool isRequestBusy(OrbisNpWebApiRequest* request);
s32 setRequestTimeout(s64 requestId, u32 timeout);
void startRequestTimer(OrbisNpWebApiRequest* request);
void checkRequestTimeout(OrbisNpWebApiRequest* request);
s32 sendRequest(s64 requestId, s32 partIndex, const void* data, u64 dataSize, s8 flag,
                OrbisNpWebApiResponseInformationOption* pResponseInformationOption);
s32 setMultipartContentType(s64 requestId, const char* pTypeName, const char* pBoundary);
s32 addMultipartPart(s64 requestId, const OrbisNpWebApiMultipartPartParameter* pParam, s32* pIndex);
s32 abortRequestInternal(OrbisNpWebApiContext* context, OrbisNpWebApiUserContext* userContext,
                         OrbisNpWebApiRequest* request);
s32 abortRequest(s64 requestId);
void releaseRequest(OrbisNpWebApiRequest* request);
s32 deleteRequest(s64 requestId);

s32 addHttpRequestHeaderInternal(s64 requestId, const char* pFieldName, const char* pValue);
s32 getHttpResponseHeaderValueInternal(s64 requestId, const char* pFieldName, char* pValue,
                                       u64 valueSize, u64* pValueLength);
s32 getLastWebApiError();

void DrainPushEvents();

// Handle functions
s32 createHandleInternal(OrbisNpWebApiContext* context);
s32 createHandle(s32 libCtxId);
s32 setHandleTimeoutInternal(OrbisNpWebApiContext* context, s32 handleId, u32 timeout);
s32 setHandleTimeout(s32 libCtxId, s32 handleId, u32 timeout);
void startHandleTimer(OrbisNpWebApiContext* context, s32 handleId);
void releaseHandle(OrbisNpWebApiContext* context, OrbisNpWebApiHandle* handle);
s32 getHandle(OrbisNpWebApiContext* context, s32 handleId, OrbisNpWebApiHandle** handleOut);
s32 abortHandle(s32 libCtxId, s32 handleId);
s32 deleteHandleInternal(OrbisNpWebApiContext* context, s32 handleId);
s32 deleteHandle(s32 libCtxId, s32 handleId);

// Push event filter functions
s32 createPushEventFilterInternal(OrbisNpWebApiContext* context,
                                  const OrbisNpWebApiPushEventFilterParameter* pFilterParam,
                                  u64 filterParamNum);
s32 createPushEventFilter(s32 libCtxId, const OrbisNpWebApiPushEventFilterParameter* pFilterParam,
                          u64 filterParamNum);
s32 deletePushEventFilterInternal(OrbisNpWebApiContext* context, s32 filterId);
s32 deletePushEventFilter(s32 libCtxId, s32 filterId);

// Push event callback functions
s32 registerPushEventCallbackInternal(OrbisNpWebApiUserContext* userContext, s32 filterId,
                                      OrbisNpWebApiPushEventCallback cbFunc, void* userArg);
s32 registerPushEventCallback(s32 titleUserCtxId, s32 filterId,
                              OrbisNpWebApiPushEventCallback cbFunc, void* pUserArg);
s32 unregisterPushEventCallback(s32 titleUserCtxId, s32 callbackId);

// Service push event filter functions
s32 createServicePushEventFilterInternal(
    OrbisNpWebApiContext* context, s32 handleId, const char* pNpServiceName, u32 npServiceLabel,
    const OrbisNpWebApiServicePushEventFilterParameter* pFilterParam, u64 filterParamNum);
s32 createServicePushEventFilter(s32 libCtxId, s32 handleId, const char* pNpServiceName,
                                 u32 npServiceLabel,
                                 const OrbisNpWebApiServicePushEventFilterParameter* pFilterParam,
                                 u64 filterParamNum);
s32 deleteServicePushEventFilterInternal(OrbisNpWebApiContext* context, s32 filterId);
s32 deleteServicePushEventFilter(s32 libCtxId, s32 filterId);

// Service push event callback functions
s32 registerServicePushEventCallbackInternal(
    OrbisNpWebApiUserContext* userContext, s32 filterId,
    OrbisNpWebApiServicePushEventCallback cbFunc,
    OrbisNpWebApiInternalServicePushEventCallback intCbFunc,
    OrbisNpWebApiInternalServicePushEventCallbackA intCbFuncA, void* pUserArg);
s32 registerServicePushEventCallback(s32 titleUserCtxId, s32 filterId,
                                     OrbisNpWebApiServicePushEventCallback cbFunc,
                                     OrbisNpWebApiInternalServicePushEventCallback intCbFunc,
                                     OrbisNpWebApiInternalServicePushEventCallbackA intCbFuncA,
                                     void* pUserArg);
s32 unregisterServicePushEventCallback(s32 titleUserCtxId, s32 callbackId);

// Extended push event filter functions
s32 createExtendedPushEventFilterInternal(
    OrbisNpWebApiContext* context, s32 handleId, const char* pNpServiceName, u32 npServiceLabel,
    const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam, u64 filterParamNum,
    bool internal);
s32 createExtendedPushEventFilter(s32 libCtxId, s32 handleId, const char* pNpServiceName,
                                  u32 npServiceLabel,
                                  const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam,
                                  u64 filterParamNum, bool internal);
s32 deleteExtendedPushEventFilterInternal(OrbisNpWebApiContext* context, s32 filterId);
s32 deleteExtendedPushEventFilter(s32 libCtxId, s32 filterId);

// Extended push event callback functions
s32 registerExtdPushEventCallbackInternal(OrbisNpWebApiUserContext* userContext, s32 filterId,
                                          OrbisNpWebApiExtdPushEventCallback cbFunc,
                                          OrbisNpWebApiExtdPushEventCallbackA cbFuncA,
                                          void* pUserArg);
s32 registerExtdPushEventCallback(s32 userCtxId, s32 filterId,
                                  OrbisNpWebApiExtdPushEventCallback cbFunc,
                                  OrbisNpWebApiExtdPushEventCallbackA cbFuncA, void* pUserArg);
s32 registerExtdPushEventCallbackA(s32 userCtxId, s32 filterId,
                                   OrbisNpWebApiExtdPushEventCallbackA cbFunc, void* pUserArg);
s32 unregisterExtdPushEventCallback(s32 titleUserCtxId, s32 callbackId);

s32 getHttpStatusCodeInternal(s64 requestId, s32* out_status_code);
s32 getHttpRequestIdFromRequest(OrbisNpWebApiRequest* request);
s32 readDataInternal(s64 requestId, void* pData, u64 size);
void setRequestEndTime(OrbisNpWebApiRequest* request);
void clearRequestEndTime(OrbisNpWebApiRequest* req);
bool hasRequestTimedOut(OrbisNpWebApiRequest* request);
void setRequestState(OrbisNpWebApiRequest* request, u8 state);
u64 copyRequestData(OrbisNpWebApiRequest* request, void* data, u64 size);

}; // namespace Libraries::Np::NpWebApi