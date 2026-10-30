// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/user_service/user_service.h"

namespace Libraries::Np::NpWebApi2 {

struct OrbisNpWebApi2IntInitializeArgs {
    s32 lib_http_ctx_id;
    s32 reserved;
    u64 pool_size;
    const char* name;
    u64 struct_size;
};

struct OrbisNpWebApi2IntInitialize2Args {
    s32 lib_http_ctx_id;
    s32 reserved;
    u64 pool_size;
    const char* name;
    u32 push_config_group;
    s32 reserved2;
    u64 struct_size;
};

struct OrbisNpWebApi2ContentParameter {
    u64 content_length;
    const char* content_type;
    u8 reserved[16];
};

struct OrbisNpWebApi2MemoryPoolStats {
    u64 pool_size;
    u64 max_inuse_size;
    u64 current_inuse_size;
    s32 reserved;
};

struct OrbisNpWebApi2ResponseInformationOption {
    s32 http_status;
    s32 reserved;
    char* error_object;
    u64 error_object_size;
    u64 response_data_size;
};

struct OrbisNpWebApi2PushEventDataType {
    char val[65];
};

struct OrbisNpWebApi2PushEventExtdDataKey {
    char val[33];
};

struct OrbisNpWebApi2PushEventFilterParameter {
    OrbisNpWebApi2PushEventDataType data_type;
    OrbisNpWebApi2PushEventExtdDataKey* extd_data_key;
    u64 extd_data_key_num;
};

struct OrbisNpWebApi2PushEventExtdData {
    OrbisNpWebApi2PushEventExtdDataKey extd_data_key;
    char* data;
    u64 data_len;
};

using OrbisNpWebApi2PushEventCallback =
    void (*)(s32 user_ctx_id, s32 callback_id, const char* np_service_name,
             OrbisNpServiceLabel np_service_label, const OrbisNpPeerAddressA* to,
             const OrbisNpOnlineId* to_online_id, const OrbisNpPeerAddressA* from,
             const OrbisNpOnlineId* from_online_id, const OrbisNpWebApi2PushEventDataType* data_type,
             const char* data, u64 data_len, const OrbisNpWebApi2PushEventExtdData* extd_data,
             u64 extd_data_num, void* user_arg);

enum OrbisNpWebApi2PushEventPushContextCallbackType {
    Unknown = -1,
    Received = 0,
    Dropped = 1,
};

struct OrbisNpWebApi2PushEventPushContextId {
    char uuid[37];
};

using OrbisNpWebApi2PushEventPushContextCallback =
    void (*)(s32 user_ctx_id, s32 callback_id,
             const OrbisNpWebApi2PushEventPushContextId* push_ctx_id,
             OrbisNpWebApi2PushEventPushContextCallbackType cb_type, const char* np_service_name,
             OrbisNpServiceLabel np_service_label, const OrbisNpPeerAddressA* to,
             const OrbisNpOnlineId* to_online_id, const OrbisNpPeerAddressA* from,
             const OrbisNpOnlineId* from_online_id, const OrbisNpWebApi2PushEventDataType* data_type,
             const char* data, u64 data_len, const OrbisNpWebApi2PushEventExtdData* extd_data,
             u64 extd_data_num, void* user_arg);

s32 sceNpWebApi2AbortRequest(s64 request_id);
s32 sceNpWebApi2AddHttpRequestHeader(s64 request_id, const char* field_name,
                                     const char* field_value);
s32 sceNpWebApi2AddMultipartPart();
s32 sceNpWebApi2AddWebTraceTag(s64 request_id, const char* value);
void sceNpWebApi2CheckTimeout();
s32 sceNpWebApi2CreateMultipartRequest(s32 user_ctx_id, const char* api_group, const char* path,
                                       const char* method, s64* request_id);
s32 sceNpWebApi2CreateRequest(s32 user_ctx_id, const char* api_group, const char* path,
                              const char* method,
                              const OrbisNpWebApi2ContentParameter* content_parameter,
                              s64* request_id);
s32 sceNpWebApi2CreateUserContext(s32 lib_ctx_id, UserService::OrbisUserServiceUserId user_id);
s32 sceNpWebApi2DeleteRequest(s64 request_id);
s32 sceNpWebApi2DeleteUserContext(s32 user_ctx_id);
s32 sceNpWebApi2GetHttpResponseHeaderValue(s64 request_id, const char* field_name, char* value,
                                           u64 value_size);
s32 sceNpWebApi2GetHttpResponseHeaderValueLength(s64 request_id, const char* field_name,
                                                 u64* field_value_length);
s32 sceNpWebApi2GetMemoryPoolStats(s32 lib_ctx_id, OrbisNpWebApi2MemoryPoolStats* stats);
s32 sceNpWebApi2Initialize(s32 lib_http_ctx_id, u64 pool_size);
s32 sceNpWebApi2InitializeForPresence(s32 lib_http_ctx_id, u64 pool_size);
s32 sceNpWebApi2IntCreateRequest();
s32 sceNpWebApi2IntInitialize(const OrbisNpWebApi2IntInitializeArgs* args);
s32 sceNpWebApi2IntInitialize2(const OrbisNpWebApi2IntInitialize2Args* args);
s32 sceNpWebApi2IntPushEventCreateCtxIndFilter();
s32 sceNpWebApi2PushEventAbortHandle(s32 lib_ctx_id, s32 handle_id);
s32 sceNpWebApi2PushEventCreateFilter(s32 lib_ctx_id, s32 handle_id, const char* np_service_name,
                                      OrbisNpServiceLabel np_service_label,
                                      const OrbisNpWebApi2PushEventFilterParameter* filter_param,
                                      u64 filter_param_num);
s32 sceNpWebApi2PushEventCreateHandle(s32 lib_ctx_id);
s32 sceNpWebApi2PushEventCreatePushContext(s32 user_ctx_id,
                                           OrbisNpWebApi2PushEventPushContextId* push_ctx_id);
s32 sceNpWebApi2PushEventDeleteFilter(s32 lib_ctx_id, s32 filter_id);
s32 sceNpWebApi2PushEventDeleteHandle(s32 lib_ctx_id, s32 handle_id);
s32 sceNpWebApi2PushEventDeletePushContext(s32 user_ctx_id,
                                           OrbisNpWebApi2PushEventPushContextId* push_ctx_id);
s32 sceNpWebApi2PushEventRegisterCallback(s32 user_ctx_id, s32 filter_id,
                                          OrbisNpWebApi2PushEventCallback cb_func, void* user_arg);
s32 sceNpWebApi2PushEventRegisterPushContextCallback(
    s32 user_ctx_id, s32 filter_id, OrbisNpWebApi2PushEventPushContextCallback cb_func,
    void* user_arg);
s32 sceNpWebApi2PushEventSetHandleTimeout(s32 lib_ctx_id, s32 handle_id, u32 timeout);
s32 sceNpWebApi2PushEventStartPushContextCallback(
    s32 user_ctx_id, const OrbisNpWebApi2PushEventPushContextId* push_ctx_id);
s32 sceNpWebApi2PushEventUnregisterCallback(s32 user_ctx_id, s32 callback_id);
s32 sceNpWebApi2PushEventUnregisterPushContextCallback(s32 user_ctx_id, s32 callback_id);
s32 sceNpWebApi2ReadData(s64 request_id, void* data, u64 size);
s32 sceNpWebApi2SendMultipartRequest(s64 request_id, s32 part_index, void* data, u64 data_size,
                                     OrbisNpWebApi2ResponseInformationOption* resp_info_option);
s32 sceNpWebApi2SendRequest(s64 request_id, void* data, u64 data_size,
                            OrbisNpWebApi2ResponseInformationOption* resp_info_option);
s32 sceNpWebApi2SetMultipartContentType();
s32 sceNpWebApi2SetRequestTimeout(s64 request_id, u32 timeout);
s32 sceNpWebApi2Terminate(s32 lib_ctx_id);

void RegisterHooks();
} // namespace Libraries::Np::NpWebApi2