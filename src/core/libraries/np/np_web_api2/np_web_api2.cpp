// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/NpWebApi2.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_manager/np_manager.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/np/np_web_api2/np_web_api2_internal.h"
#include "core/libraries/user_service/user_service.h"

extern "C" {
void sceNpWebApi2AddMultipartPart();
void sceNpWebApi2AddWebTraceTag();
void sceNpWebApi2CheckTimeout();
void sceNpWebApi2CreateMultipartRequest();
void sceNpWebApi2GetHttpResponseHeaderValue();
void sceNpWebApi2GetHttpResponseHeaderValueLength();
void sceNpWebApi2GetMemoryPoolStats();
void sceNpWebApi2Initialize();
void sceNpWebApi2InitializeForPresence();
void sceNpWebApi2IntCreateRequest();
void sceNpWebApi2IntInitialize();
void sceNpWebApi2IntInitialize2();
void sceNpWebApi2PushEventAbortHandle();
void sceNpWebApi2PushEventCreateFilter();
void sceNpWebApi2PushEventCreatePushContext();
void sceNpWebApi2PushEventDeletePushContext();
void sceNpWebApi2PushEventRegisterPushContextCallback();
void sceNpWebApi2PushEventSetHandleTimeout();
void sceNpWebApi2PushEventStartPushContextCallback();
void sceNpWebApi2PushEventUnregisterPushContextCallback();
void sceNpWebApi2SendMultipartRequest();
void sceNpWebApi2SetMultipartContentType();
void sceNpWebApi2SetRequestTimeout();
void sceNpWebApi2Terminate();
}

SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2AbortRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2AddHttpRequestHeader);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2AddMultipartPart);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2AddWebTraceTag);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2CheckTimeout);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2CreateMultipartRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2CreateRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2CreateUserContext);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2DeleteRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2DeleteUserContext);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2GetHttpResponseHeaderValue);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2GetHttpResponseHeaderValueLength);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2GetMemoryPoolStats);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2Initialize);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2InitializeForPresence);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2IntCreateRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2IntInitialize);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2IntInitialize2);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2IntPushEventCreateCtxIndFilter);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventAbortHandle);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventCreateFilter);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventCreateHandle);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventCreatePushContext);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventDeleteFilter);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventDeleteHandle);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventDeletePushContext);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventRegisterCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventRegisterPushContextCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventSetHandleTimeout);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventStartPushContextCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventUnregisterCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventUnregisterPushContextCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2ReadData);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2SendMultipartRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2SendRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2SetMultipartContentType);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2SetRequestTimeout);
SHADNET_HOOK_DECLARE(Libraries::Np::NpWebApi2, sceNpWebApi2Terminate);

static void RegisterNpWebApi2Hooks() {
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2AbortRequest);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2AddHttpRequestHeader);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2AddMultipartPart);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2AddWebTraceTag);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2CheckTimeout);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2CreateMultipartRequest);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2CreateRequest);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2CreateUserContext);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2DeleteRequest);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2DeleteUserContext);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2GetHttpResponseHeaderValue);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2GetHttpResponseHeaderValueLength);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2GetMemoryPoolStats);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2Initialize);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2InitializeForPresence);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2IntCreateRequest);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2IntInitialize);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2IntInitialize2);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2IntPushEventCreateCtxIndFilter);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventAbortHandle);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventCreateFilter);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventCreateHandle);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventCreatePushContext);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventDeleteFilter);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventDeleteHandle);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventDeletePushContext);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventRegisterCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventRegisterPushContextCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventSetHandleTimeout);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventStartPushContextCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventUnregisterCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2PushEventUnregisterPushContextCallback);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2ReadData);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2SendMultipartRequest);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2SendRequest);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2SetMultipartContentType);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2SetRequestTimeout);
    SHADNET_HOOK(Libraries::Np::NpWebApi2, sceNpWebApi2Terminate);
}

