// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <deque>
#include <absl/container/flat_hash_map.h>
#include <orbis/NpManager.h>
#include <orbis/libkernel.h>
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/plugin_common.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler.h"
#include "core/libraries/np/np_manager/np_callbacks.h"
#include "core/libraries/np/np_manager/np_manager.h"

extern "C" {
// void sceNpRegisterStateCallbackForToolkit();
// void sceNpUnregisterStateCallbackForToolkit();

SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetState);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetNpId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetOnlineId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpCheckCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpCheckCallbackForLib);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpRegisterStateCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpRegisterStateCallbackA);
// SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpRegisterStateCallbackForToolkit);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpUnregisterStateCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpUnregisterStateCallbackA);
// SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpUnregisterStateCallbackForToolkit);

void RegisterLibraryHooks() {
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetState);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetNpId);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetOnlineId);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpCheckCallback);
    SHADNET_HOOK32(Libraries::Np::NpManager, sceNpCheckCallbackForLib);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpRegisterStateCallback);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpRegisterStateCallbackA);
    // SHADNET_HOOK(Libraries::Np::NpManager, sceNpRegisterStateCallbackForToolkit);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpUnregisterStateCallback);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpUnregisterStateCallbackA);
    // SHADNET_HOOK(Libraries::Np::NpManager, sceNpUnregisterStateCallbackForToolkit);
}
}

namespace Libraries::Np::NpManager {

static s32 g_firmware_version = 0;

s32 sceNpGetState(s32 user_id, OrbisNpState* state) {
    if (user_id == -1 && g_firmware_version >= Common::ElfInfo::FW_900) {
        // FW < 9.00 behavior needs validating.
        LOG_ERROR(Lib_NpManager, "invalid user id {}", user_id);
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!state) {
        LOG_ERROR(Lib_NpManager, "null state pointer");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    
    if (NpHandler::Instance().IsSignedIn(user_id)) {
        LOG_INFO(Lib_NpManager, "called, returning signed in state");
        *state = ORBIS_NP_STATE_SIGNED_IN;
    } else {
        LOG_INFO(Lib_NpManager, "called, returning signed out state");
        *state = ORBIS_NP_STATE_SIGNED_OUT;
    }
    return ORBIS_OK;
}

s32 sceNpGetNpId(s32 user_id, OrbisNpId* np_id) {
    if (user_id == -1) {
        LOG_ERROR(Lib_NpManager, "invalid user id {}", user_id);
        return g_firmware_version >= Common::ElfInfo::FW_900 ? ORBIS_NP_ERROR_INVALID_ARGUMENT
                                                             : ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    if (!np_id) {
        LOG_ERROR(Lib_NpManager, "null np_id");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!NpHandler::Instance().IsSignedIn(user_id)) {
        // Not currently connected to shadNet, treat this as signed out.
        LOG_INFO(Lib_NpManager, "called, returning signed out");
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    LOG_INFO(Lib_NpManager, "called");
    *np_id = NpHandler::Instance().GetNpId(user_id);
    return ORBIS_OK;
}

s32 sceNpGetOnlineId(s32 user_id, OrbisNpOnlineId* online_id) {
    if (user_id == -1) {
        LOG_ERROR(Lib_NpManager, "invalid user id {}", user_id);
        return g_firmware_version >= Common::ElfInfo::FW_900 ? ORBIS_NP_ERROR_INVALID_ARGUMENT
                                                             : ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    if (!online_id) {
        LOG_ERROR(Lib_NpManager, "null online_id");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!NpHandler::Instance().IsSignedIn(user_id)) {
        // Not currently connected to shadNet, treat this as signed out.
        LOG_INFO(Lib_NpManager, "called, returning signed out");
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    LOG_INFO(Lib_NpManager, "called");
    *online_id = NpHandler::Instance().GetNpId(user_id).handle;
    return ORBIS_OK;
}

void RegisterHooks() {
    sceKernelGetCompiledSdkVersion(&g_firmware_version);
    NpHandler::Instance().RegisterStateCallback(
        [](s32 user_id, OrbisNpState state) { QueueNpStateEvent(user_id, state); }, nullptr);

    return RegisterLibraryHooks();
}
} // namespace Libraries::Np::NpManager