// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <magic_enum/magic_enum.hpp>
#include <orbis/NpWebApi.h>
#include <orbis/UserService.h>

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_manager/np_manager.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/np/np_web_api/np_web_api.h"
#include "core/libraries/np/np_web_api/np_web_api_internal.h"

extern "C" {
void sceNpWebApiVshInitialize();
void sceNpWebApiInitializeForPresence();
void sceNpWebApiIntCreateRequest();
void sceNpWebApiIntRegisterServicePushEventCallback();
void sceNpWebApiIntRegisterServicePushEventCallbackA();
};

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

static void RegisterNpWebApiHooks() {
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
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiCreateContextA);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiCreateExtdPushEventFilter);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiCreateHandle);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiCreateMultipartRequest);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiCreateRequest);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiDeleteContext);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiDeleteExtdPushEventFilter);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiDeleteHandle);
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiDeleteRequest);
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
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiRegisterExtdPushEventCallbackA);
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
    SHADNET_HOOK(Libraries::Np::NpWebApi, sceNpWebApiVshInitialize);
}

namespace Libraries::Np::NpWebApi {

static bool g_is_initialized = false;
static s32 g_active_library_contexts = 0;

s32 sceNpWebApiCreateContext(s32 lib_ctx_id, OrbisNpOnlineId* online_id) {
    if (lib_ctx_id >= 0x8000) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_LIB_CONTEXT_ID;
    }
    if (online_id == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    return createUserContextWithOnlineId(lib_ctx_id, online_id);
}

s32 sceNpWebApiCreatePushEventFilter(s32 lib_ctx_id,
                                     const OrbisNpWebApiPushEventFilterParameter* filter_param,
                                     u64 filter_param_num) {
    if (filter_param == nullptr || filter_param_num == 0) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_WARNING(Lib_NpWebApi, "called, lib_ctx_id = {:#x}", lib_ctx_id);
    return createPushEventFilter(lib_ctx_id, filter_param, filter_param_num);
}

s32 sceNpWebApiCreateServicePushEventFilter(
    s32 lib_ctx_id, s32 handle_id, const char* np_service_name, u32 np_service_label,
    const OrbisNpWebApiServicePushEventFilterParameter* filter_param, u64 filter_param_num) {
    if (np_service_name == nullptr || filter_param == nullptr || filter_param_num == 0) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    if (getCompiledSdkVersion() >= Common::ElfInfo::FW_200 && np_service_label == -1) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_WARNING(Lib_NpWebApi,
                "called, lib_ctx_id = {:#x}, handle_id = {:#x}, np_service_name = '{}', "
                "np_service_label = {:#x}",
                lib_ctx_id, handle_id, np_service_name, np_service_label);
    return createServicePushEventFilter(lib_ctx_id, handle_id, np_service_name, np_service_label,
                                        filter_param, filter_param_num);
}

s32 sceNpWebApiDeletePushEventFilter(s32 lib_ctx_id, s32 filter_id) {
    LOG_INFO(Lib_NpWebApi, "called, lib_ctx_id = {:#x}, filter_id = {:#x}", lib_ctx_id, filter_id);
    return deletePushEventFilter(lib_ctx_id, filter_id);
}

s32 sceNpWebApiDeleteServicePushEventFilter(s32 lib_ctx_id, s32 filter_id) {
    LOG_INFO(Lib_NpWebApi, "called, lib_ctx_id = {:#x}, filter_id = {:#x}", lib_ctx_id, filter_id);
    return deleteServicePushEventFilter(lib_ctx_id, filter_id);
}

s32 sceNpWebApiRegisterExtdPushEventCallback(s32 title_user_ctx_id, s32 filter_id,
                                             OrbisNpWebApiExtdPushEventCallback cb_func,
                                             void* user_arg) {
    if (cb_func == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi, "called, title_user_ctx_id = {:#x}, filter_id = {:#x}, cb_func = {}",
             title_user_ctx_id, filter_id, reinterpret_cast<void*>(cb_func));
    return registerExtdPushEventCallback(title_user_ctx_id, filter_id, cb_func, nullptr, user_arg);
}

s32 sceNpWebApiRegisterNotificationCallback(s32 title_user_ctx_id,
                                            OrbisNpWebApiNotificationCallback cb_func,
                                            void* user_arg) {
    if (cb_func == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi, "called, title_user_ctx_id = {:#x}, cb_func = {}", title_user_ctx_id,
             reinterpret_cast<void*>(cb_func));
    return registerNotificationCallback(title_user_ctx_id, cb_func, user_arg);
}

s32 sceNpWebApiRegisterPushEventCallback(s32 title_user_ctx_id, s32 filter_id,
                                         OrbisNpWebApiPushEventCallback cb_func, void* user_arg) {
    if (getCompiledSdkVersion() >= Common::ElfInfo::FW_100 && cb_func == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi, "called, title_user_ctx_id = {:#x}, filter_id = {:#x}, cb_func = {}",
             title_user_ctx_id, filter_id, reinterpret_cast<void*>(cb_func));
    return registerPushEventCallback(title_user_ctx_id, filter_id, cb_func, user_arg);
}

s32 sceNpWebApiRegisterServicePushEventCallback(s32 title_user_ctx_id, s32 filter_id,
                                                OrbisNpWebApiServicePushEventCallback cb_func,
                                                void* user_arg) {
    if (getCompiledSdkVersion() >= Common::ElfInfo::FW_100 && cb_func == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi, "called, title_user_ctx_id = {:#x}, filter_id = {:#x}, cb_func = {}",
             title_user_ctx_id, filter_id, reinterpret_cast<void*>(cb_func));
    return registerServicePushEventCallback(title_user_ctx_id, filter_id, cb_func, nullptr, nullptr,
                                            user_arg);
}

s32 sceNpWebApiUnregisterNotificationCallback(s32 title_user_ctx_id) {
    LOG_INFO(Lib_NpWebApi, "called, title_user_ctx_id = {:#x}", title_user_ctx_id);
    return unregisterNotificationCallback(title_user_ctx_id);
}

s32 sceNpWebApiUnregisterPushEventCallback(s32 title_user_ctx_id, s32 callback_id) {
    LOG_INFO(Lib_NpWebApi, "called, title_user_ctx_id = {:#x}, callback_id = {:#x}",
             title_user_ctx_id, callback_id);
    return unregisterPushEventCallback(title_user_ctx_id, callback_id);
}

s32 sceNpWebApiUnregisterServicePushEventCallback(s32 title_user_ctx_id, s32 callback_id) {
    LOG_INFO(Lib_NpWebApi, "called, title_user_ctx_id = {:#x}, callback_id = {:#x}",
             title_user_ctx_id, callback_id);
    return unregisterServicePushEventCallback(title_user_ctx_id, callback_id);
}

s32 sceNpWebApiAbortHandle(s32 lib_ctx_id, s32 handle_id) {
    LOG_INFO(Lib_NpWebApi, "called, lib_ctx_id = {:#x}, handle_id = {:#x}", lib_ctx_id, handle_id);
    return abortHandle(lib_ctx_id, handle_id);
}

s32 sceNpWebApiAbortRequest(s64 request_id) {
    LOG_INFO(Lib_NpWebApi, "called, request_id = {:#x}", request_id);
    return abortRequest(request_id);
}

s32 sceNpWebApiAddHttpRequestHeader(s64 request_id, const char* field_name, const char* value) {
    LOG_INFO(Lib_NpWebApi, "called, request_id = {:#x}, field_name = '{}', value = '{}'",
             request_id, (field_name ? field_name : "null"), (value ? value : "null"));
    if (field_name == nullptr || value == nullptr)
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    return addHttpRequestHeaderInternal(request_id, field_name, value);
}

s32 sceNpWebApiAddMultipartPart(s64 request_id, const OrbisNpWebApiMultipartPartParameter* param,
                                s32* index) {
    LOG_INFO(Lib_NpWebApi, "called, request_id = {:#x}, header_num = {}, content_length = {}",
             request_id, (param ? param->header_num : 0), (param ? param->content_length : 0));
    return addMultipartPart(request_id, param, index);
}

void sceNpWebApiCheckTimeout() {
    if (!g_is_initialized) {
        return;
    }
    return checkTimeout();
}

s32 sceNpWebApiClearAllUnusedConnection(s32 user_ctx_id, bool remain_keep_alive_connection) {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpWebApiClearUnusedConnection(s32 user_ctx_id, const char* api_group,
                                     bool remain_keep_alive_connection) {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpWebApiCreateContextA(s32 lib_ctx_id, s32 user_id) {
    if (lib_ctx_id >= 0x8000) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_LIB_CONTEXT_ID;
    }
    if (user_id == ORBIS_USER_SERVICE_USER_ID_INVALID) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    return createUserContext(lib_ctx_id, user_id);
}

s32 sceNpWebApiCreateExtdPushEventFilter(
    s32 lib_ctx_id, s32 handle_id, const char* np_service_name, u32 np_service_label,
    const OrbisNpWebApiExtdPushEventFilterParameter* filter_param, u64 filter_param_num) {
    if ((np_service_name != nullptr && np_service_label == -1) || filter_param == nullptr ||
        filter_param_num == 0) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    LOG_INFO(Lib_NpWebApi,
             "called, lib_ctx_id = {:#x}, handle_id = {:#x}, np_service_name = '{}', "
             "np_service_label = {:#x}",
             lib_ctx_id, handle_id, (np_service_name ? np_service_name : "null"), np_service_label);
    return createExtendedPushEventFilter(lib_ctx_id, handle_id, np_service_name, np_service_label,
                                         filter_param, filter_param_num, false);
}

s32 sceNpWebApiCreateHandle(s32 lib_ctx_id) {
    return createHandle(lib_ctx_id);
}

s32 sceNpWebApiCreateMultipartRequest(s32 title_user_ctx_id, const char* api_group,
                                      const char* path, OrbisNpWebApiHttpMethod method,
                                      s64* request_id) {
    if (api_group == nullptr || path == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    if (getCompiledSdkVersion() >= Common::ElfInfo::FW_250 &&
        method > OrbisNpWebApiHttpMethod::ORBIS_NP_WEBAPI_HTTP_METHOD_DELETE) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    LOG_INFO(Lib_NpWebApi,
             "called, title_user_ctx_id = {:#x}, api_group = '{}', path = '{}', method = {}",
             title_user_ctx_id, api_group, path, magic_enum::enum_name(method));

    return createRequest(title_user_ctx_id, api_group, path, method, nullptr, nullptr, request_id,
                         true);
}

s32 sceNpWebApiCreateRequest(s32 title_user_ctx_id, const char* api_group, const char* path,
                             OrbisNpWebApiHttpMethod method,
                             const OrbisNpWebApiContentParameter* content_parameter,
                             s64* request_id) {
    if (api_group == nullptr || path == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    if (content_parameter != nullptr && content_parameter->content_length != 0 &&
        content_parameter->content_type == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_CONTENT_PARAMETER;
    }

    if (getCompiledSdkVersion() >= Common::ElfInfo::FW_250 &&
        method > OrbisNpWebApiHttpMethod::ORBIS_NP_WEBAPI_HTTP_METHOD_DELETE) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    LOG_INFO(Lib_NpWebApi,
             "called, title_user_ctx_id = {:#x}, api_group = '{}', path = '{}', method = {}",
             title_user_ctx_id, api_group, path, magic_enum::enum_name(method));

    return createRequest(title_user_ctx_id, api_group, path, method, content_parameter, nullptr,
                         request_id, false);
}

s32 sceNpWebApiDeleteContext(s32 title_user_ctx_id) {
    LOG_INFO(Lib_NpWebApi, "called title_user_ctx_id = {:#x}", title_user_ctx_id);
    return deleteUserContext(title_user_ctx_id);
}

s32 sceNpWebApiDeleteExtdPushEventFilter(s32 lib_ctx_id, s32 filter_id) {
    LOG_INFO(Lib_NpWebApi, "called lib_ctx_id = {:#x}, filter_id = {:#x}", lib_ctx_id, filter_id);
    return deleteExtendedPushEventFilter(lib_ctx_id, filter_id);
}

s32 sceNpWebApiDeleteHandle(s32 lib_ctx_id, s32 handle_id) {
    LOG_INFO(Lib_NpWebApi, "called lib_ctx_id = {:#x}, handle_id = {:#x}", lib_ctx_id, handle_id);
    return deleteHandle(lib_ctx_id, handle_id);
}

s32 sceNpWebApiDeleteRequest(s64 request_id) {
    LOG_INFO(Lib_NpWebApi, "called request_id = {:#x}", request_id);
    return deleteRequest(request_id);
}

s32 sceNpWebApiGetConnectionStats(s32 user_ctx_id, const char* api_group,
                                  OrbisNpWebApiConnectionStats* stats) {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpWebApiGetErrorCode() {
    const s32 code = getLastWebApiError();
    LOG_INFO(Lib_NpWebApi, "called, last_error_code = {:#x}", code);
    return code;
}

s32 sceNpWebApiGetHttpResponseHeaderValue(s64 request_id, const char* field_name, char* value,
                                          u64 value_size) {
    LOG_INFO(Lib_NpWebApi, "called, request_id = {:#x}, field_name = '{}', value_size = {}",
             request_id, (field_name ? field_name : "null"), value_size);
    if (field_name == nullptr || value == nullptr || value_size == 0) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    return getHttpResponseHeaderValueInternal(request_id, field_name, value, value_size, nullptr);
}

s32 sceNpWebApiGetHttpResponseHeaderValueLength(s64 request_id, const char* field_name,
                                                u64* value_length) {
    LOG_INFO(Lib_NpWebApi, "called, request_id = {:#x}, field_name = '{}'", request_id,
             (field_name ? field_name : "null"));
    if (field_name == nullptr || value_length == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    return getHttpResponseHeaderValueInternal(request_id, field_name, nullptr, 0, value_length);
}

s32 sceNpWebApiGetHttpStatusCode(s64 request_id, s32* out_status_code) {
    LOG_INFO(Lib_NpWebApi, "called, request_id = {:#x}", request_id);
    // On newer SDKs, NULL output pointer is invalid
    if (getCompiledSdkVersion() > Common::ElfInfo::FW_100 && out_status_code == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    s32 returncode = getHttpStatusCodeInternal(request_id, out_status_code);
    return returncode;
}

s32 sceNpWebApiGetMemoryPoolStats(s32 lib_ctx_id, OrbisNpWebApiMemoryPoolStats* current_stat) {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpWebApiInitialize(s32 lib_http_ctx_id, u64 pool_size) {
    LOG_INFO(Lib_NpWebApi, "called, lib_http_ctx_id = {:#x}, pool_size = {:#x} bytes",
             lib_http_ctx_id, pool_size);
    if (!g_is_initialized) {
        g_is_initialized = true;
        s32 result = initializeLibrary();
        if (result < ORBIS_OK) {
            return result;
        }
    }

    s32 result = createLibraryContext(lib_http_ctx_id, pool_size, nullptr, 0);
    if (result >= ORBIS_OK) {
        g_active_library_contexts++;
    }
    return result;
}

s32 sceNpWebApiInitializeForPresence(s32 lib_http_ctx_id, u64 pool_size) {
    LOG_INFO(Lib_NpWebApi, "called, lib_http_ctx_id = {:#x}, pool_size = {:#x} bytes",
             lib_http_ctx_id, pool_size);
    if (!g_is_initialized) {
        g_is_initialized = true;
        s32 result = initializeLibrary();
        if (result < ORBIS_OK) {
            return result;
        }
    }

    s32 result = createLibraryContext(lib_http_ctx_id, pool_size, nullptr, 3);
    if (result >= ORBIS_OK) {
        g_active_library_contexts++;
    }
    return result;
}

s32 sceNpWebApiIntCreateCtxIndExtdPushEventFilter(
    s32 lib_ctx_id, s32 handle_id, const OrbisNpWebApiExtdPushEventFilterParameter* filter_param,
    u64 filter_param_num) {
    if (filter_param == nullptr || filter_param_num == 0) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    LOG_INFO(Lib_NpWebApi, "called, lib_ctx_id = {:#x}, handle_id = {:#x}", lib_ctx_id, handle_id);
    return createExtendedPushEventFilter(lib_ctx_id, handle_id, nullptr, -1, filter_param,
                                         filter_param_num, true);
}

s32 sceNpWebApiIntCreateRequest(s32 title_user_ctx_id, const char* api_group, const char* path,
                                OrbisNpWebApiHttpMethod method,
                                const OrbisNpWebApiContentParameter* content_parameter,
                                const OrbisNpWebApiIntCreateRequestExtraArgs* internal_args,
                                s64* request_id) {
    LOG_INFO(Lib_NpWebApi, "called");
    if (api_group == nullptr || path == nullptr ||
        method > OrbisNpWebApiHttpMethod::ORBIS_NP_WEBAPI_HTTP_METHOD_PATCH) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    if (content_parameter != nullptr && content_parameter->content_length != 0 &&
        content_parameter->content_type == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_CONTENT_PARAMETER;
    }

    LOG_INFO(Lib_NpWebApi,
             "called, title_user_ctx_id = {:#x}, api_group = '{}', path = '{}', method = {}",
             title_user_ctx_id, api_group, path, magic_enum::enum_name(method));

    return createRequest(title_user_ctx_id, api_group, path, method, content_parameter,
                         internal_args, request_id, false);
}

s32 sceNpWebApiIntCreateServicePushEventFilter(
    s32 lib_ctx_id, s32 handle_id, const char* np_service_name, u32 np_service_label,
    const OrbisNpWebApiServicePushEventFilterParameter* filter_param, u64 filter_param_num) {
    if (filter_param == nullptr || filter_param_num == 0) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_WARNING(Lib_NpWebApi,
                "called, lib_ctx_id = {:#x}, handle_id = {:#x}, np_service_name = '{}', "
                "np_service_label = {:#x}",
                lib_ctx_id, handle_id, (np_service_name ? np_service_name : "null"),
                np_service_label);
    return createServicePushEventFilter(lib_ctx_id, handle_id, np_service_name, np_service_label,
                                        filter_param, filter_param_num);
}

s32 sceNpWebApiIntInitialize(const OrbisNpWebApiInitializeParameter* args) {
    LOG_INFO(Lib_NpWebApi, "called");
    if (args == nullptr || args->struct_size != sizeof(OrbisNpWebApiInitializeParameter)) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    if (!g_is_initialized) {
        g_is_initialized = true;
        s32 result = initializeLibrary();
        if (result < ORBIS_OK) {
            return result;
        }
    }

    s32 result = createLibraryContext(args->lib_http_ctx_id, args->pool_size, args->name, 2);
    if (result >= ORBIS_OK) {
        g_active_library_contexts++;
    }
    return result;
}

s32 sceNpWebApiIntRegisterServicePushEventCallback(
    s32 title_user_ctx_id, s32 filter_id, OrbisNpWebApiInternalServicePushEventCallback cb_func,
    void* user_arg) {
    if (cb_func == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi, "called, title_user_ctx_id = {:#x}, cb_func = {}", title_user_ctx_id,
             reinterpret_cast<void*>(cb_func));
    return registerServicePushEventCallback(title_user_ctx_id, filter_id, nullptr, cb_func, nullptr,
                                            user_arg);
}

s32 sceNpWebApiIntRegisterServicePushEventCallbackA(
    s32 title_user_ctx_id, s32 filter_id, OrbisNpWebApiInternalServicePushEventCallbackA cb_func,
    void* user_arg) {
    if (cb_func == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi, "called, title_user_ctx_id = {:#x}, cb_func = {}", title_user_ctx_id,
             reinterpret_cast<void*>(cb_func));
    return registerServicePushEventCallback(title_user_ctx_id, filter_id, nullptr, nullptr, cb_func,
                                            user_arg);
}

s32 sceNpWebApiReadData(s64 request_id, void* data, u64 size) {
    LOG_INFO(Lib_NpWebApi, "called, request_id = {:#x}, data = {}, size = {:#x}", request_id,
             fmt::ptr(data), size);
    if (data == nullptr || size == 0) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    return readDataInternal(request_id, data, size);
}

s32 sceNpWebApiRegisterExtdPushEventCallbackA(s32 title_user_ctx_id, s32 filter_id,
                                              OrbisNpWebApiExtdPushEventCallbackA cb_func,
                                              void* user_arg) {
    if (cb_func == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi, "called, title_user_ctx_id = {:#x}, cb_func = {}", title_user_ctx_id,
             reinterpret_cast<void*>(cb_func));
    return registerExtdPushEventCallbackA(title_user_ctx_id, filter_id, cb_func, user_arg);
}

s32 sceNpWebApiSendMultipartRequest(s64 request_id, s32 part_index, const void* data,
                                    u64 data_size) {
    if (part_index <= 0 || data == nullptr || data_size == 0) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    LOG_INFO(Lib_NpWebApi,
             "called, request_id = {:#x}, "
             "part_index = {:#x}, data = {}, data_size = {:#x}",
             request_id, part_index, fmt::ptr(data), data_size);
    return sendRequest(request_id, part_index, data, data_size, 0, nullptr);
}

s32 sceNpWebApiSendMultipartRequest2(s64 request_id, s32 part_index, const void* data,
                                     u64 data_size,
                                     OrbisNpWebApiResponseInformationOption* resp_info_option) {
    if (part_index <= 0 || data == nullptr || data_size == 0) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    LOG_INFO(Lib_NpWebApi,
             "called, request_id = {:#x}, "
             "part_index = {:#x}, data = {}, data_size = {:#x}, resp_info_option = {}",
             request_id, part_index, fmt::ptr(data), data_size, fmt::ptr(resp_info_option));
    return sendRequest(request_id, part_index, data, data_size, 1, resp_info_option);
}

s32 sceNpWebApiSendRequest(s64 request_id, const void* data, u64 data_size) {
    LOG_INFO(Lib_NpWebApi, "called, request_id = {:#x}, data = {}, data_size = {:#x}", request_id,
             fmt::ptr(data), data_size);
    return sendRequest(request_id, 0, data, data_size, 0, nullptr);
}

s32 sceNpWebApiSendRequest2(s64 request_id, const void* data, u64 data_size,
                            OrbisNpWebApiResponseInformationOption* resp_info_option) {
    LOG_INFO(Lib_NpWebApi,
             "called, request_id = {:#x}, "
             "data = {}, data_size = {:#x}, resp_info_option = {}",
             request_id, fmt::ptr(data), data_size, fmt::ptr(resp_info_option));
    return sendRequest(request_id, 0, data, data_size, 1, resp_info_option);
}

s32 sceNpWebApiSetHandleTimeout(s32 lib_ctx_id, s32 handle_id, u32 timeout) {
    LOG_INFO(Lib_NpWebApi, "called, lib_ctx_id = {:#x}, handle_id = {:#x}, timeout = {} ms",
             lib_ctx_id, handle_id, timeout);
    return setHandleTimeout(lib_ctx_id, handle_id, timeout);
}

s32 sceNpWebApiSetMaxConnection(s32 lib_ctx_id, s32 max_connection) {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpWebApiSetMultipartContentType(s64 request_id, const char* type_name,
                                       const char* boundary) {
    LOG_INFO(Lib_NpWebApi, "called, request_id = {:#x}, type_name = '{}', boundary = '{}'",
             request_id, (type_name ? type_name : "null"), (boundary ? boundary : "null"));
    return setMultipartContentType(request_id, type_name, boundary);
}

s32 sceNpWebApiSetRequestTimeout(s64 request_id, u32 timeout) {
    LOG_INFO(Lib_NpWebApi, "called, request_id = {:#x}, timeout = {} ms", request_id, timeout);
    return setRequestTimeout(request_id, timeout);
}

s32 sceNpWebApiTerminate(s32 lib_ctx_id) {
    LOG_INFO(Lib_NpWebApi, "called, lib_ctx_id = {:#x}", lib_ctx_id);
    s32 result = terminateContext(lib_ctx_id);
    if (result != ORBIS_OK) {
        return result;
    }

    g_active_library_contexts--;
    if (g_active_library_contexts == 0) {
        g_is_initialized = false;
    }
    return ORBIS_OK;
}

s32 sceNpWebApiUnregisterExtdPushEventCallback(s32 title_user_ctx_id, s32 callback_id) {
    LOG_INFO(Lib_NpWebApi, "called, title_user_ctx_id = {:#x}, callback_id = {:#x}",
             title_user_ctx_id, callback_id);
    return unregisterExtdPushEventCallback(title_user_ctx_id, callback_id);
}

s32 sceNpWebApiVshInitialize(s32 lib_http_ctx_id, u64 pool_size) {
    LOG_INFO(Lib_NpWebApi, "called, lib_http_ctx_id = {:#x}, pool_size = {:#x} bytes",
             lib_http_ctx_id, pool_size);
    if (!g_is_initialized) {
        g_is_initialized = true;
        s32 result = initializeLibrary();
        if (result < ORBIS_OK) {
            return result;
        }
    }

    s32 result = createLibraryContext(lib_http_ctx_id, pool_size, nullptr, 4);
    if (result >= ORBIS_OK) {
        g_active_library_contexts++;
    }
    return result;
}

void RegisterHooks() {
    RegisterNpWebApiHooks();
    NpManager::RegisterNpCallback("npwebapi_push", DrainPushEvents);
};

} // namespace Libraries::Np::NpWebApi
