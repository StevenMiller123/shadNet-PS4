// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::AppContent {

using OrbisAppContentAppParamId = u32;
using OrbisAppContentGameTrialsFlag = u32;

s32 sceAppContentAppParamGetInt(OrbisAppContentAppParamId param_id, s32* out_value);
s32 sceAppContentGetGameTrialsFlag(OrbisAppContentGameTrialsFlag* game_trials_flag);

void RegisterHooks();
} // namespace Libraries::AppContent