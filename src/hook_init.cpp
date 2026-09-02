// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/NpManager.h>

#include "common/plugin_common.h"
#include "common/types.h"
#include "core/libraries/np/np_manager.h"
#include "hook_init.h"

HOOK_INIT(sceNpGetState);

s32 sceNpGetState_hook(s32 user_id, OrbisNpState* state) {
    return Libraries::Np::NpManager::sceNpGetState(user_id, state);
}

void init_hooks() {
    HOOK16(sceNpGetState);
}