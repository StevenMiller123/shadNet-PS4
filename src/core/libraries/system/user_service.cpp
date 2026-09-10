// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/UserService.h>
#include <string.h>
#include "client_main.h"
#include "common/logging/log.h"
#include "common/plugin_common.h"
#include "core/libraries/np/np_handler.h"
#include "core/libraries/system/user_service.h"

SHADNET_HOOK_DECLARE(Libraries::System::UserService, sceUserServiceGetUserName);
SHADNET_HOOK_DECLARE(Libraries::System::UserService, sceUserServiceInitialize);
SHADNET_HOOK_DECLARE(Libraries::System::UserService, sceUserServiceInitialize2);

void RegisterUserServiceHooks() {
    s32 ret = sceUserServiceInitialize(nullptr);
    if (ret != 0) {
        LOG_INFO(Lib_UserService, "sceUserServiceInitialize returned {:#x}", ret);
    }
    SHADNET_HOOK(Libraries::System::UserService, sceUserServiceGetUserName);
    SHADNET_HOOK(Libraries::System::UserService, sceUserServiceInitialize);
    SHADNET_HOOK(Libraries::System::UserService, sceUserServiceInitialize2);
}

namespace Libraries::System::UserService {

static bool g_lib_init = false;

s32 sceUserServiceInitialize(const OrbisUserServiceInitializeParams* params) {
    LOG_INFO(Lib_UserService, "called");
    if (g_lib_init) {
        return ORBIS_USER_SERVICE_ERROR_ALREADY_INITIALIZED;
    }
    g_lib_init = true;
    return ORBIS_OK;
}

s32 sceUserServiceInitialize2(s32 thread_prio, u64 cpu_mask) {
    LOG_INFO(Lib_UserService, "called");
    if (g_lib_init) {
        return ORBIS_USER_SERVICE_ERROR_ALREADY_INITIALIZED;
    }
    g_lib_init = true;
    return ORBIS_OK;
}

s32 sceUserServiceGetUserName(s32 user_id, char* user_name, u64 name_len) {
    LOG_INFO(Lib_UserService, "called");
    if (!g_lib_init) {
        return ORBIS_USER_SERVICE_ERROR_NOT_INITIALIZED;
    }
    if (!user_name) {
        return ORBIS_USER_SERVICE_ERROR_INVALID_ARGUMENT;
    }

    if (Np::NpHandler::Instance().IsActive()) {
        // If we're signed into shadNet, supply the npid instead.
        // Still need to reverse this and figure out remaining error cases.
        const OrbisNpId& np_id = Np::NpHandler::Instance().GetNpId(user_id);
        u64 copy_len = std::min<u64>(name_len, strnlen(np_id.handle.data, 16));
        strncpy(user_name, np_id.handle.data, copy_len);
        return ORBIS_OK;
    }
    // Fallback to the real function instead.
    return SHADNET_HOOK_CONTINUE(sceUserServiceGetUserName, user_id, user_name, name_len);
}

void RegisterHooks() {
    return RegisterUserServiceHooks();
}
} // namespace Libraries::System::UserService