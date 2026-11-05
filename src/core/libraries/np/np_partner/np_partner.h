// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/user_service/user_service.h"

namespace Libraries::Np::NpPartner {

s32 sceNpEAAccessInitialize();
s32 sceNpHasEAAccessSubscription(UserService::OrbisUserServiceUserId user_id, bool* result);
s32 sceNpHasEAAccessSubscriptionAbortRequest();
s32 sceNpEAAccessTerminate();
s32 sceNpPartnerSubscriptionInitialize();
s32 sceNpPartnerSubscriptionHasSubscription(UserService::OrbisUserServiceUserId user_id,
                                            const char* service_name, bool* result);
s32 sceNpPartnerSubscriptionHasSubscriptionAbortRequest();
s32 sceNpPartnerSubscriptionTerminate();
s32 sceNpUbisoftPlusInitialize();
s32 sceNpUbisoftPlusHasSubscription(UserService::OrbisUserServiceUserId user_id, bool* result);
s32 sceNpUbisoftPlusHasSubscriptionAbortRequest();
s32 sceNpUbisoftPlusTerminate();

void RegisterHooks();
} // namespace Libraries::Np::NpPartner