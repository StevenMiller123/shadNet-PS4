// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/NpManager.h>
#include <orbis/libkernel.h>
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/plugin_common.h"
#include "core/libraries/np/np_handler.h"
#include "core/libraries/np/np_manager.h"

HOOK_INIT(sceNpGetState);
HOOK_INIT(sceNpGetNpId);
HOOK_INIT(sceNpGetOnlineId);

s32 sceNpGetState_hook(s32 user_id, OrbisNpState* state) {
    return Libraries::Np::NpManager::sceNpGetState(user_id, state);
}

s32 sceNpGetNpId_hook(s32 user_id, OrbisNpId* np_id) {
    return Libraries::Np::NpManager::sceNpGetNpId(user_id, np_id);
}

s32 sceNpGetOnlineId_hook(s32 user_id, OrbisNpOnlineId* online_id) {
    return Libraries::Np::NpManager::sceNpGetOnlineId(user_id, online_id);
}

void RegisterLibraryHooks() {
    HOOK(sceNpGetState);
    HOOK(sceNpGetNpId);
    HOOK(sceNpGetOnlineId);
}

namespace Libraries::Np::NpManager {

static s32 g_firmware_version = 0;

s32 sceNpGetState(s32 user_id, OrbisNpState* state) {
    LOG_INFO(Lib_NpManager, "called");
    if (user_id == -1 && g_firmware_version >= Common::ElfInfo::FW_900) {
        // FW < 9.00 behavior needs validating.
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!state) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (NpHandler::Instance().IsActive()) {
        *state = ORBIS_NP_STATE_SIGNED_IN;
    } else {
        *state = ORBIS_NP_STATE_SIGNED_OUT;
    }
    return ORBIS_OK;
}

s32 sceNpGetNpId(s32 user_id, OrbisNpId* np_id) {
    LOG_INFO(Lib_NpManager, "called");
    if (user_id == -1) {
        return g_firmware_version >= Common::ElfInfo::FW_900 ? ORBIS_NP_ERROR_INVALID_ARGUMENT
                                                             : ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    if (!np_id) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!NpHandler::Instance().IsActive()) {
        // Not currently connected to shadNet, treat this as signed out.
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    *np_id = NpHandler::Instance().GetNpId();
    return ORBIS_OK;
}

s32 sceNpGetOnlineId(s32 user_id, OrbisNpOnlineId* online_id) {
    LOG_INFO(Lib_NpManager, "called");
    if (user_id == -1) {
        return g_firmware_version >= Common::ElfInfo::FW_900 ? ORBIS_NP_ERROR_INVALID_ARGUMENT
                                                             : ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    if (!online_id) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!NpHandler::Instance().IsActive()) {
        // Not currently connected to shadNet, treat this as signed out.
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    *online_id = NpHandler::Instance().GetNpId().handle;
    return ORBIS_OK;
}

void RegisterHooks() {
    sceKernelGetCompiledSdkVersion(&g_firmware_version);
    return RegisterLibraryHooks();
}
} // namespace Libraries::Np::NpManager