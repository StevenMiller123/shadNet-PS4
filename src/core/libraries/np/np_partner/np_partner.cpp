// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/plugin_common.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_partner/np_partner.h"

extern "C" {
void sceNpEAAccessInitialize();
void sceNpHasEAAccessSubscription();
void sceNpHasEAAccessSubscriptionAbortRequest();
void sceNpEAAccessTerminate();
void sceNpUbisoftPlusInitialize();
void sceNpUbisoftPlusHasSubscription();
void sceNpUbisoftPlusHasSubscriptionAbortRequest();
void sceNpUbisoftPlusTerminate();
void sceNpPartnerSubscriptionInitialize();
void sceNpPartnerSubscriptionHasSubscription();
void sceNpPartnerSubscriptionHasSubscriptionAbortRequest();
void sceNpPartnerSubscriptionTerminate();
}

SHADNET_HOOK_DECLARE(Libraries::Np::NpPartner, sceNpEAAccessInitialize);
SHADNET_HOOK_DECLARE(Libraries::Np::NpPartner, sceNpHasEAAccessSubscription);
SHADNET_HOOK_DECLARE(Libraries::Np::NpPartner, sceNpHasEAAccessSubscriptionAbortRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpPartner, sceNpEAAccessTerminate);
SHADNET_HOOK_DECLARE(Libraries::Np::NpPartner, sceNpPartnerSubscriptionInitialize);
SHADNET_HOOK_DECLARE(Libraries::Np::NpPartner, sceNpPartnerSubscriptionHasSubscription);
SHADNET_HOOK_DECLARE(Libraries::Np::NpPartner, sceNpPartnerSubscriptionHasSubscriptionAbortRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpPartner, sceNpPartnerSubscriptionTerminate);
SHADNET_HOOK_DECLARE(Libraries::Np::NpPartner, sceNpUbisoftPlusInitialize);
SHADNET_HOOK_DECLARE(Libraries::Np::NpPartner, sceNpUbisoftPlusHasSubscription);
SHADNET_HOOK_DECLARE(Libraries::Np::NpPartner, sceNpUbisoftPlusHasSubscriptionAbortRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpPartner, sceNpUbisoftPlusTerminate);

static void RegisterNpPartnerHooks() {
    SHADNET_HOOK(Libraries::Np::NpPartner, sceNpEAAccessInitialize);
    SHADNET_HOOK(Libraries::Np::NpPartner, sceNpHasEAAccessSubscription);
    SHADNET_HOOK(Libraries::Np::NpPartner, sceNpHasEAAccessSubscriptionAbortRequest);
    SHADNET_HOOK(Libraries::Np::NpPartner, sceNpEAAccessTerminate);
    SHADNET_HOOK(Libraries::Np::NpPartner, sceNpPartnerSubscriptionInitialize);
    SHADNET_HOOK(Libraries::Np::NpPartner, sceNpPartnerSubscriptionHasSubscription);
    SHADNET_HOOK(Libraries::Np::NpPartner, sceNpPartnerSubscriptionHasSubscriptionAbortRequest);
    SHADNET_HOOK(Libraries::Np::NpPartner, sceNpPartnerSubscriptionTerminate);
    SHADNET_HOOK(Libraries::Np::NpPartner, sceNpUbisoftPlusInitialize);
    SHADNET_HOOK(Libraries::Np::NpPartner, sceNpUbisoftPlusHasSubscription);
    SHADNET_HOOK(Libraries::Np::NpPartner, sceNpUbisoftPlusHasSubscriptionAbortRequest);
    SHADNET_HOOK(Libraries::Np::NpPartner, sceNpUbisoftPlusTerminate);
}

namespace Libraries::Np::NpPartner {

static bool g_ea_access_init = false;

s32 sceNpEAAccessInitialize() {
    LOG_INFO(Lib_NpPartner, "called");
    g_ea_access_init = true;
    return ORBIS_OK;
}

s32 sceNpHasEAAccessSubscription(UserService::OrbisUserServiceUserId user_id, bool* result) {
    LOG_WARNING(Lib_NpPartner, "called");
    if (!g_ea_access_init) {
        return ORBIS_NP_EA_ACCESS_ERROR_NOT_INITIALIZED;
    }
    if (!result) {
        return ORBIS_NP_EA_ACCESS_ERROR_PARAMETER;
    }
    // Stub no subscription, should probably be revisited later for server emulator support.
    *result = false;
    return ORBIS_OK;
}

s32 sceNpHasEAAccessSubscriptionAbortRequest() {
    LOG_INFO(Lib_NpPartner, "called");
    // The HasSubscription functions are blocking, and these would be the way to cancel them.
    return ORBIS_OK;
}

s32 sceNpEAAccessTerminate() {
    LOG_INFO(Lib_NpPartner, "called");
    if (!g_ea_access_init) {
        return ORBIS_NP_EA_ACCESS_ERROR_NOT_INITIALIZED;
    }
    g_ea_access_init = false;
    return ORBIS_OK;
}

static bool g_subscription_init = false;

s32 sceNpPartnerSubscriptionInitialize() {
    LOG_INFO(Lib_NpPartner, "called");
    g_subscription_init = true;
    return ORBIS_OK;
}

s32 sceNpPartnerSubscriptionHasSubscription(UserService::OrbisUserServiceUserId user_id,
                                            const char* service_name, bool* result) {
    LOG_WARNING(Lib_NpPartner, "called, service = {}", service_name ? service_name : "(null)");
    if (!g_subscription_init) {
        return ORBIS_NP_PARTNER_SUBSCRIPTION_ERROR_NOT_INITIALIZED;
    }
    if (!result) {
        return ORBIS_NP_PARTNER_SUBSCRIPTION_ERROR_PARAMETER;
    }
    *result = false;
    return ORBIS_OK;
}

s32 sceNpPartnerSubscriptionHasSubscriptionAbortRequest() {
    LOG_INFO(Lib_NpPartner, "called");
    return ORBIS_OK;
}

s32 sceNpPartnerSubscriptionTerminate() {
    LOG_INFO(Lib_NpPartner, "called");
    if (!g_subscription_init) {
        return ORBIS_NP_PARTNER_SUBSCRIPTION_ERROR_NOT_INITIALIZED;
    }
    g_subscription_init = false;
    return ORBIS_OK;
}

s32 sceNpUbisoftPlusInitialize() {
    return sceNpPartnerSubscriptionInitialize();
}

s32 sceNpUbisoftPlusHasSubscription(UserService::OrbisUserServiceUserId user_id, bool* result) {
    return sceNpPartnerSubscriptionHasSubscription(user_id, "UBISOFT_PLUS", result);
}

s32 sceNpUbisoftPlusHasSubscriptionAbortRequest() {
    return sceNpPartnerSubscriptionHasSubscriptionAbortRequest();
}

s32 sceNpUbisoftPlusTerminate() {
    return sceNpPartnerSubscriptionTerminate();
}

void RegisterHooks() {
    return RegisterNpPartnerHooks();
}

} // namespace Libraries::Np::NpPartner
