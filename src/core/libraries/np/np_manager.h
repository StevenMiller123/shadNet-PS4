// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <orbis/NpManager.h>
#include "common/types.h"

namespace Libraries::Np::NpManager {
s32 sceNpGetState(s32 user_id, OrbisNpState* state);
s32 sceNpGetNpId(s32 user_id, OrbisNpId* np_id);
s32 sceNpGetOnlineId(s32 user_id, OrbisNpOnlineId* online_id);
void RegisterHooks();
} // namespace Libraries::Np::NpManager