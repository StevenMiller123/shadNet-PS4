// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <orbis/UserService.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/np/np_auth/np_auth.h"
#include "core/libraries/np/np_auth/np_auth_ctx.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/user_service/user_service.h"

extern "C" {
void sceNpAuthCreateRequest();
void sceNpAuthCreateAsyncRequest();
void sceNpAuthGetAuthorizationCode();
void sceNpAuthGetAuthorizationCodeA();
void sceNpAuthGetAuthorizationCodeV3();
void sceNpAuthGetIdToken();
void sceNpAuthGetIdTokenA();
void sceNpAuthGetIdTokenV3();
void sceNpAuthSetTimeout();
void sceNpAuthAbortRequest();
void sceNpAuthWaitAsync();
void sceNpAuthPollAsync();
void sceNpAuthDeleteRequest();
};

SHADNET_HOOK_DECLARE(Libraries::Np::NpAuth, sceNpAuthCreateRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpAuth, sceNpAuthCreateAsyncRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpAuth, sceNpAuthGetAuthorizationCode);
SHADNET_HOOK_DECLARE(Libraries::Np::NpAuth, sceNpAuthGetAuthorizationCodeA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpAuth, sceNpAuthGetAuthorizationCodeV3);
SHADNET_HOOK_DECLARE(Libraries::Np::NpAuth, sceNpAuthGetIdToken);
SHADNET_HOOK_DECLARE(Libraries::Np::NpAuth, sceNpAuthGetIdTokenA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpAuth, sceNpAuthGetIdTokenV3);
SHADNET_HOOK_DECLARE(Libraries::Np::NpAuth, sceNpAuthSetTimeout);
SHADNET_HOOK_DECLARE(Libraries::Np::NpAuth, sceNpAuthAbortRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpAuth, sceNpAuthWaitAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpAuth, sceNpAuthPollAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpAuth, sceNpAuthDeleteRequest);

static void RegisterNpAuthHooks() {
    SHADNET_HOOK(Libraries::Np::NpAuth, sceNpAuthCreateRequest);
    SHADNET_HOOK(Libraries::Np::NpAuth, sceNpAuthCreateAsyncRequest);
    SHADNET_HOOK(Libraries::Np::NpAuth, sceNpAuthGetAuthorizationCode);
    SHADNET_HOOK(Libraries::Np::NpAuth, sceNpAuthGetAuthorizationCodeA);
    SHADNET_HOOK(Libraries::Np::NpAuth, sceNpAuthGetAuthorizationCodeV3);
    SHADNET_HOOK(Libraries::Np::NpAuth, sceNpAuthGetIdToken);
    SHADNET_HOOK(Libraries::Np::NpAuth, sceNpAuthGetIdTokenA);
    SHADNET_HOOK(Libraries::Np::NpAuth, sceNpAuthGetIdTokenV3);
    SHADNET_HOOK(Libraries::Np::NpAuth, sceNpAuthSetTimeout);
    SHADNET_HOOK(Libraries::Np::NpAuth, sceNpAuthAbortRequest);
    SHADNET_HOOK(Libraries::Np::NpAuth, sceNpAuthWaitAsync);
    SHADNET_HOOK(Libraries::Np::NpAuth, sceNpAuthPollAsync);
    SHADNET_HOOK(Libraries::Np::NpAuth, sceNpAuthDeleteRequest);
}

