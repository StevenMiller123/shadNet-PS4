// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "common/types.h"

namespace Libraries::Np::NpWebApi {

#define ORBIS_NP_WEBAPI_DEFAULT_CONNECTION_NUM 1
#define ORBIS_NP_WEBAPI_MAX_CONNECTION_NUM 16
#define ORBIS_NP_WEBAPI_PUSH_EVENT_DATA_TYPE_LEN_MAX 64
#define ORBIS_NP_WEBAPI_EXTD_PUSH_EVENT_EXTD_DATA_KEY_LEN_MAX 32

struct OrbisNpWebApiPushEventDataType {
    char val[ORBIS_NP_WEBAPI_PUSH_EVENT_DATA_TYPE_LEN_MAX + 1];
};

struct OrbisNpWebApiExtdPushEventExtdDataKey {
    char val[ORBIS_NP_WEBAPI_EXTD_PUSH_EVENT_EXTD_DATA_KEY_LEN_MAX + 1];
};

struct OrbisNpWebApiPushEventFilterParameter {
    OrbisNpWebApiPushEventDataType data_type;
};

struct OrbisNpWebApiServicePushEventFilterParameter {
    OrbisNpWebApiPushEventDataType data_type;
};

struct OrbisNpWebApiExtdPushEventFilterParameter {
    OrbisNpWebApiPushEventDataType data_type;
    OrbisNpWebApiExtdPushEventExtdDataKey* extd_data_key;
    u64 extd_data_key_num;
};

struct OrbisNpWebApiExtdPushEventExtdData {
    OrbisNpWebApiExtdPushEventExtdDataKey extd_data_key;
    char* data;
    u64 data_len;
};

struct OrbisNpWebApiHttpHeader {
    char* name;
    char* value;
};

struct OrbisNpWebApiMultipartPartParameter {
    OrbisNpWebApiHttpHeader* headers;
    u64 header_num;
    u64 content_length;
};

enum OrbisNpWebApiHttpMethod : s32 {
    ORBIS_NP_WEBAPI_HTTP_METHOD_GET,
    ORBIS_NP_WEBAPI_HTTP_METHOD_POST,
    ORBIS_NP_WEBAPI_HTTP_METHOD_PUT,
    ORBIS_NP_WEBAPI_HTTP_METHOD_DELETE,
    ORBIS_NP_WEBAPI_HTTP_METHOD_PATCH
};

struct OrbisNpWebApiContentParameter {
    u64 content_length;
    const char* content_type;
    u8 reserved[16];
};

struct OrbisNpWebApiResponseInformationOption {
    s32 http_status;
    char* error_object;
    u64 error_object_size;
    u64 response_data_size;
};

struct OrbisNpWebApiMemoryPoolStats {
    u64 pool_size;
    u64 max_inuse_size;
    u64 current_inuse_size;
    s32 reserved;
};

struct OrbisNpWebApiConnectionStats {
    u32 max;
    u32 used;
    u32 unused;
    u32 keep_alive;
    u64 reserved;
};

struct OrbisNpWebApiInitializeParameter {
    u32 lib_http_ctx_id;
    u8 reserved[4];
    u64 pool_size;
    const char* name;
    u64 struct_size;
};

// Needs reversing
struct OrbisNpWebApiIntCreateRequestExtraArgs {
    void* unk_0;
    void* unk_1;
    void* unk_2;
};

using OrbisNpWebApiPushEventCallback = void (*)(s32 user_ctx_id, s32 callback_id,
                                                OrbisNpPeerAddress* to, OrbisNpPeerAddress* from,
                                                OrbisNpWebApiPushEventDataType* data_type,
                                                const char* data, u64 data_len, void* user_arg);

using OrbisNpWebApiServicePushEventCallback = void (*)(
    s32 user_ctx_id, s32 callback_id, const char* np_service_name,
    OrbisNpServiceLabel np_service_label, OrbisNpPeerAddress* to, OrbisNpPeerAddress* from,
    OrbisNpWebApiPushEventDataType* data_type, const char* data, u64 data_len, void* user_arg);

using OrbisNpWebApiExtdPushEventCallback = void (*)(
    s32 user_ctx_id, s32 callback_id, const char* np_service_name,
    OrbisNpServiceLabel np_service_label, OrbisNpPeerAddress* to, OrbisNpPeerAddress* from,
    OrbisNpWebApiPushEventDataType* data_type, const char* data, u64 data_len,
    const OrbisNpWebApiExtdPushEventExtdData* extd_data, u64 extd_data_num, void* user_arg);

using OrbisNpWebApiExtdPushEventCallbackA =
    void (*)(s32 user_ctx_id, s32 callback_id, const char* np_service_name,
             OrbisNpServiceLabel np_service_label, const OrbisNpPeerAddressA* to,
             const OrbisNpOnlineId* to_online_id, const OrbisNpPeerAddressA* from,
             const OrbisNpOnlineId* from_online_id, const OrbisNpWebApiPushEventDataType* data_type,
             const char* data, u64 data_len, const OrbisNpWebApiExtdPushEventExtdData* extd_data,
             u64 extd_data_num, void* user_arg);

// Needs reversing
using OrbisNpWebApiInternalServicePushEventCallback = PS4_SYSV_ABI void (*)();
using OrbisNpWebApiInternalServicePushEventCallbackA = PS4_SYSV_ABI void (*)();
using OrbisNpWebApiNotificationCallback = PS4_SYSV_ABI void (*)();

struct PushEventInput {
    s32 target_user_id = 0;
    std::string np_service_name;
    u32 np_service_label = 0;
    std::string data_type;
    std::string data;
    OrbisNpOnlineId from_online_id{};
    bool has_from = false;
    OrbisNpOnlineId to_online_id{};
    bool has_to = false;
    u64 from_account_id = 0;
    u64 to_account_id = 0;
    std::vector<std::pair<std::string, std::string>> extd_data;
};

s32 sceNpWebApiCreateContext(s32 lib_ctx_id, OrbisNpOnlineId* online_id);
s32 sceNpWebApiCreatePushEventFilter(s32 lib_ctx_id,
                                     const OrbisNpWebApiPushEventFilterParameter* filter_param,
                                     u64 filter_param_num);
s32 sceNpWebApiCreateServicePushEventFilter(
    s32 lib_ctx_id, s32 handle_id, const char* np_service_name, u32 np_service_label,
    const OrbisNpWebApiServicePushEventFilterParameter* filter_param, u64 filter_param_num);
s32 sceNpWebApiDeletePushEventFilter(s32 lib_ctx_id, s32 filter_id);
s32 sceNpWebApiDeleteServicePushEventFilter(s32 lib_ctx_id, s32 filter_id);
s32 sceNpWebApiRegisterExtdPushEventCallback(s32 title_user_ctx_id, s32 filter_id,
                                             OrbisNpWebApiExtdPushEventCallback cb_func,
                                             void* user_arg);
s32 sceNpWebApiRegisterNotificationCallback(s32 title_user_ctx_id,
                                            OrbisNpWebApiNotificationCallback cb_func,
                                            void* user_arg);
s32 sceNpWebApiRegisterPushEventCallback(s32 title_user_ctx_id, s32 filter_id,
                                         OrbisNpWebApiPushEventCallback cb_func, void* user_arg);
s32 sceNpWebApiRegisterServicePushEventCallback(s32 title_user_ctx_id, s32 filter_id,
                                                OrbisNpWebApiServicePushEventCallback cb_func,
                                                void* user_arg);
s32 sceNpWebApiUnregisterNotificationCallback(s32 title_user_ctx_id);
s32 sceNpWebApiUnregisterPushEventCallback(s32 title_user_ctx_id, s32 callback_id);
s32 sceNpWebApiUnregisterServicePushEventCallback(s32 title_user_ctx_id, s32 callback_id);
s32 sceNpWebApiAbortHandle(s32 lib_ctx_id, s32 handle_id);
s32 sceNpWebApiAbortRequest(s64 request_id);
s32 sceNpWebApiAddHttpRequestHeader(s64 request_id, const char* field_name, const char* value);
s32 sceNpWebApiAddMultipartPart(s64 request_id, const OrbisNpWebApiMultipartPartParameter* param,
                                s32* index);
void sceNpWebApiCheckTimeout();
s32 sceNpWebApiClearAllUnusedConnection(s32 user_ctx_id, bool remain_keep_alive_connection);
s32 sceNpWebApiClearUnusedConnection(s32 user_ctx_id, const char* api_group,
                                     bool remain_keep_alive_connection);
s32 sceNpWebApiCreateContextA(s32 lib_ctx_id, s32 user_id);
s32 sceNpWebApiCreateExtdPushEventFilter(
    s32 lib_ctx_id, s32 handle_id, const char* np_service_name, u32 np_service_label,
    const OrbisNpWebApiExtdPushEventFilterParameter* filter_param, u64 filter_param_num);
s32 sceNpWebApiCreateHandle(s32 lib_ctx_id);
s32 sceNpWebApiCreateMultipartRequest(s32 title_user_ctx_id, const char* api_group,
                                      const char* path, OrbisNpWebApiHttpMethod method,
                                      s64* request_id);
s32 sceNpWebApiCreateRequest(s32 title_user_ctx_id, const char* api_group, const char* path,
                             OrbisNpWebApiHttpMethod method,
                             const OrbisNpWebApiContentParameter* content_parameter,
                             s64* request_id);
s32 sceNpWebApiDeleteContext(s32 title_user_ctx_id);
s32 sceNpWebApiDeleteExtdPushEventFilter(s32 lib_ctx_id, s32 filter_id);
s32 sceNpWebApiDeleteHandle(s32 lib_ctx_id, s32 handle_id);
s32 sceNpWebApiDeleteRequest(s64 request_id);
s32 sceNpWebApiGetConnectionStats(s32 user_ctx_id, const char* api_group,
                                  OrbisNpWebApiConnectionStats* stats);
s32 sceNpWebApiGetHttpResponseHeaderValue(s64 request_id, const char* field_name, char* value,
                                          u64 value_size);
s32 sceNpWebApiGetHttpResponseHeaderValueLength(s64 request_id, const char* field_name,
                                                u64* value_length);
s32 sceNpWebApiGetHttpStatusCode(s64 request_id, s32* out_status_code);
s32 sceNpWebApiGetMemoryPoolStats(s32 lib_ctx_id, OrbisNpWebApiMemoryPoolStats* current_stat);
s32 sceNpWebApiInitialize(s32 lib_http_ctx_id, u64 pool_size);
s32 sceNpWebApiInitializeForPresence(s32 lib_http_ctx_id, u64 pool_size);
s32 sceNpWebApiIntCreateCtxIndExtdPushEventFilter(
    s32 lib_ctx_id, s32 handle_id, const OrbisNpWebApiExtdPushEventFilterParameter* filter_param,
    u64 filter_param_num);
s32 sceNpWebApiIntCreateRequest(s32 title_user_ctx_id, const char* api_group, const char* path,
                                OrbisNpWebApiHttpMethod method,
                                const OrbisNpWebApiContentParameter* content_parameter,
                                const OrbisNpWebApiIntCreateRequestExtraArgs* internal_args,
                                s64* request_id);
s32 sceNpWebApiIntCreateServicePushEventFilter(
    s32 lib_ctx_id, s32 handle_id, const char* np_service_name, u32 np_service_label,
    const OrbisNpWebApiServicePushEventFilterParameter* filter_param, u64 filter_param_num);
s32 sceNpWebApiIntInitialize(const OrbisNpWebApiInitializeParameter* args);
s32 sceNpWebApiIntRegisterServicePushEventCallback(
    s32 title_user_ctx_id, s32 filter_id, OrbisNpWebApiInternalServicePushEventCallback cb_func,
    void* user_arg);
s32 sceNpWebApiIntRegisterServicePushEventCallbackA(
    s32 title_user_ctx_id, s32 filter_id, OrbisNpWebApiInternalServicePushEventCallbackA cb_func,
    void* user_arg);
s32 sceNpWebApiReadData(s64 request_id, void* data, u64 size);
s32 sceNpWebApiRegisterExtdPushEventCallbackA(s32 title_user_ctx_id, s32 filter_id,
                                              OrbisNpWebApiExtdPushEventCallbackA cb_func,
                                              void* user_arg);
s32 sceNpWebApiSendMultipartRequest(s64 request_id, s32 part_index, const void* data,
                                    u64 data_size);
s32 sceNpWebApiSendMultipartRequest2(s64 request_id, s32 part_index, const void* data,
                                     u64 data_size,
                                     OrbisNpWebApiResponseInformationOption* resp_info_option);
s32 sceNpWebApiSendRequest(s64 request_id, const void* data, u64 data_size);
s32 sceNpWebApiSendRequest2(s64 request_id, const void* data, u64 data_size,
                            OrbisNpWebApiResponseInformationOption* resp_info_option);
s32 sceNpWebApiSetHandleTimeout(s32 lib_ctx_id, s32 handle_id, u32 timeout);
s32 sceNpWebApiSetMaxConnection(s32 lib_ctx_id, s32 max_connection);
s32 sceNpWebApiSetMultipartContentType(s64 request_id, const char* type_name, const char* boundary);
s32 sceNpWebApiSetRequestTimeout(s64 request_id, u32 timeout);
s32 sceNpWebApiTerminate(s32 lib_ctx_id);
s32 sceNpWebApiUnregisterExtdPushEventCallback(s32 title_user_ctx_id, s32 callback_id);
s32 sceNpWebApiVshInitialize(s32 lib_http_ctx_id, u64 pool_size);

void EnqueuePushEvent(const PushEventInput& ev);
void RegisterHooks();
} // namespace Libraries::Np::NpWebApi
