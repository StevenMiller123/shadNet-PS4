// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "common/types.h"
#include "shadnet/config.h"
#include "shadnet/dialogs/message_dialog.h"

namespace ShadNet {

class SettingsPage {
public:
    explicit SettingsPage(MessageDialog& dialog) : m_dialog{dialog} {}

    void Run(s32 user_id);

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

    std::optional<std::string> ShowKeyboard(const char* title, const std::string& value,
                                            FieldType type);

    std::string FormatValue(const Field& field, bool use_default = false);
    std::string FormatSummary();
    void EditFields(std::vector<Field> fields);
    void EditField(Field& field);
    void ResetField(Field& field);

    MessageDialog& m_dialog;
    s32 m_user_id{-1};
    bool m_dirty{};

    UserSettings m_user{};
    ServerSettings m_server{};

    std::map<s32, UserSettings> m_saved_user{};
    std::optional<ServerSettings> m_saved_server{};
};

} // namespace ShadNet