namespace Libraries::Np::NpWebApi2 {

s32 sceNpWebApi2AbortRequest(s64 request_id) {
    LOG_INFO(Lib_NpWebApi2, "called, request_id = {:#x}", request_id);
    return abortRequest(request_id);
}

s32 sceNpWebApi2AddHttpRequestHeader(s64 request_id, const char* field_name,
                                     const char* field_value) {
    if (!field_name || !field_value) {
        LOG_ERROR(Lib_NpWebApi2, "invalid argument");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi2, "called, request_id = {:#x}, field_name = {}, field_value = {}",
             request_id, field_name, field_value);
    return addHttpRequestHeader(request_id, field_name, field_value);
}

s32 sceNpWebApi2AddMultipartPart() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpWebApi2AddWebTraceTag(s64 request_id, const char* value) {
    if (!value) {
        LOG_ERROR(Lib_NpWebApi2, "invalid argument");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi2, "called, request_id = {:#x}, value = {}", request_id, value);
    return addHttpRequestHeader(request_id, "X-Psn-WebTrace-Tag", value);
}

void sceNpWebApi2CheckTimeout() {
    LOG_TRACE(Lib_NpWebApi2, "called");
    return checkTimeout();
}

s32 sceNpWebApi2CreateMultipartRequest(s32 user_ctx_id, const char* api_group, const char* path,
                                       const char* method, s64* request_id) {
    if (!api_group || !path || !method) {
        LOG_ERROR(Lib_NpWebApi2, "invalid argument");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }

    LOG_INFO(Lib_NpWebApi2, "called, user_ctx_id = {:#x}, api_group = {}, path = {}, method = {}",
             user_ctx_id, api_group, path, method);
    s64 temp_request_id = 0;
    s32 result =
        createRequest(user_ctx_id, api_group, path, method, nullptr, true, &temp_request_id);
    if (result == 0) {
        LOG_INFO(Lib_NpWebApi2, "created request_id = {:#x}", temp_request_id);
        if (request_id) {
            *request_id = temp_request_id;
        }
    }
    return result;
}

s32 sceNpWebApi2CreateRequest(s32 user_ctx_id, const char* api_group, const char* path,
                              const char* method,
                              const OrbisNpWebApi2ContentParameter* content_parameter,
                              s64* request_id) {
    if (!api_group || !path || !method) {
        LOG_ERROR(Lib_NpWebApi2, "invalid argument");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    if (content_parameter && content_parameter->content_length != 0 &&
        !content_parameter->content_type) {
        LOG_ERROR(Lib_NpWebApi2, "invalid content parameter");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_CONTENT_PARAMETER;
    }

    LOG_INFO(Lib_NpWebApi2, "called, user_ctx_id = {:#x}, api_group = {}, path = {}, method = {}",
             user_ctx_id, api_group, path, method);
    s64 temp_request_id = 0;
    s32 result = createRequest(user_ctx_id, api_group, path, method, content_parameter, false,
                               &temp_request_id);
    if (result == 0) {
        LOG_INFO(Lib_NpWebApi2, "created request_id = {:#x}", temp_request_id);
        if (request_id) {
            *request_id = temp_request_id;
        }
    }
    return result;
}

s32 sceNpWebApi2CreateUserContext(s32 lib_ctx_id, UserService::OrbisUserServiceUserId user_id) {
    if (lib_ctx_id >= 0x8000) {
        LOG_ERROR(Lib_NpWebApi2, "invalid lib context id {:#x}", lib_ctx_id);
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_LIB_CONTEXT_ID;
    }
    if (user_id == -1) {
        LOG_ERROR(Lib_NpWebApi2, "invalid user id {}", user_id);
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_DEBUG(Lib_NpWebApi2, "called, lib_ctx_id = {:#x}, user_id = {}", lib_ctx_id, user_id);
    s32 user_ctx_id = createUserContext(lib_ctx_id, user_id);
    if (user_ctx_id > 0) {
        LOG_INFO(Lib_NpWebApi2, "created user_ctx_id = {:#x}", user_ctx_id);
    }
    return user_ctx_id;
}

s32 sceNpWebApi2DeleteRequest(s64 request_id) {
    LOG_INFO(Lib_NpWebApi2, "called, request_id = {:#x}", request_id);
    return deleteRequest(request_id);
}

s32 sceNpWebApi2DeleteUserContext(s32 user_ctx_id) {
    LOG_INFO(Lib_NpWebApi2, "called, user_ctx_id = {:#x}", user_ctx_id);
    return deleteUserContext(user_ctx_id);
}

s32 sceNpWebApi2GetHttpResponseHeaderValue(s64 request_id, const char* field_name, char* value,
                                           u64 value_size) {
    if (!field_name || !value || value_size == 0) {
        LOG_ERROR(Lib_NpWebApi2, "invalid argument");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi2, "called, request_id = {:#x}, field_name = {}", request_id, field_name);
    return getHttpResponseHeaderData(request_id, field_name, value, value_size, nullptr);
}

s32 sceNpWebApi2GetHttpResponseHeaderValueLength(s64 request_id, const char* field_name,
                                                 u64* field_value_length) {
    if (!field_name || !field_value_length) {
        LOG_ERROR(Lib_NpWebApi2, "invalid argument");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    s32 result = getHttpResponseHeaderData(request_id, field_name, nullptr, 0, field_value_length);
    if (result >= 0) {
        LOG_INFO(Lib_NpWebApi2,
                 "on request_id = {:#x}, field_name = {} returned field_value_length = {:#x}",
                 request_id, field_name, *field_value_length);
    }
    return result;
}

s32 sceNpWebApi2GetMemoryPoolStats(s32 lib_ctx_id, OrbisNpWebApi2MemoryPoolStats* stats) {
    LOG_INFO(Lib_NpWebApi2, "called, lib_ctx_id = {:#x}", lib_ctx_id);
    return getMemoryPoolStats(lib_ctx_id, stats);
}

s32 sceNpWebApi2Initialize(s32 lib_http_ctx_id, u64 pool_size) {
    LOG_DEBUG(Lib_NpWebApi2, "called, lib_http_ctx_id = {:#x}, pool_size = {:#x}", lib_http_ctx_id,
              pool_size);

    // Uses a sceLncUtilGetAppStatus check to enable debug mode. For now, default to normal.
    s32 ctx_id = createLibraryContext(lib_http_ctx_id, 1, pool_size, nullptr);
    if (ctx_id > 0) {
        LOG_INFO(Lib_NpWebApi2, "created lib_ctx_id = {:#x}", ctx_id);
    }
    return ctx_id;
}

s32 sceNpWebApi2InitializeForPresence(s32 lib_http_ctx_id, u64 pool_size) {
    LOG_DEBUG(Lib_NpWebApi2, "called, lib_http_ctx_id = {:#x}, pool_size = {:#x}", lib_http_ctx_id,
              pool_size);

    s32 ctx_id = createLibraryContext(lib_http_ctx_id, 3, pool_size, nullptr);
    if (ctx_id > 0) {
        LOG_INFO(Lib_NpWebApi2, "created lib_ctx_id = {:#x}", ctx_id);
    }
    return ctx_id;
}

s32 sceNpWebApi2IntCreateRequest() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpWebApi2IntInitialize(const OrbisNpWebApi2IntInitializeArgs* args) {
    if (args == nullptr || args->struct_size != sizeof(OrbisNpWebApi2IntInitializeArgs)) {
        LOG_ERROR(Lib_NpWebApi2, "invalid argument");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_DEBUG(Lib_NpWebApi2, "called, lib_http_ctx_id = {:#x}, pool_size = {:#x}, name = {}",
              args->lib_http_ctx_id, args->pool_size, args->name ? args->name : "(null)");

    s32 ctx_id = createLibraryContext(args->lib_http_ctx_id, 2, args->pool_size, args->name);
    if (ctx_id > 0) {
        LOG_INFO(Lib_NpWebApi2, "created lib_ctx_id = {:#x}", ctx_id);
    }
    return ctx_id;
}

s32 sceNpWebApi2IntInitialize2(const OrbisNpWebApi2IntInitialize2Args* args) {

    if (args == nullptr || args->struct_size != sizeof(OrbisNpWebApi2IntInitialize2Args)) {
        LOG_ERROR(Lib_NpWebApi2, "invalid argument");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_DEBUG(
        Lib_NpWebApi2,
        "called, lib_http_ctx_id = {:#x}, pool_size = {:#x}, name = {}, push_config_group = {:#x}",
        args->lib_http_ctx_id, args->pool_size, args->name ? args->name : "(null)",
        args->push_config_group);

    s32 ctx_id = createLibraryContext(args->lib_http_ctx_id, 2, args->pool_size, args->name);
    if (ctx_id > 0) {
        LOG_INFO(Lib_NpWebApi2, "created lib_ctx_id = {:#x}", ctx_id);
    }
    return ctx_id;
}

s32 sceNpWebApi2IntPushEventCreateCtxIndFilter() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpWebApi2PushEventAbortHandle(s32 lib_ctx_id, s32 handle_id) {
    LOG_INFO(Lib_NpWebApi2, "called, lib_ctx_id = {:#x}, handle_id = {:#x}", lib_ctx_id, handle_id);
    return abortPushEventHandle(lib_ctx_id, handle_id);
}

s32 sceNpWebApi2PushEventCreateFilter(s32 lib_ctx_id, s32 handle_id, const char* np_service_name,
                                      OrbisNpServiceLabel np_service_label,
                                      const OrbisNpWebApi2PushEventFilterParameter* filter_param,
                                      u64 filter_param_num) {
    if ((np_service_name && np_service_label == ORBIS_NP_INVALID_SERVICE_LABEL) || !filter_param ||
        filter_param_num == 0) {
        LOG_ERROR(Lib_NpWebApi2, "invalid argument");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_DEBUG(Lib_NpWebApi2,
              "called, lib_ctx_id = {:#x}, handle_id = {:#x}, np_service_name = {}, "
              "np_service_label = {:#x}, filter_param_num = {:#x}",
              lib_ctx_id, handle_id, np_service_name ? np_service_name : "(null)",
              static_cast<u32>(np_service_label), filter_param_num);
    for (u64 i = 0; i < filter_param_num; i++) {
        LOG_DEBUG(Lib_NpWebApi2, "filter_param[{}].data_type.val = {}", i,
                  filter_param[i].data_type.val);
        LOG_DEBUG(Lib_NpWebApi2, "filter_param[{}].extd_data_key_num = {}", i,
                  filter_param[i].extd_data_key_num);
        for (u64 j = 0; j < filter_param[i].extd_data_key_num && filter_param[i].extd_data_key;
             j++) {
            LOG_DEBUG(Lib_NpWebApi2, "filter_param[{}].extd_data_key[{}].val = {}", i, j,
                      filter_param[i].extd_data_key[j].val);
        }
    }
    s32 filter_id = createPushEventFilter(lib_ctx_id, handle_id, np_service_name, np_service_label,
                                          filter_param, filter_param_num, false);
    if (filter_id > 0) {
        LOG_INFO(Lib_NpWebApi2, "created filter_id = {:#x}", filter_id);
    }
    return filter_id;
}

s32 sceNpWebApi2PushEventCreateHandle(s32 lib_ctx_id) {
    LOG_DEBUG(Lib_NpWebApi2, "called, lib_ctx_id = {:#x}", lib_ctx_id);
    s32 handle_id = createPushEventHandle(lib_ctx_id);
    if (handle_id > 0) {
        LOG_INFO(Lib_NpWebApi2, "created handle_id = {:#x}", handle_id);
    }
    return handle_id;
}

s32 sceNpWebApi2PushEventCreatePushContext(s32 user_ctx_id,
                                           OrbisNpWebApi2PushEventPushContextId* push_ctx_id) {
    if (!push_ctx_id) {
        LOG_ERROR(Lib_NpWebApi2, "invalid argument");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_DEBUG(Lib_NpWebApi2, "called, user_ctx_id = {:#x}", user_ctx_id);
    s32 result = createPushContext(user_ctx_id, push_ctx_id);
    if (result == 0) {
        s64 raw_id{};
        std::memcpy(&raw_id, push_ctx_id, sizeof(s64));
        LOG_INFO(Lib_NpWebApi2, "created push_ctx_id = {:#x}", raw_id);
    }
    return result;
}

s32 sceNpWebApi2PushEventDeleteFilter(s32 lib_ctx_id, s32 filter_id) {
    LOG_INFO(Lib_NpWebApi2, "called, lib_ctx_id = {:#x}, filter_id = {:#x}", lib_ctx_id, filter_id);
    return deletePushEventFilter(lib_ctx_id, filter_id);
}

s32 sceNpWebApi2PushEventDeleteHandle(s32 lib_ctx_id, s32 handle_id) {
    LOG_INFO(Lib_NpWebApi2, "called, lib_ctx_id = {:#x}, handle_id = {:#x}", lib_ctx_id, handle_id);
    return deletePushEventHandle(lib_ctx_id, handle_id);
}

s32 sceNpWebApi2PushEventDeletePushContext(s32 user_ctx_id,
                                           OrbisNpWebApi2PushEventPushContextId* push_ctx_id) {
    if (!push_ctx_id) {
        LOG_ERROR(Lib_NpWebApi2, "invalid argument");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    s64 raw_id{};
    std::memcpy(&raw_id, push_ctx_id, sizeof(s64));
    LOG_INFO(Lib_NpWebApi2, "called, user_ctx_id = {:#x}, push_ctx_id = {:#x}", user_ctx_id,
             raw_id);
    return deletePushContext(user_ctx_id, push_ctx_id);
}

s32 sceNpWebApi2PushEventRegisterCallback(s32 user_ctx_id, s32 filter_id,
                                          OrbisNpWebApi2PushEventCallback cb_func, void* user_arg) {
    LOG_DEBUG(Lib_NpWebApi2, "called, user_ctx_id = {:#x}, filter_id = {:#x}", user_ctx_id,
              filter_id);
    if (!cb_func) {
        LOG_ERROR(Lib_NpWebApi2, "invalid argument");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    s32 callback_id = registerPushEventCallback(user_ctx_id, filter_id, cb_func, user_arg);
    if (callback_id > 0) {
        LOG_INFO(Lib_NpWebApi2, "created callback_id = {:#x}", callback_id);
    }
    return callback_id;
}

s32 sceNpWebApi2PushEventRegisterPushContextCallback(
    s32 user_ctx_id, s32 filter_id, OrbisNpWebApi2PushEventPushContextCallback cb_func,
    void* user_arg) {
    LOG_DEBUG(Lib_NpWebApi2, "called, user_ctx_id = {:#x}, filter_id = {:#x}", user_ctx_id,
              filter_id);
    if (!cb_func) {
        LOG_ERROR(Lib_NpWebApi2, "invalid argument");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    s32 callback_id = registerPushContextCallback(user_ctx_id, filter_id, cb_func, user_arg);
    if (callback_id > 0) {
        LOG_INFO(Lib_NpWebApi2, "created callback_id = {:#x}", callback_id);
    }
    return callback_id;
}

s32 sceNpWebApi2PushEventSetHandleTimeout(s32 lib_ctx_id, s32 handle_id, u32 timeout) {
    LOG_INFO(Lib_NpWebApi2, "called, lib_ctx_id = {:#x}, handle_id = {:#x}, timeout = {}",
             lib_ctx_id, handle_id, timeout);
    return setHandleTimeout(lib_ctx_id, handle_id, timeout);
}

s32 sceNpWebApi2PushEventStartPushContextCallback(
    s32 user_ctx_id, const OrbisNpWebApi2PushEventPushContextId* push_ctx_id) {
    if (!push_ctx_id) {
        LOG_ERROR(Lib_NpWebApi2, "invalid argument");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    s64 raw_id{};
    std::memcpy(&raw_id, push_ctx_id, sizeof(s64));
    LOG_INFO(Lib_NpWebApi2, "called, user_ctx_id = {:#x}, push_ctx_id = {:#x}", user_ctx_id,
             raw_id);
    return startPushContextCallback(user_ctx_id, push_ctx_id);
}

s32 sceNpWebApi2PushEventUnregisterCallback(s32 user_ctx_id, s32 callback_id) {
    LOG_INFO(Lib_NpWebApi2, "called, user_ctx_id = {:#x}, callback_id = {:#x}", user_ctx_id,
             callback_id);
    return unregisterPushEventCallback(user_ctx_id, callback_id);
}

s32 sceNpWebApi2PushEventUnregisterPushContextCallback(s32 user_ctx_id, s32 callback_id) {
    LOG_INFO(Lib_NpWebApi2, "called, user_ctx_id = {:#x}, callback_id = {:#x}", user_ctx_id,
             callback_id);
    return unregisterPushContextCallback(user_ctx_id, callback_id);
}

s32 sceNpWebApi2ReadData(s64 request_id, void* data, u64 size) {
    if (!data || size == 0) {
        LOG_ERROR(Lib_NpWebApi2, "invalid argument");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi2, "called, request_id = {:#x}, size = {:#x}", request_id, size);
    return readData(request_id, data, size);
}

s32 sceNpWebApi2SendMultipartRequest(s64 request_id, s32 part_index, void* data, u64 data_size,
                                     OrbisNpWebApi2ResponseInformationOption* resp_info_option) {
    if (part_index <= 0 || !data || data_size == 0) {
        LOG_ERROR(Lib_NpWebApi2, "invalid argument");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }

    LOG_WARNING(Lib_NpWebApi2, "called, request_id = {:#x}, part_index = {}, data_size = {:#x}",
                request_id, part_index, data_size);
    return sendRequest(request_id, part_index, data, data_size, resp_info_option);
}

s32 sceNpWebApi2SendRequest(s64 request_id, void* data, u64 data_size,
                            OrbisNpWebApi2ResponseInformationOption* resp_info_option) {
    LOG_INFO(Lib_NpWebApi2, "called, request_id = {:#x}, data_size = {:#x}", request_id, data_size);
    return sendRequest(request_id, 0, data, data_size, resp_info_option);
}

s32 sceNpWebApi2SetMultipartContentType() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpWebApi2SetRequestTimeout(s64 request_id, u32 timeout) {
    LOG_INFO(Lib_NpWebApi2, "called, request_id = {:#x}, timeout = {}", request_id, timeout);
    return setRequestTimeout(request_id, timeout);
}

s32 sceNpWebApi2Terminate(s32 lib_ctx_id) {
    LOG_INFO(Lib_NpWebApi2, "called, lib_ctx_id = {:#x}", lib_ctx_id);
    return terminateLibraryContext(lib_ctx_id);
}

void RegisterHooks() {
    RegisterNpWebApi2Hooks();
    NpManager::RegisterNpCallback("npwebapi2_push", processPushEvents);
}

} // namespace Libraries::Np::NpWebApi2
