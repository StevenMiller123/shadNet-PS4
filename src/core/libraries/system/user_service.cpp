// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/UserService.h>
#include <string.h>
#include "common/logging/log.h"
#include "common/plugin_common.h"
#include "core/libraries/np/np_handler.h"
#include "core/libraries/system/user_service.h"

HOOK_INIT(sceUserServiceGetUserName);
s32 sceUserServiceGetUserName_hook(s32 user_id, char* user_name, u64 name_len) {
    s32 result =
        Libraries::System::UserService::sceUserServiceGetUserName(user_id, user_name, name_len);
    if (result == 1) {
        // placeholder return to indicate we need to get the actual user name.
        return HOOK_CONTINUE(sceUserServiceGetUserName, s32 (*)(s32, char*, u64), user_id,
                             user_name, name_len);
    }
    return result;
}

void RegisterUserServiceHooks() {
    HOOK(sceUserServiceGetUserName);
}

namespace Libraries::System::UserService {

s32 sceUserServiceGetUserName(s32 user_id, char* user_name, u64 name_len) {
    if (!user_name) {
        return ORBIS_USER_SERVICE_ERROR_INVALID_ARGUMENT;
    }

    if (Np::NpHandler::Instance().IsActive()) {
        // If we're signed into shadNet, supply the npid instead.
        // Still need to reverse this and figure out remaining error cases.
        const OrbisNpId& np_id = Np::NpHandler::Instance().GetNpId();
        u64 copy_len = std::min<u64>(name_len, strnlen(np_id.handle.data, 16));
        strncpy(user_name, np_id.handle.data, copy_len);
        return ORBIS_OK;
    }
    // Fallback to the real function instead.
    return 1;
}

void RegisterHooks() {
    return RegisterUserServiceHooks();
}
} // namespace Libraries::System::UserService