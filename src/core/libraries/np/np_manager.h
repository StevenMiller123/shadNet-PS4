// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::Np::NpManager {
s32 sceNpGetState(s32 user_id, OrbisNpState* state);
void RegisterHooks();
} // namespace Libraries::Np::NpManager