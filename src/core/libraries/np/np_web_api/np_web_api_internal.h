// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "common/recursive_mutex.h"
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
    s32 user_count;
    s32 lib_ctx_id;
    s32 lib_http_ctx_id;
    Common::RecursiveMutex context_lock;
    std::map<s32, OrbisNpWebApiUserContext*> user_contexts;
    std::map<s32, OrbisNpWebApiHandle*> handles;
    std::map<s32, OrbisNpWebApiTimerHandle*> timer_handles;
    std::map<s32, OrbisNpWebApiPushEventFilter*> push_event_filters;
    std::map<s32, OrbisNpWebApiServicePushEventFilter*> service_push_event_filters;
    std::map<s32, OrbisNpWebApiExtendedPushEventFilter*> extended_push_event_filters;
    std::string name;
    bool terminated;
};

struct OrbisNpWebApiUserContext {
    OrbisNpWebApiContext* parent_context;
    s32 user_count;
    s32 user_ctx_id;
    s32 user_id;
    std::map<s64, OrbisNpWebApiRequest*> requests;
    std::map<s32, OrbisNpWebApiRegisteredPushEventCallback*> push_event_callbacks;
    std::map<s32, OrbisNpWebApiRegisteredServicePushEventCallback*> service_push_event_callbacks;
    std::map<s32, OrbisNpWebApiRegisteredExtendedPushEventCallback*> extended_push_event_callbacks;
    bool deleted;
    OrbisNpWebApiNotificationCallback notification_callback_function;
    void* notification_callback_user_args;
};

struct OrbisNpWebApiRequest {
    OrbisNpWebApiContext* parent_context;
    s32 user_count;
    s64 request_id;
    std::string user_api_group;
    std::string user_path;
    OrbisNpWebApiHttpMethod user_method;
    u64 user_content_length;
    std::string user_content_type;
    std::vector<std::pair<std::string, std::string>> user_headers;
    bool multipart;
    struct MultipartPart {
        std::string raw_headers;
        u64 content_length = 0;
        std::string data;
        u64 sent_size = 0;
    };
    std::string multipart_content_type;
    std::string multipart_boundary;
    std::vector<MultipartPart> multipart_parts;
    bool aborted;
    bool sent;
    u32 request_timeout;
    u64 request_end_time;
    bool timed_out;
    u8 request_state;
    u64 remaining_data;
    u32 read_offset;
    char data[64];
    s32 http_connection_id = 0;
    s32 http_request_id = 0;
    s32 http_template_id = 0;
};

struct OrbisNpWebApiHandle {
    s32 handle_id;
    bool aborted;
    bool deleted;
    s32 user_count;
};

struct OrbisNpWebApiTimerHandle {
    s32 handle_id;
    u32 handle_timeout;
    u64 handle_end_time;
    bool timed_out;
};

struct OrbisNpWebApiPushEventFilter {
    s32 filter_id;
    std::vector<OrbisNpWebApiPushEventFilterParameter> filter_params;
    OrbisNpWebApiContext* parent_context;
};

struct OrbisNpWebApiServicePushEventFilter {
    s32 filter_id;
    bool internal;
    std::vector<OrbisNpWebApiServicePushEventFilterParameter> filter_params;
    std::string np_service_name;
    u32 np_service_label;
    OrbisNpWebApiContext* parent_context;
};

struct OrbisNpWebApiExtendedPushEventFilter {
    s32 filter_id;
    bool internal;
    std::vector<OrbisNpWebApiExtdPushEventFilterParameter> filter_params;
    std::vector<std::vector<std::string>> extd_data_keys;
    std::string np_service_name;
    u32 np_service_label;
    OrbisNpWebApiContext* parent_context;
};

struct OrbisNpWebApiRegisteredPushEventCallback {
    s32 callback_id;
    s32 filter_id;
    OrbisNpWebApiPushEventCallback cb_func;
    void* user_arg;
};

struct OrbisNpWebApiRegisteredServicePushEventCallback {
    s32 callback_id;
    s32 filter_id;
    OrbisNpWebApiServicePushEventCallback cb_func;
    OrbisNpWebApiInternalServicePushEventCallback internal_cb_func;
    // Note: real struct stores both internal callbacks in one field
    OrbisNpWebApiInternalServicePushEventCallbackA internal_cb_func_a;
    void* user_arg;
};

struct OrbisNpWebApiRegisteredExtendedPushEventCallback {
    s32 callback_id;
    s32 filter_id;
    OrbisNpWebApiExtdPushEventCallback cb_func;
    // Note: real struct stores both callbacks in one field
    OrbisNpWebApiExtdPushEventCallbackA cb_func_a;
    void* user_arg;
};

// General functions
s32 initializeLibrary();
s32 getCompiledSdkVersion();

