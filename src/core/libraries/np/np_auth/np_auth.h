// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/user_service/user_service.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpAuth {

constexpr s32 ORBIS_NP_AUTH_REQUEST_LIMIT = 0x10;
constexpr s32 ORBIS_NP_AUTH_REQUEST_ID_OFFSET = 0x10000000;

struct OrbisNpAuthCreateAsyncRequestParameter {
    u64 size;
    u64 cpu_affinity_mask;
    s32 thread_priority;
    u8 padding[4];
};

struct OrbisNpAuthGetAuthorizationCodeParameter {
    u64 size;
    const OrbisNpOnlineId* online_id;
    const OrbisNpClientId* client_id;
    const char* scope;
};

struct OrbisNpAuthGetAuthorizationCodeParameterA {
    u64 size;
    UserService::OrbisUserServiceUserId user_id;
    u8 padding[4];
    const OrbisNpClientId* client_id;
    const char* scope;
};

struct OrbisNpAuthGetIdTokenParameter {
    u64 size;
    const OrbisNpOnlineId* online_id;
    const OrbisNpClientId* client_id;
    const OrbisNpClientSecret* client_secret;
    const char* scope;
};

struct OrbisNpAuthGetIdTokenParameterA {
    u64 size;
    UserService::OrbisUserServiceUserId user_id;
    u8 padding[4];
    const OrbisNpClientId* client_id;
    const OrbisNpClientSecret* client_secret;
    const char* scope;
};

s32 sceNpAuthCreateRequest();
s32 sceNpAuthCreateAsyncRequest(const OrbisNpAuthCreateAsyncRequestParameter* param);
s32 sceNpAuthGetAuthorizationCode(s32 req_id, const OrbisNpAuthGetAuthorizationCodeParameter* param,
                                  OrbisNpAuthorizationCode* auth_code, s32* issuer_id);
s32 sceNpAuthGetAuthorizationCodeA(s32 req_id,
                                   const OrbisNpAuthGetAuthorizationCodeParameterA* param,
                                   OrbisNpAuthorizationCode* auth_code, s32* issuer_id);
s32 sceNpAuthGetAuthorizationCodeV3(s32 req_id,
                                    const OrbisNpAuthGetAuthorizationCodeParameterA* param,
                                    OrbisNpAuthorizationCode* auth_code, s32* issuer_id);
s32 sceNpAuthSetTimeout(s32 req_id, s32 resolve_retry, u32 resolve_timeout, u32 conn_timeout,
                        u32 send_timeout, u32 recv_timeout);
s32 sceNpAuthAbortRequest(s32 req_id);
s32 sceNpAuthWaitAsync(s32 req_id, s32* result);
s32 sceNpAuthPollAsync(s32 req_id, s32* result);
s32 sceNpAuthDeleteRequest(s32 req_id);
s32 sceNpAuthGetIdToken(s32 req_id, const OrbisNpAuthGetIdTokenParameter* param,
                        OrbisNpIdToken* token);
s32 sceNpAuthGetIdTokenA(s32 req_id, const OrbisNpAuthGetIdTokenParameterA* param,
                         OrbisNpIdToken* token);
s32 sceNpAuthGetIdTokenV3(s32 req_id, const OrbisNpAuthGetIdTokenParameterA* param,
                          OrbisNpIdToken* token);

void RegisterHooks();
} // namespace Libraries::Np::NpAuth