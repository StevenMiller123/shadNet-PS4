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
#include "shadnet/config.h"

extern "C" {
void sceNpRegisterStateCallbackForToolkit();
void sceNpUnregisterStateCallbackForToolkit();
void sceNpSetGamePresenceOnline();
void sceNpUnregisterGamePresenceCallbackA();
void sceNpIsPlusMember();
}

SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetState);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetNpId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetOnlineId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpCheckCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpCheckCallbackForLib);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpRegisterStateCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpRegisterStateCallbackA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpRegisterStateCallbackForToolkit);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpRegisterNpReachabilityStateCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpUnregisterStateCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpUnregisterStateCallbackA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpUnregisterStateCallbackForToolkit);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpUnregisterNpReachabilityStateCallback);
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
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetAccountCountry);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetAccountCountryA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetAccountDateOfBirth);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetAccountDateOfBirthA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetGamePresenceStatus);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetGamePresenceStatusA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpSetGamePresenceOnline);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpSetGamePresenceOnlineA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpRegisterGamePresenceCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpRegisterGamePresenceCallbackA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpUnregisterGamePresenceCallbackA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpIsPlusMember);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpNotifyPlusFeature);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpRegisterPlusEventCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpUnregisterPlusEventCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetNpReachabilityState);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetUserIdByAccountId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetUserIdByOnlineId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpHasSignedUp);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpSetContentRestriction);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpSetNpTitleId);

static void RegisterNpManagerHooks() {
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetState);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetNpId);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetOnlineId);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpCheckCallback);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpCheckCallbackForLib);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpRegisterStateCallback);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpRegisterStateCallbackA);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpRegisterStateCallbackForToolkit);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpRegisterNpReachabilityStateCallback);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpUnregisterStateCallback);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpUnregisterStateCallbackA);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpUnregisterStateCallbackForToolkit);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpUnregisterNpReachabilityStateCallback);
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
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetAccountCountry);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetAccountCountryA);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetAccountDateOfBirth);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetAccountDateOfBirthA);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetGamePresenceStatus);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetGamePresenceStatusA);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpSetGamePresenceOnline);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpSetGamePresenceOnlineA);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpRegisterGamePresenceCallback);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpRegisterGamePresenceCallbackA);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpUnregisterGamePresenceCallbackA);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpIsPlusMember);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpNotifyPlusFeature);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpRegisterPlusEventCallback);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpUnregisterPlusEventCallback);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetNpReachabilityState);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetUserIdByAccountId);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetUserIdByOnlineId);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpHasSignedUp);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpSetContentRestriction);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpSetNpTitleId);
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
        LOG_INFO(Lib_NpManager, "called, returning signed out");
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    *online_id = NpHandler::Instance().GetNpId(user_id).handle;
    return ORBIS_OK;
}

