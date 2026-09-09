// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/UserService.h>
#include <string.h>
#include "client_main.h"
#include "common/assert.h"
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
        return SHADNET_HOOK_CONTINUE(sceUserServiceGetUserName, user_id, user_name, name_len);
    }
    return result;
}

void RegisterUserServiceHooks() {
    HOOK(sceUserServiceGetUserName);
}

namespace Libraries::System::UserService {

HOOK_INIT(sceUserServiceInitialize);
HOOK_INIT(sceUserServiceInitialize2);

static bool g_lib_init = false;

s32 sceUserServiceInitialize(const OrbisUserServiceInitializeParams* params) {
    u32 ret = HOOK_CONTINUE(sceUserServiceInitialize,
                            s32 (*)(const OrbisUserServiceInitializeParams*), params);
    LOG_INFO(Lib_UserService, "called from {}, params: {}, ret: {:#x}", __builtin_return_address(0),
             fmt::ptr(params), ret);
    if (ret != ORBIS_OK) {
        return ret;
    }
    g_lib_init = true;
    return client_start() == ORBIS_OK ? ORBIS_OK : ORBIS_USER_SERVICE_ERROR_INTERNAL;
}

s32 sceUserServiceInitialize2(s32 thread_prio, u64 cpu_mask) {
    u32 ret = HOOK_CONTINUE(sceUserServiceInitialize2, s32 (*)(s32, u64), thread_prio, cpu_mask);
    LOG_INFO(Lib_UserService, "called from {}, thread_prio: {:#x}, cpu_mask: {:#x}, ret: {:#x}",
             __builtin_return_address(0), thread_prio, cpu_mask, ret);
    if (ret != ORBIS_OK) {
        return ret;
    }
    g_lib_init = true;
    return client_start() == ORBIS_OK ? ORBIS_OK : ORBIS_USER_SERVICE_ERROR_INTERNAL;
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
    return 1;
}

void RegisterHooks() {
    int temp;
    ASSERT(sceUserServiceGetInitialUser(&temp) == ORBIS_USER_SERVICE_ERROR_NOT_INITIALIZED);
    SHADNET_HOOK1(sceUserServiceInitialize);
    // SHADNET_HOOK1(sceUserServiceInitialize2);

    return RegisterUserServiceHooks();
}
} // namespace Libraries::System::UserService