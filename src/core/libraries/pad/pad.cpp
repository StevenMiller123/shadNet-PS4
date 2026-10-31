// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <mutex>
#include <orbis/UserService.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/pad/pad.h"
#include "shadnet/settings_menu.h"

SHADNET_HOOK_DECLARE(Libraries::Pad, scePadRead);
SHADNET_HOOK_DECLARE(Libraries::Pad, scePadReadState);

static void RegisterPadHooks() {
    SHADNET_HOOK16(Libraries::Pad, scePadRead);
    SHADNET_HOOK16(Libraries::Pad, scePadReadState);
}

namespace Libraries::Pad {

static std::mutex g_handle_mutex;
static std::map<s32, s32> g_handle_users;

static s32 GetHandleUser(s32 handle) {
    std::scoped_lock lk{g_handle_mutex};
    if (auto it = g_handle_users.find(handle); it != g_handle_users.end()) {
        return it->second;
    }

    OrbisUserServiceLoginUserIdList user_list{};
    if (sceUserServiceGetLoginUserIdList(&user_list) != 0) {
        return -1;
    }
    for (s32 user_id : user_list.userId) {
        if (user_id == -1) {
            continue;
        }
        if (scePadGetHandle(user_id, 0, 0) == handle) {
            g_handle_users[handle] = user_id;
            return user_id;
        }
    }
    return -1;
}

static void ClearPadData(OrbisPadData* data) {
    data->buttons = 0;
    data->leftStick = {0x80, 0x80};
    data->rightStick = {0x80, 0x80};
    data->analogButtons = {0, 0};
    data->touch.fingers = 0;
}

static void ProcessPadData(s32 handle, OrbisPadData* data, s32 num) {
    auto& menu = ShadNet::SettingsMenu::GetInstance();
    if (!menu.IsOpen()) {
        const OrbisPadData& latest = data[num - 1];
        if (latest.connected) {
            s32 user_id = GetHandleUser(handle);
            if (user_id != -1) {
                menu.OnPadInput(user_id, latest.buttons);
            }
        }
    }

    if (menu.IsOpen()) {
        for (s32 i = 0; i < num; i++) {
            ClearPadData(&data[i]);
        }
    }
}

s32 scePadRead(s32 handle, OrbisPadData* data, s32 num) {
    s32 result = SHADNET_HOOK_CONTINUE(scePadRead, handle, data, num);
    if (result > 0 && data != nullptr) {
        ProcessPadData(handle, data, result);
    }
    return result;
}

s32 scePadReadState(s32 handle, OrbisPadData* data) {
    s32 result = SHADNET_HOOK_CONTINUE(scePadReadState, handle, data);
    if (result == ORBIS_OK && data != nullptr) {
        ProcessPadData(handle, data, 1);
    }
    return result;
}

void RegisterHooks() {
    return RegisterPadHooks();
}
} // namespace Libraries::Pad