s32 sceNpGetAccountId(OrbisNpOnlineId* online_id, OrbisNpAccountId* account_id) {
    if (!online_id || !account_id) {
        LOG_ERROR(Lib_NpManager, "invalid argument");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    s32 user_id = NpHandler::Instance().GetUserIdByOnlineId(*online_id);
    if (user_id == -1) {
        LOG_ERROR(Lib_NpManager, "invalid online id");
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    if (!NpHandler::Instance().IsSignedIn(user_id)) {
        LOG_INFO(Lib_NpManager, "called, returning signed out");
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    *account_id = NpHandler::Instance().GetAccountId(user_id);
    return ORBIS_OK;
}

s32 sceNpGetAccountIdA(s32 user_id, OrbisNpAccountId* account_id) {
    if (!account_id || user_id == -1) {
        LOG_ERROR(Lib_NpManager, "invalid argument");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!NpHandler::Instance().IsSignedIn(user_id)) {
        LOG_INFO(Lib_NpManager, "called, returning signed out");
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    *account_id = NpHandler::Instance().GetAccountId(user_id);
    return ORBIS_OK;
}

s32 sceNpGetAccountCountry(OrbisNpOnlineId* online_id, OrbisNpCountryCode* country_code) {
    if (!online_id || !country_code) {
        LOG_ERROR(Lib_NpManager, "invalid argument");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    s32 user_id = NpHandler::Instance().GetUserIdByOnlineId(*online_id);
    if (user_id == -1) {
        LOG_ERROR(Lib_NpManager, "invalid online id");
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    return sceNpGetAccountCountryA(user_id, country_code);
}

s32 sceNpGetAccountCountryA(s32 user_id, OrbisNpCountryCode* country_code) {
    if (!country_code || user_id == -1) {
        LOG_ERROR(Lib_NpManager, "invalid argument");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!NpHandler::Instance().IsSignedIn(user_id)) {
        LOG_INFO(Lib_NpManager, "called, returning signed out");
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }

    // TODO: Retrieve from console?
    std::memset(country_code, 0, sizeof(OrbisNpCountryCode));
    std::memcpy(country_code->country_code, "us", 2);
    return ORBIS_OK;
}

s32 sceNpGetAccountDateOfBirth(OrbisNpOnlineId* online_id, OrbisNpDate* date_of_birth) {
    if (!online_id || !date_of_birth) {
        LOG_ERROR(Lib_NpManager, "invalid argument");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    s32 user_id = NpHandler::Instance().GetUserIdByOnlineId(*online_id);
    if (user_id == -1) {
        LOG_ERROR(Lib_NpManager, "invalid online id");
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    return sceNpGetAccountDateOfBirthA(user_id, date_of_birth);
}

s32 sceNpGetAccountDateOfBirthA(s32 user_id, OrbisNpDate* date_of_birth) {
    if (!date_of_birth || user_id == -1) {
        LOG_ERROR(Lib_NpManager, "invalid argument");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!NpHandler::Instance().IsSignedIn(user_id)) {
        LOG_INFO(Lib_NpManager, "called, returning signed out");
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }

    // TODO: Retrieve from console?
    date_of_birth->year = 2000;
    date_of_birth->month = 1;
    date_of_birth->day = 1;
    return ORBIS_OK;
}

s32 sceNpGetGamePresenceStatus(OrbisNpOnlineId* online_id, OrbisNpGamePresenseStatus* status) {
    if (!online_id || !status) {
        LOG_ERROR(Lib_NpManager, "invalid argument");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    s32 user_id = NpHandler::Instance().GetUserIdByOnlineId(*online_id);
    *status = user_id != -1 && NpHandler::Instance().IsSignedIn(user_id)
                  ? OrbisNpGamePresenseStatus::Online
                  : OrbisNpGamePresenseStatus::Offline;
    return ORBIS_OK;
}

s32 sceNpGetGamePresenceStatusA(s32 user_id, OrbisNpGamePresenseStatus* status) {
    if (!status || user_id == -1) {
        LOG_ERROR(Lib_NpManager, "invalid argument");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    *status = NpHandler::Instance().IsSignedIn(user_id) ? OrbisNpGamePresenseStatus::Online
                                                        : OrbisNpGamePresenseStatus::Offline;
    return ORBIS_OK;
}

s32 sceNpGetNpReachabilityState(s32 user_id, OrbisNpReachabilityState* state) {
    if (!state) {
        LOG_ERROR(Lib_NpManager, "null state pointer");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (user_id == -1 && g_firmware_version >= Common::ElfInfo::FW_400) {
        LOG_ERROR(Lib_NpManager, "invalid user id {}", user_id);
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    *state = NpHandler::Instance().IsSignedIn(user_id) ? OrbisNpReachabilityState::Reachable
                                                       : OrbisNpReachabilityState::Unavailable;
    return ORBIS_OK;
}

s32 sceNpGetUserIdByAccountId(OrbisNpAccountId account_id, s32* user_id) {
    if (account_id == 0 || !user_id) {
        LOG_ERROR(Lib_NpManager, "invalid argument");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    s32 found = NpHandler::Instance().GetUserIdByAccountId(account_id);
    if (found == -1) {
        LOG_INFO(Lib_NpManager, "called, returning signed out");
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    *user_id = found;
    return ORBIS_OK;
}

s32 sceNpGetUserIdByOnlineId(const OrbisNpOnlineId* online_id, s32* user_id) {
    if (!online_id || !user_id) {
        LOG_ERROR(Lib_NpManager, "invalid argument");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    s32 found = NpHandler::Instance().GetUserIdByOnlineId(*online_id);
    if (found == -1) {
        LOG_INFO(Lib_NpManager, "called, returning signed out");
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    *user_id = found;
    return ORBIS_OK;
}

s32 sceNpHasSignedUp(s32 user_id, bool* has_signed_up) {
    if (!has_signed_up) {
        LOG_ERROR(Lib_NpManager, "null has_signed_up");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    *has_signed_up = false;
    if (user_id == -1 && g_firmware_version >= Common::ElfInfo::FW_900) {
        LOG_ERROR(Lib_NpManager, "invalid user id {}", user_id);
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    *has_signed_up = !ShadNet::Settings::GetInstance().GetNpId(user_id).empty();
    return ORBIS_OK;
}

s32 sceNpIsPlusMember(s32 user_id, bool* result) {
    if (!result) {
        LOG_ERROR(Lib_NpManager, "null result");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    *result = true;
    return ORBIS_OK;
}

s32 sceNpSetContentRestriction(const OrbisNpContentRestriction* restriction) {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    if (!restriction) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (restriction->size != sizeof(OrbisNpContentRestriction)) {
        return ORBIS_NP_ERROR_INVALID_SIZE;
    }
    if (restriction->default_age_restriction < 0 || restriction->age_restriction_count > 0x100) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (restriction->age_restriction_count > 0 && !restriction->age_restriction) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    return ORBIS_OK;
}

s32 sceNpSetNpTitleId(const OrbisNpTitleId* title_id, const OrbisNpTitleSecret* title_secret) {
    if (!title_id || !title_secret) {
        LOG_ERROR(Lib_NpManager, "invalid argument");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    LOG_ERROR(Lib_NpManager, "(STUBBED) called, title_id = {}", title_id->id);
    return ORBIS_OK;
}

void RegisterHooks() {
    sceKernelGetCompiledSdkVersion(&g_firmware_version);
    NpHandler::Instance().RegisterStateCallback(
        [](s32 user_id, OrbisNpState state) { QueueNpStateEvent(user_id, state); }, nullptr);

    // Initialize NpHandler with NpManager
    NpHandler::Instance().Initialize();
    return RegisterNpManagerHooks();
}
} // namespace Libraries::Np::NpManager
