// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::UserService {

using OrbisUserServiceUserId = s32;

struct OrbisUserServiceInitializeParams {
    s32 priority;
};

struct OrbisUserServiceLoginUserIdList {
    OrbisUserServiceUserId user_id[4];
};

struct OrbisUserServiceRegisteredUserIdList {
    OrbisUserServiceUserId user_id[16];
};

enum class OrbisUserServiceUserColor {
    Blue = 0,
    Red = 1,
    Green = 2,
    Pink = 3,
};

enum class OrbisUserServiceEventType {
    Login = 0,
    Logout = 1,
};

struct OrbisUserServiceEvent {
    OrbisUserServiceEventType event;
    OrbisUserServiceUserId user_id;
};

s32 sceUserServiceInitialize(const OrbisUserServiceInitializeParams* params);
s32 sceUserServiceInitialize2(s32 thread_prio, u64 cpu_mask);
s32 sceUserServiceTerminate();
s32 sceUserServiceGetUserName(s32 user_id, char* user_name, u64 name_len);
s32 sceUserServiceGetEvent(OrbisUserServiceEvent* event);
void RegisterHooks();
} // namespace Libraries::UserService
