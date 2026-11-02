// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include <thread>
#include <orbis/UserService.h>
#include <orbis/libkernel.h>

#include "common/logging/log.h"
#include "core/libraries/pad/pad.h"
#include "shadnet/dialogs/invitation_prompt.h"
#include "shadnet/dialogs/settings_menu.h"

namespace Libraries::Pad {

static constexpr u32 POLL_INTERVAL = 16000;
static constexpr u64 USER_REFRESH_INTERVAL = 1000000;

static void PollPads() {
    auto& menu = ShadNet::SettingsMenu::GetInstance();
    auto& prompt = ShadNet::InvitationPrompt::GetInstance();

    OrbisUserServiceLoginUserIdList user_list{};
    bool have_users = false;
    u64 last_refresh = 0;

    while (true) {
        const u64 now = sceKernelGetProcessTime();

        if (!menu.IsOpen() && !prompt.IsOpen()) {
            if (!have_users || now - last_refresh >= USER_REFRESH_INTERVAL) {
                have_users = sceUserServiceGetLoginUserIdList(&user_list) == 0;
                last_refresh = now;
            }
            if (have_users) {
                for (s32 user_id : user_list.userId) {
                    if (user_id == ORBIS_USER_SERVICE_USER_ID_INVALID) {
                        continue;
                    }
                    s32 handle = scePadGetHandle(user_id, 0, 0);
                    if (handle < 0) {
                        continue;
                    }
                    OrbisPadData data{};
                    if (scePadReadState(handle, &data) == ORBIS_OK && data.connected) {
                        menu.OnPadInput(user_id, data.buttons);
                    }
                }
            }
        }
        sceKernelUsleep(POLL_INTERVAL);
    }
}

void StartPolling() {
    std::thread(PollPads).detach();
}
} // namespace Libraries::Pad
