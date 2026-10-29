// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <deque>
#include <orbis/NpManager.h>
#include <orbis/libkernel.h>

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_manager/np_manager.h"
#include "core/libraries/np/np_types.h"

extern "C" {
void sceNpRegisterStateCallbackForToolkit();
void sceNpUnregisterStateCallbackForToolkit();
}

SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetState);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetNpId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetOnlineId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpCheckCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpCheckCallbackForLib);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpRegisterStateCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpRegisterStateCallbackA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpRegisterStateCallbackForToolkit);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpUnregisterStateCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpUnregisterStateCallbackA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpUnregisterStateCallbackForToolkit);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpCreateRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpCreateAsyncRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpCheckNpAvailability);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpCheckNpAvailabilityA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpCheckNpReachability);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpCheckPlus);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetAccountLanguage);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetAccountLanguageA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetParentalControlInfo);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetParentalControlInfoA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpAbortRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpSetTimeout);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpWaitAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpPollAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpDeleteRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetAccountId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetAccountIdA);

static void RegisterNpManagerHooks() {
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetState);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetNpId);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetOnlineId);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpCheckCallback);
    SHADNET_HOOK16(Libraries::Np::NpManager, sceNpCheckCallbackForLib);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpRegisterStateCallback);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpRegisterStateCallbackA);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpRegisterStateCallbackForToolkit);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpUnregisterStateCallback);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpUnregisterStateCallbackA);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpUnregisterStateCallbackForToolkit);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpCreateRequest);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpCreateAsyncRequest);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpCheckNpAvailability);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpCheckNpAvailabilityA);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpCheckNpReachability);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpCheckPlus);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetAccountLanguage);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetAccountLanguageA);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetParentalControlInfo);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetParentalControlInfoA);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpAbortRequest);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpSetTimeout);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpWaitAsync);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpPollAsync);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpDeleteRequest);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetAccountId);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetAccountIdA);
}

namespace Libraries::Np::NpManager {

static s32 g_firmware_version = 0;

s32 GetFirmwareVersion() {
    return g_firmware_version;
}

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
        *state = OrbisNpState::SignedIn;
    } else {
        *state = OrbisNpState::SignedOut;
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
    *online_id = NpHandler::Instance().GetNpId(user_id).handle;
    return ORBIS_OK;
}

s32 sceNpGetAccountId(OrbisNpOnlineId* online_id, OrbisNpAccountId* account_id) {
    if (!online_id || !account_id) {
        LOG_ERROR(Lib_NpManager, "null arguments");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    s32 user_id = NpHandler::Instance().GetUserIdByOnlineId(*online_id);
    if (user_id == -1) {
        LOG_ERROR(Lib_NpManager, "invalid online id");
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    if (!NpHandler::Instance().IsSignedIn(user_id)) {
        // Not currently connected to shadNet, treat this as signed out.
        LOG_INFO(Lib_NpManager, "called, returning signed out");
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    *account_id = NpHandler::Instance().GetAccountId(user_id);
    return ORBIS_OK;
}

s32 sceNpGetAccountIdA(s32 user_id, OrbisNpAccountId* account_id) {
    if (!account_id || user_id == -1) {
        LOG_ERROR(Lib_NpManager, "invalid arguments");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!NpHandler::Instance().IsSignedIn(user_id)) {
        // Not currently connected to shadNet, treat this as signed out.
        LOG_INFO(Lib_NpManager, "called, returning signed out");
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    *account_id = NpHandler::Instance().GetAccountId(user_id);
    return ORBIS_OK;
}

void RegisterHooks() {
    sceKernelGetCompiledSdkVersion(&g_firmware_version);
    NpHandler::Instance().RegisterStateCallback(
        [](s32 user_id, OrbisNpState state) { QueueNpStateEvent(user_id, state); }, nullptr);

    return RegisterNpManagerHooks();
}
} // namespace Libraries::Np::NpManager