// Library context functions
s32 createLibraryContext(s32 lib_http_ctx_id, u64 pool_size, const char* name, s32 type);
OrbisNpWebApiContext* findAndValidateContext(s32 lib_ctx_id, s32 flag = 0);
void releaseContext(OrbisNpWebApiContext* context);
bool isContextTerminated(OrbisNpWebApiContext* context);
bool isContextBusy(OrbisNpWebApiContext* context);
bool areContextHandlesBusy(OrbisNpWebApiContext* context);
void lockContext(OrbisNpWebApiContext* context);
void unlockContext(OrbisNpWebApiContext* context);
void markContextAsTerminated(OrbisNpWebApiContext* context);
void checkContextTimeout(OrbisNpWebApiContext* context);
void checkTimeout();
s32 deleteContext(s32 lib_ctx_id);
s32 terminateContext(s32 lib_ctx_id);

// User context functions
OrbisNpWebApiUserContext* findUserContextByUserId(OrbisNpWebApiContext* context, s32 user_id);
OrbisNpWebApiUserContext* findUserContext(OrbisNpWebApiContext* context, s32 user_ctx_id);
s32 createUserContextWithOnlineId(s32 lib_ctx_id, OrbisNpOnlineId* online_id);
s32 createUserContext(s32 lib_ctx_id, s32 user_id);
s32 registerNotificationCallback(s32 title_user_ctx_id, OrbisNpWebApiNotificationCallback cb_func,
                                 void* user_arg);
s32 unregisterNotificationCallback(s32 title_user_ctx_id);
bool isUserContextBusy(OrbisNpWebApiUserContext* user_context);
bool areUserContextRequestsBusy(OrbisNpWebApiUserContext* user_context);
void releaseUserContext(OrbisNpWebApiUserContext* user_context);
void checkUserContextTimeout(OrbisNpWebApiUserContext* user_context);
s32 deleteUserContext(s32 user_ctx_id);

// Request functions
s32 createRequest(s32 title_user_ctx_id, const char* api_group, const char* path,
                  OrbisNpWebApiHttpMethod method,
                  const OrbisNpWebApiContentParameter* content_parameter,
                  const OrbisNpWebApiIntCreateRequestExtraArgs* internal_args, s64* out_request_id,
                  bool is_multipart);
OrbisNpWebApiRequest* findRequest(OrbisNpWebApiUserContext* user_context, s64 request_id);
OrbisNpWebApiRequest* findRequestAndMarkBusy(OrbisNpWebApiUserContext* user_context,
                                             s64 request_id);
bool isRequestBusy(OrbisNpWebApiRequest* request);
s32 setRequestTimeout(s64 request_id, u32 timeout);
void startRequestTimer(OrbisNpWebApiRequest* request);
void checkRequestTimeout(OrbisNpWebApiRequest* request);
s32 sendRequest(s64 request_id, s32 part_index, const void* data, u64 data_size, s8 flag,
                OrbisNpWebApiResponseInformationOption* response_information_option);
s32 setMultipartContentType(s64 request_id, const char* type_name, const char* boundary);
s32 addMultipartPart(s64 request_id, const OrbisNpWebApiMultipartPartParameter* param, s32* index);
s32 abortRequestInternal(OrbisNpWebApiContext* context, OrbisNpWebApiUserContext* user_context,
                         OrbisNpWebApiRequest* request);
s32 abortRequest(s64 request_id);
void releaseRequest(OrbisNpWebApiRequest* request);
s32 deleteRequest(s64 request_id);

s32 addHttpRequestHeaderInternal(s64 request_id, const char* field_name, const char* value);
s32 getHttpResponseHeaderValueInternal(s64 request_id, const char* field_name, char* out_value,
                                       u64 value_size, u64* value_length);
s32 getLastWebApiError();

void DrainPushEvents();

// Handle functions
s32 createHandleInternal(OrbisNpWebApiContext* context);
s32 createHandle(s32 lib_ctx_id);
s32 setHandleTimeoutInternal(OrbisNpWebApiContext* context, s32 handle_id, u32 timeout);
s32 setHandleTimeout(s32 lib_ctx_id, s32 handle_id, u32 timeout);
void startHandleTimer(OrbisNpWebApiContext* context, s32 handle_id);
void releaseHandle(OrbisNpWebApiContext* context, OrbisNpWebApiHandle* handle);
s32 getHandle(OrbisNpWebApiContext* context, s32 handle_id, OrbisNpWebApiHandle** handle_out);
s32 abortHandle(s32 lib_ctx_id, s32 handle_id);
s32 deleteHandleInternal(OrbisNpWebApiContext* context, s32 handle_id);
s32 deleteHandle(s32 lib_ctx_id, s32 handle_id);

// Push event filter functions
s32 createPushEventFilterInternal(OrbisNpWebApiContext* context,
                                  const OrbisNpWebApiPushEventFilterParameter* filter_param,
                                  u64 filter_param_num);
