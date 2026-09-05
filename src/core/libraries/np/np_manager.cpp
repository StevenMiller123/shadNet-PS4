// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/NpManager.h>
#include "common/logging/log.h"
#include "common/plugin_common.h"
#include "core/libraries/np/np_handler.h"
#include "core/libraries/np/np_manager.h"

HOOK_INIT(sceNpGetState);

s32 sceNpGetState_hook(s32 user_id, OrbisNpState* state) {
    return Libraries::Np::NpManager::sceNpGetState(user_id, state);
}

void RegisterLibraryHooks() {
    HOOK(sceNpGetState);
}

namespace Libraries::Np::NpManager {
s32 sceNpGetState(s32 user_id, OrbisNpState* state) {
    LOG_INFO(Lib_NpManager, "(STUBBED) called, returning SIGNED_IN");
    if (state) {
        *state = ORBIS_NP_STATE_SIGNED_IN;
    }
    return ORBIS_OK;
}

void RegisterHooks() {
    return RegisterLibraryHooks();
}
} // namespace Libraries::Np::NpManager