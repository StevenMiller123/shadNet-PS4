// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "common/types.h"
#include "shadnet/config.h"
#include "shadnet/dialogs/message_dialog.h"

namespace ShadNet {

class SettingsMenu {
public:
    static SettingsMenu& GetInstance();

    SettingsMenu(const SettingsMenu&) = delete;
    SettingsMenu& operator=(const SettingsMenu&) = delete;

    void OnPadInput(s32 user_id, u32 buttons);

    void Open(s32 user_id);

    bool IsOpen() {
        return m_open;
    }

private:
    enum class FieldType {
        Bool,
        Text,
        Url,
        Password,
        Port,
    };

    struct Field {
        const char* name;
        FieldType type;
        std::variant<Setting<bool>*, Setting<std::string>*, Setting<s32>*> value;
    };

    struct ComboState {
        u64 start_time{};
        bool fired{};
    };

    SettingsMenu() = default;
    ~SettingsMenu() = default;

    void MenuThread(s32 user_id);
    std::optional<std::string> ShowKeyboard(const char* title, const std::string& value,
                                            FieldType type);

    std::string FormatValue(const Field& field);
    std::string FormatSummary();
    void EditFields(std::vector<Field> fields);
    void EditField(Field& field);
    void ResetField(Field& field);

    std::atomic<bool> m_open{false};
    std::thread m_thread;
    s32 m_user_id{-1};
    MessageDialog m_dialog;
    bool m_dirty{};

    UserSettings m_user{};
    ServerSettings m_server{};

    std::map<s32, UserSettings> m_saved_user{};
    std::optional<ServerSettings> m_saved_server{};

    std::mutex m_input_mutex;
    std::map<s32, ComboState> m_combo{};
    bool m_first_run_handled{};
};

} // namespace ShadNet