namespace Libraries::Np::NpAuth {

static bool g_shadnet_enabled = false;
static std::mutex g_auth_request_mutex;
static constexpr s32 ORBIS_NP_AUTH_POLL_ASYNC_RET_FINISHED = 0;
static constexpr s32 ORBIS_NP_AUTH_POLL_ASYNC_RET_RUNNING = 1;
static constexpr u32 ORBIS_NP_AUTH_TIMEOUT_NO_EFFECT = 0;

struct NpAuthRequest {
    bool async = false;
    bool aborted = false;
    std::shared_ptr<AuthRequestCtx> ctx;
};

static std::map<s32, NpAuthRequest> g_auth_requests;

s32 CreateNpAuthRequest(bool async) {
    std::scoped_lock lk{g_auth_request_mutex};
    if (g_auth_requests.size() == ORBIS_NP_AUTH_REQUEST_LIMIT) {
        LOG_ERROR(Lib_NpAuth, "request limit reached, cannot create new request");
        return ORBIS_NP_AUTH_ERROR_REQUEST_MAX;
    }

    s32 req_id = ORBIS_NP_AUTH_REQUEST_ID_OFFSET + 1;
    while (g_auth_requests.contains(req_id)) {
        req_id++;
    }
    g_auth_requests[req_id].async = async;
    LOG_DEBUG(Lib_NpAuth, "called, async = {}", async);
    return req_id;
}

s32 sceNpAuthCreateRequest() {
    return CreateNpAuthRequest(false);
}

s32 sceNpAuthCreateAsyncRequest(const OrbisNpAuthCreateAsyncRequestParameter* param) {
    if (param == nullptr) {
        LOG_ERROR(Lib_NpAuth, "called with null parameter");
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (param->size != sizeof(OrbisNpAuthCreateAsyncRequestParameter)) {
        LOG_ERROR(Lib_NpAuth, "called with invalid parameter size: {}", param->size);
        return ORBIS_NP_AUTH_ERROR_INVALID_SIZE;
    }

    return CreateNpAuthRequest(true);
}

static s32 FindRequest(s32 req_id, NpAuthRequest** request) {
    if (req_id <= 0) {
        LOG_ERROR(Lib_NpAuth, "called with invalid request id: {}", req_id);
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    const auto it = g_auth_requests.find(req_id);
    if (it == g_auth_requests.end()) {
        LOG_ERROR(Lib_NpAuth, "request not found: {}", req_id);
        return ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND;
    }
    *request = &it->second;
    return ORBIS_OK;
}

static s32 StartRequest(s32 req_id, bool* async, std::shared_ptr<AuthRequestCtx>* ctx) {
    std::scoped_lock lk{g_auth_request_mutex};
    NpAuthRequest* request = nullptr;
    if (const s32 error = FindRequest(req_id, &request); error != ORBIS_OK) {
        return error;
    }
    if (request->ctx) {
        LOG_ERROR(Lib_NpAuth, "request already started: {}", req_id);
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (request->aborted) {
        LOG_ERROR(Lib_NpAuth, "request already aborted: {}", req_id);
        return ORBIS_NP_AUTH_ERROR_ABORTED;
    }
    request->ctx = std::make_shared<AuthRequestCtx>();
    *async = request->async;
    *ctx = request->ctx;
    return ORBIS_OK;
}

static s32 FinishStartedRequest(bool async, const std::shared_ptr<AuthRequestCtx>& ctx) {
    return async ? ORBIS_OK : ctx->Wait();
}

s32 GetAuthorizationCode(s32 req_id, const OrbisNpAuthGetAuthorizationCodeParameterA* param,
                         s32 flag, OrbisNpAuthorizationCode* auth_code, s32* issuer_id) {
    if (param == nullptr || auth_code == nullptr) {
        LOG_ERROR(Lib_NpAuth, "called with null parameter");
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (param->size != sizeof(OrbisNpAuthGetAuthorizationCodeParameterA)) {
        LOG_ERROR(Lib_NpAuth, "called with invalid parameter size: {}", param->size);
        return ORBIS_NP_AUTH_ERROR_INVALID_SIZE;
    }
    if (param->client_id == nullptr || param->scope == nullptr) {
        LOG_ERROR(Lib_NpAuth, "called with null parameter");
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }

    OrbisUserServiceLoginUserIdList user_list{};
    s32 result = sceUserServiceGetLoginUserIdList(&user_list);
    if (result != 0) {
        LOG_ERROR(Lib_NpAuth, "failed to retrieve user list: {:#x}", result);
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    } else {
        bool user_found = false;
        for (s32 i = 0; i < ORBIS_USER_SERVICE_MAX_LOGIN_USERS; i++) {
            if (user_list.userId[i] == param->user_id) {
                user_found = true;
                break;
            }
        }
        if (!user_found) {
            LOG_ERROR(Lib_NpAuth, "could not find user for user id {}", param->user_id);
            return ORBIS_NP_ERROR_USER_NOT_FOUND;
        }
    }

    bool async = false;
    std::shared_ptr<AuthRequestCtx> ctx;
    if (const s32 error = StartRequest(req_id, &async, &ctx); error != ORBIS_OK) {
        return error;
    }

    if (!g_shadnet_enabled) {
        ctx->SetResult(ORBIS_NP_ERROR_SIGNED_OUT);
        return FinishStartedRequest(async, ctx);
    }

    const std::string client_id(param->client_id->id,
                                strnlen(param->client_id->id, sizeof(param->client_id->id)));
    const std::string scope = param->scope;
    LOG_INFO(Lib_NpAuth, "req_id = {:#x}, async = {}, client_id = '{}', scope = '{}'", req_id,
             async, client_id, scope);

    ctx->authCode = auth_code;
    ctx->issuerId = issuer_id;
    const s32 submit =
        NpHandler::GetInstance().RequestAuthorizationCode(param->user_id, client_id, scope, ctx);
    if (submit < 0) {
        ctx->SetResult(submit);
    }
    return FinishStartedRequest(async, ctx);
}

s32 sceNpAuthGetAuthorizationCode(s32 req_id, const OrbisNpAuthGetAuthorizationCodeParameter* param,
                                  OrbisNpAuthorizationCode* auth_code, s32* issuer_id) {
    if (param == nullptr || auth_code == nullptr) {
        LOG_ERROR(Lib_NpAuth, "called with null parameter");
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (param->size != sizeof(OrbisNpAuthGetAuthorizationCodeParameter)) {
        LOG_ERROR(Lib_NpAuth, "called with invalid parameter size: {}", param->size);
        return ORBIS_NP_AUTH_ERROR_INVALID_SIZE;
    }
    if (param->online_id == nullptr || param->client_id == nullptr || param->scope == nullptr) {
        LOG_ERROR(Lib_NpAuth, "called with null parameter");
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (!g_shadnet_enabled) {
        LOG_ERROR(Lib_NpAuth, "shadnet is not enabled , user not found");
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }

    const s32 user_id = NpHandler::GetInstance().GetUserIdByOnlineId(*param->online_id);
    if (user_id < 0) {
        LOG_ERROR(Lib_NpAuth, "user not found for online_id");
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    OrbisNpAuthGetAuthorizationCodeParameterA internal_params;
    std::memset(&internal_params, 0, sizeof(internal_params));
    internal_params.size = sizeof(internal_params);
    internal_params.client_id = param->client_id;
    internal_params.user_id = user_id;
    internal_params.scope = param->scope;

    return GetAuthorizationCode(req_id, &internal_params, 0, auth_code, issuer_id);
}

s32 sceNpAuthGetAuthorizationCodeA(s32 req_id,
                                   const OrbisNpAuthGetAuthorizationCodeParameterA* param,
                                   OrbisNpAuthorizationCode* auth_code, s32* issuer_id) {
    return GetAuthorizationCode(req_id, param, 0, auth_code, issuer_id);
}

s32 sceNpAuthGetAuthorizationCodeV3(s32 req_id,
                                    const OrbisNpAuthGetAuthorizationCodeParameterA* param,
                                    OrbisNpAuthorizationCode* auth_code, s32* issuer_id) {
    return GetAuthorizationCode(req_id, param, 1, auth_code, issuer_id);
}

s32 sceNpAuthSetTimeout(s32 req_id, s32 resolve_retry, u32 resolve_timeout, u32 conn_timeout,
                        u32 send_timeout, u32 recv_timeout) {
    LOG_DEBUG(Lib_NpAuth,
              "called req_id = {:#x}, resolve_retry = {}, resolve_timeout = {}, conn_timeout = {}, "
              "send_timeout = {}, recv_timeout = {}",
              req_id, resolve_retry, resolve_timeout, conn_timeout, send_timeout, recv_timeout);
    constexpr u32 MinResolveTimeoutUs = 1000000;
    constexpr u32 MinTransferTimeoutUs = 10000000;
    const auto valid = [](u32 value, u32 min) {
        return value == ORBIS_NP_AUTH_TIMEOUT_NO_EFFECT || value >= min;
    };
    if (resolve_retry < 0 || !valid(resolve_timeout, MinResolveTimeoutUs) ||
        !valid(conn_timeout, MinTransferTimeoutUs) || !valid(send_timeout, MinTransferTimeoutUs) ||
        !valid(recv_timeout, MinTransferTimeoutUs) ||
        (resolve_retry == 0 && resolve_timeout == 0 && conn_timeout == 0 && send_timeout == 0 &&
         recv_timeout == 0)) {
        LOG_ERROR(Lib_NpAuth, "invalid timeout values");
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{g_auth_request_mutex};
    NpAuthRequest* request = nullptr;
    return FindRequest(req_id, &request);
}

s32 sceNpAuthAbortRequest(s32 req_id) {
    LOG_DEBUG(Lib_NpAuth, "called req_id = {:#x}", req_id);

    std::scoped_lock lk{g_auth_request_mutex};
    NpAuthRequest* request = nullptr;
    if (const s32 error = FindRequest(req_id, &request); error != ORBIS_OK) {
        return error;
    }
    if (request->ctx) {
        request->ctx->SetResult(ORBIS_NP_AUTH_ERROR_ABORTED);
    } else {
        request->aborted = true;
    }
    return ORBIS_OK;
}

static s32 FindAsyncRequest(s32 req_id, std::shared_ptr<AuthRequestCtx>* ctx) {
    std::scoped_lock lk{g_auth_request_mutex};
    NpAuthRequest* request = nullptr;
    if (const s32 error = FindRequest(req_id, &request); error != ORBIS_OK) {
        return error;
    }
    if (!request->async || !request->ctx) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ID;
    }
    *ctx = request->ctx;
    return ORBIS_OK;
}

s32 sceNpAuthWaitAsync(s32 req_id, s32* result) {
    if (result == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }

    std::shared_ptr<AuthRequestCtx> ctx;
    if (const s32 error = FindAsyncRequest(req_id, &ctx); error != ORBIS_OK) {
        return error;
    }
    *result = ctx->Wait();
    LOG_DEBUG(Lib_NpAuth, "called req_id = {:#x}, returning result = {:#x}", req_id,
              static_cast<u32>(*result));
    return ORBIS_OK;
}

s32 sceNpAuthPollAsync(s32 req_id, s32* result) {
    if (result == nullptr) {
        LOG_ERROR(Lib_NpAuth, "called with null result pointer");
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }

    std::shared_ptr<AuthRequestCtx> ctx;
    if (const s32 error = FindAsyncRequest(req_id, &ctx); error != ORBIS_OK) {
        return error;
    }
    std::scoped_lock lock{ctx->mutex};
    if (!ctx->result.has_value()) {
        return ORBIS_NP_AUTH_POLL_ASYNC_RET_RUNNING;
    }
    *result = *ctx->result;
    LOG_DEBUG(Lib_NpAuth, "called req_id = {:#x}, returning result = {:#x}", req_id,
              static_cast<u32>(*result));
    return ORBIS_NP_AUTH_POLL_ASYNC_RET_FINISHED;
}

s32 sceNpAuthDeleteRequest(s32 req_id) {
    LOG_DEBUG(Lib_NpAuth, "called req_id = {:#x}", req_id);

    std::scoped_lock lk{g_auth_request_mutex};
    NpAuthRequest* request = nullptr;
    if (const s32 error = FindRequest(req_id, &request); error != ORBIS_OK) {
        return error;
    }
    if (request->ctx) {
        request->ctx->SetResult(ORBIS_NP_AUTH_ERROR_ABORTED);
    }
    g_auth_requests.erase(req_id);
    return ORBIS_OK;
}

// TODO token code taken from old np auth implementation, needs to be reworked to use shadnet and
// np handler
s32 GetIdToken(s32 req_id, const OrbisNpAuthGetIdTokenParameterA* param, s32 flag,
               OrbisNpIdToken* token) {
    if (param == nullptr || token == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (param->size != sizeof(OrbisNpAuthGetIdTokenParameterA)) {
        return ORBIS_NP_AUTH_ERROR_INVALID_SIZE;
    }
    if (param->user_id == -1 || param->client_id == nullptr || param->client_secret == nullptr ||
        param->scope == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }

    bool async = false;
    std::shared_ptr<AuthRequestCtx> ctx;
    if (const s32 error = StartRequest(req_id, &async, &ctx); error != ORBIS_OK) {
        return error;
    }

    if (!g_shadnet_enabled) {
        ctx->SetResult(ORBIS_NP_ERROR_SIGNED_OUT);
        return FinishStartedRequest(async, ctx);
    }

    LOG_ERROR(Lib_NpAuth, "(STUBBED) called, req_id = {:#x}, async = {}", req_id, async);

    // Not sure what values are expected here, so zeroing this for now.
    std::memset(token, 0, sizeof(OrbisNpIdToken));
    std::strncpy(token->token, "TOKEN", 6);
    ctx->SetResult(ORBIS_OK);
    return FinishStartedRequest(async, ctx);
}

s32 sceNpAuthGetIdToken(s32 req_id, const OrbisNpAuthGetIdTokenParameter* param,
                        OrbisNpIdToken* token) {
    if (param == nullptr || token == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (param->size != sizeof(OrbisNpAuthGetIdTokenParameter)) {
        return ORBIS_NP_AUTH_ERROR_INVALID_SIZE;
    }
    if (param->online_id == nullptr || param->client_id == nullptr ||
        param->client_secret == nullptr || param->scope == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (!g_shadnet_enabled) {
        // Calls sceNpManagerIntGetUserIdByOnlineId to get a user id, returning any errors.
        // This call will not succeed while signed out because games cannot retrieve an online id.
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }

    // For simplicity, call sceUserServiceGetInitialUser to get the user_id.
    s32 user_id = 0;
    sceUserServiceGetInitialUser(&user_id);

    // Library constructs an OrbisNpAuthGetIdTokenParameterA struct,
    // then uses that to call GetIdToken.
    OrbisNpAuthGetIdTokenParameterA internal_params;
    std::memset(&internal_params, 0, sizeof(internal_params));
    internal_params.size = sizeof(internal_params);
    internal_params.user_id = user_id;
    internal_params.client_id = param->client_id;
    internal_params.client_secret = param->client_secret;
    internal_params.scope = param->scope;

    return GetIdToken(req_id, &internal_params, 0, token);
}

s32 sceNpAuthGetIdTokenA(s32 req_id, const OrbisNpAuthGetIdTokenParameterA* param,
                         OrbisNpIdToken* token) {
    return GetIdToken(req_id, param, 0, token);
}

s32 sceNpAuthGetIdTokenV3(s32 req_id, const OrbisNpAuthGetIdTokenParameterA* param,
                          OrbisNpIdToken* token) {
    return GetIdToken(req_id, param, 1, token);
}

void RegisterHooks() {
    g_shadnet_enabled = NpHandler::Instance().IsActive();
    RegisterNpAuthHooks();
}

} // namespace Libraries::Np::NpAuth