// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/user_service/user_service.h"

namespace Libraries::Np::NpManager {

constexpr s32 ORBIS_NP_MANAGER_REQUEST_LIMIT = 0x20;
constexpr s32 ORBIS_NP_MANAGER_REQUEST_ID_OFFSET = 0x20000000;

enum class OrbisNpReachabilityState {
    Unavailable = 0,
    Available = 1,
    Reachable = 2,
};
using OrbisNpReachabilityStateCallback = void (*)(s32 userId, OrbisNpReachabilityState state,
                                                  void* userdata);
using OrbisNpStateCallback = void (*)(s32 userId, OrbisNpState state, OrbisNpId* npId,
                                      void* userdata);
using OrbisNpStateCallbackA = void (*)(s32 userId, OrbisNpState state, void* userdata);
using OrbisNpStateCallbackForToolkit = void (*)(s32 userId, OrbisNpState state, void* userdata);

enum class OrbisNpGamePresenseStatus {
    Offline = 0,
    Online = 1,
};

struct OrbisNpCountryCode {
    char country_code[2];
    char end;
    char pad;
};

struct OrbisNpAgeRestriction {
    OrbisNpCountryCode country_code;
    s8 age;
    u8 padding[3];
};

struct OrbisNpContentRestriction {
    u64 size;
    s8 default_age_restriction;
    u8 padding[3];
    s32 age_restriction_count;
    const OrbisNpAgeRestriction* age_restriction;
};

struct OrbisNpDate {
    u16 year;
    u8 month;
    u8 day;
};

struct OrbisNpLanguageCode {
    char code[6];
    char padding[10];
};

struct OrbisNpParentalControlInfo {
    bool content_restriction;
    bool chat_restriction;
    bool user_generated_content_restriction;
};

struct OrbisNpCheckPlusParameter {
    u64 size;
    UserService::OrbisUserServiceUserId user_id;
    u8 padding[4];
    u64 features;
    u8 reserved[32];
};

struct OrbisNpCheckPlusResult {
    bool authorized;
    u8 reserved[32];
};

struct OrbisNpCreateAsyncRequestParameter {
    u64 size;
    u64 cpu_affinity_mask;
    s32 thread_priority;
    u8 padding[4];
};

// Callbacks
s32 sceNpRegisterNpReachabilityStateCallback(OrbisNpReachabilityStateCallback callback,
                                             void* userdata);
s32 sceNpUnregisterNpReachabilityStateCallback();
s32 sceNpRegisterStateCallback(OrbisNpStateCallback callback, void* userdata);
s32 sceNpUnregisterStateCallback();
s32 sceNpRegisterStateCallbackA(OrbisNpStateCallbackA callback, void* userdata);
s32 sceNpUnregisterStateCallbackA(s32 callback_id);
s32 sceNpRegisterStateCallbackForToolkit(OrbisNpStateCallbackForToolkit callback, void* userdata);
s32 sceNpUnregisterStateCallbackForToolkit();
s32 sceNpCheckCallback();
s32 sceNpCheckCallbackForLib();
void RegisterNpCallback(std::string key, std::function<void()> cb);

// State
s32 sceNpGetState(s32 user_id, OrbisNpState* state);
void QueueNpStateEvent(s32 user_id, OrbisNpState state);
void UpdateNpStateFromEvent(UserService::OrbisUserServiceEventType event_type, s32 user_id);

// Requests
s32 sceNpCreateRequest();
s32 sceNpCreateAsyncRequest(const OrbisNpCreateAsyncRequestParameter* param);
s32 sceNpCheckNpAvailability(s32 req_id, OrbisNpOnlineId* online_id);
s32 sceNpCheckNpAvailabilityA(s32 req_id, UserService::OrbisUserServiceUserId user_id);
s32 sceNpCheckNpReachability(s32 req_id, UserService::OrbisUserServiceUserId user_id);
s32 sceNpCheckPlus(s32 req_id, const OrbisNpCheckPlusParameter* param,
                   OrbisNpCheckPlusResult* result);
s32 sceNpGetAccountLanguage(s32 req_id, OrbisNpOnlineId* online_id, OrbisNpLanguageCode* language);
s32 sceNpGetAccountLanguageA(s32 req_id, UserService::OrbisUserServiceUserId user_id,
                             OrbisNpLanguageCode* language);
s32 sceNpGetParentalControlInfo(s32 req_id, OrbisNpOnlineId* online_id, s8* age,
                                OrbisNpParentalControlInfo* info);
s32 sceNpGetParentalControlInfoA(s32 req_id, UserService::OrbisUserServiceUserId user_id, s8* age,
                                 OrbisNpParentalControlInfo* info);
s32 sceNpAbortRequest(s32 req_id);
s32 sceNpSetTimeout(s32 req_id, s32 resolve_retry, u32 resolve_timeout, u32 conn_timeout,
                    u32 send_timeout, u32 recv_timeout);
s32 sceNpWaitAsync(s32 req_id, s32* result);
s32 sceNpPollAsync(s32 req_id, s32* result);
s32 sceNpDeleteRequest(s32 req_id);

// Getters
s32 sceNpGetNpId(s32 user_id, OrbisNpId* np_id);
s32 sceNpGetOnlineId(s32 user_id, OrbisNpOnlineId* online_id);

s32 GetFirmwareVersion();
void RegisterHooks();
} // namespace Libraries::Np::NpManager