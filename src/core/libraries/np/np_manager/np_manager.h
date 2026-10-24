// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/user_service/user_service.h"

namespace Libraries::Np::NpManager {
s32 sceNpGetState(s32 user_id, OrbisNpState* state);
s32 sceNpGetNpId(s32 user_id, OrbisNpId* np_id);
s32 sceNpGetOnlineId(s32 user_id, OrbisNpOnlineId* online_id);

void RegisterNpCallback(std::string key, std::function<void()> cb);
void QueueNpStateEvent(s32 user_id, OrbisNpState state);
void UpdateNpStateFromEvent(UserService::OrbisUserServiceEventType event_type, s32 user_id);
void RegisterHooks();
} // namespace Libraries::Np::NpManager