s32 createPushEventFilter(s32 lib_ctx_id, const OrbisNpWebApiPushEventFilterParameter* filter_param,
                          u64 filter_param_num);
s32 deletePushEventFilterInternal(OrbisNpWebApiContext* context, s32 filter_id);
s32 deletePushEventFilter(s32 lib_ctx_id, s32 filter_id);

// Push event callback functions
s32 registerPushEventCallbackInternal(OrbisNpWebApiUserContext* user_context, s32 filter_id,
                                      OrbisNpWebApiPushEventCallback cb_func, void* user_arg);
s32 registerPushEventCallback(s32 title_user_ctx_id, s32 filter_id,
                              OrbisNpWebApiPushEventCallback cb_func, void* user_arg);
s32 unregisterPushEventCallback(s32 title_user_ctx_id, s32 callback_id);

// Service push event filter functions
s32 createServicePushEventFilterInternal(
    OrbisNpWebApiContext* context, s32 handle_id, const char* np_service_name, u32 np_service_label,
    const OrbisNpWebApiServicePushEventFilterParameter* filter_param, u64 filter_param_num);
s32 createServicePushEventFilter(s32 lib_ctx_id, s32 handle_id, const char* np_service_name,
                                 u32 np_service_label,
                                 const OrbisNpWebApiServicePushEventFilterParameter* filter_param,
                                 u64 filter_param_num);
s32 deleteServicePushEventFilterInternal(OrbisNpWebApiContext* context, s32 filter_id);
s32 deleteServicePushEventFilter(s32 lib_ctx_id, s32 filter_id);

// Service push event callback functions
s32 registerServicePushEventCallbackInternal(
    OrbisNpWebApiUserContext* user_context, s32 filter_id,
    OrbisNpWebApiServicePushEventCallback cb_func,
    OrbisNpWebApiInternalServicePushEventCallback int_cb_func,
    OrbisNpWebApiInternalServicePushEventCallbackA int_cb_func_a, void* user_arg);
s32 registerServicePushEventCallback(s32 title_user_ctx_id, s32 filter_id,
                                     OrbisNpWebApiServicePushEventCallback cb_func,
                                     OrbisNpWebApiInternalServicePushEventCallback int_cb_func,
                                     OrbisNpWebApiInternalServicePushEventCallbackA int_cb_func_a,
                                     void* user_arg);
s32 unregisterServicePushEventCallback(s32 title_user_ctx_id, s32 callback_id);

// Extended push event filter functions
s32 createExtendedPushEventFilterInternal(
    OrbisNpWebApiContext* context, s32 handle_id, const char* np_service_name, u32 np_service_label,
    const OrbisNpWebApiExtdPushEventFilterParameter* filter_param, u64 filter_param_num,
    bool internal);
s32 createExtendedPushEventFilter(s32 lib_ctx_id, s32 handle_id, const char* np_service_name,
                                  u32 np_service_label,
                                  const OrbisNpWebApiExtdPushEventFilterParameter* filter_param,
                                  u64 filter_param_num, bool internal);
s32 deleteExtendedPushEventFilterInternal(OrbisNpWebApiContext* context, s32 filter_id);
s32 deleteExtendedPushEventFilter(s32 lib_ctx_id, s32 filter_id);

// Extended push event callback functions
s32 registerExtdPushEventCallbackInternal(OrbisNpWebApiUserContext* user_context, s32 filter_id,
                                          OrbisNpWebApiExtdPushEventCallback cb_func,
                                          OrbisNpWebApiExtdPushEventCallbackA cb_func_a,
                                          void* user_arg);
s32 registerExtdPushEventCallback(s32 user_ctx_id, s32 filter_id,
                                  OrbisNpWebApiExtdPushEventCallback cb_func,
                                  OrbisNpWebApiExtdPushEventCallbackA cb_func_a, void* user_arg);
s32 registerExtdPushEventCallbackA(s32 user_ctx_id, s32 filter_id,
                                   OrbisNpWebApiExtdPushEventCallbackA cb_func, void* user_arg);
s32 unregisterExtdPushEventCallback(s32 title_user_ctx_id, s32 callback_id);

s32 getHttpStatusCodeInternal(s64 request_id, s32* out_status_code);
s32 getHttpRequestIdFromRequest(OrbisNpWebApiRequest* request);
s32 readDataInternal(s64 request_id, void* data, u64 size);
void setRequestEndTime(OrbisNpWebApiRequest* request);
void clearRequestEndTime(OrbisNpWebApiRequest* req);
bool hasRequestTimedOut(OrbisNpWebApiRequest* request);
void setRequestState(OrbisNpWebApiRequest* request, u8 state);
u64 copyRequestData(OrbisNpWebApiRequest* request, void* data, u64 size);

}; // namespace Libraries::Np::NpWebApi
