// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include "common/types.h"
#include "shadnet/dialogs/friends_page.h"
#include "shadnet/dialogs/message_dialog.h"
#include "shadnet/dialogs/settings_page.h"

namespace ShadNet {

class PluginMenu {
public:
    static PluginMenu& GetInstance();

    PluginMenu(const PluginMenu&) = delete;
    PluginMenu& operator=(const PluginMenu&) = delete;

    void OnPadInput(s32 user_id, u32 buttons);

    void Open(s32 user_id, bool settings_only = false);

    bool IsOpen() {
        return m_open;
    }

private:
    struct ComboState {
        u64 start_time{};
        bool fired{};
    };

    PluginMenu() = default;
    ~PluginMenu() = default;

    void MenuThread(s32 user_id, bool settings_only);
    std::string FormatHome(s32 user_id);

    std::atomic<bool> m_open{false};
    std::thread m_thread;
    MessageDialog m_dialog;
    SettingsPage m_settings{m_dialog};
    FriendsPage m_friends{m_dialog};

    std::mutex m_input_mutex;
    std::map<s32, ComboState> m_combo{};
    bool m_first_run_handled{};
};

} // namespace ShadNet
