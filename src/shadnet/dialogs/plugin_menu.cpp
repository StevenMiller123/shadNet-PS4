// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <orbis/MsgDialog.h>
#include <orbis/libkernel.h>

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "shadnet/config.h"
#include "shadnet/dialogs/plugin_menu.h"

namespace ShadNet {

static constexpr u32 MENU_COMBO = ORBIS_PAD_BUTTON_L3 | ORBIS_PAD_BUTTON_R3;
static constexpr u64 MENU_COMBO_HOLD_TIME = 2000000;
static constexpr u64 MAX_ONLINE_FRIENDS = 10;

using Choice = MessageDialog::Choice;

PluginMenu& PluginMenu::GetInstance() {
    static PluginMenu s_instance;
    return s_instance;
}

void PluginMenu::OnPadInput(s32 user_id, u32 buttons) {
    std::scoped_lock lk{m_input_mutex};

    if (Settings::GetInstance().IsFirstRun() && !m_first_run_handled) {
        // Open the settings on first boot so new users can sign in.
        m_first_run_handled = true;
        Open(user_id, true);

        // Wait for the menu thread to finish up.
        // Once it's done, tell the user how to access the menu in the future.
        m_thread.join();
        LOG_NOTIFICATION(shadNet, "Hold L3 + R3 to reopen the plugin menu while ingame");
        return;
    }

    // Open the menu after the right button combo is held
    u64 time = sceKernelGetProcessTime();
    ComboState& combo = m_combo[user_id];
    if ((buttons & MENU_COMBO) != MENU_COMBO) {
        combo = {};
        return;
    }
    if (combo.fired) {
        return;
    }
    if (combo.start_time == 0) {
        combo.start_time = time;
    } else if (time - combo.start_time >= MENU_COMBO_HOLD_TIME) {
        combo.fired = true;
        Open(user_id);
    }
}

void PluginMenu::Open(s32 user_id, bool settings_only) {
    if (m_open.exchange(true)) {
        return;
    }
    if (m_thread.joinable()) {
        m_thread.join();
    }
    LOG_INFO(Config, "opening for user {:#x}", user_id);
    m_thread = std::thread(&PluginMenu::MenuThread, this, user_id, settings_only);
}

void PluginMenu::MenuThread(s32 user_id, bool settings_only) {
    if (!m_dialog.Begin(user_id)) {
        m_open = false;
        return;
    }

    if (settings_only) {
        m_settings.Run(user_id);
    } else {
        while (true) {
            Choice choice = m_dialog.Show(
                FormatHome(user_id), ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS, "Settings", "Friends");
            if (choice == Choice::Button1) {
                m_settings.Run(user_id);
            } else if (choice == Choice::Button2) {
                m_friends.Run(user_id);
            } else {
                break;
            }
        }
    }

    m_dialog.End();
    LOG_INFO(Config, "closed");
    m_open = false;
}

std::string PluginMenu::FormatHome(s32 user_id) {
    auto& np_handler = Libraries::Np::NpHandler::Instance();

    std::string msg = "shadNet\n\n";
    if (np_handler.IsSignedIn(user_id)) {
        const auto np_id = np_handler.GetNpId(user_id);
        const auto* name = np_id.handle.data;
        msg += fmt::format("Signed in as {}\n\nOnline friends:\n",
                           std::string(name, strnlen(name, sizeof(np_id.handle.data))));

        u64 online = 0;
        for (const auto& info : np_handler.GetFriendList(user_id).friends) {
            if (!info.online) {
                continue;
            }
            if (online++ < MAX_ONLINE_FRIENDS) {
                msg += info.npid + "\n";
            }
        }
        if (online == 0) {
            msg += "None\n";
        } else if (online > MAX_ONLINE_FRIENDS) {
            msg += fmt::format("...and {} more\n", online - MAX_ONLINE_FRIENDS);
        }
    } else {
        const auto user_name = Settings::GetInstance().GetNpId(user_id);
        msg += fmt::format("User {} is not signed in\n", user_name);
    }

    const auto& game_info = Common::ElfInfo::Instance();
    msg +=
        fmt::format("\n{}\n{} v{}", game_info.Title(), game_info.GameSerial(), game_info.AppVer());
    return msg;
}

} // namespace ShadNet
