// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <charconv>
#include <orbis/ImeDialog.h>
#include <orbis/MsgDialog.h>

#include "common/logging/log.h"
#include "shadnet/dialogs/settings_page.h"

namespace ShadNet {

static constexpr u32 MAX_TEXT_LENGTH = 128;
static constexpr u32 MAX_PORT_LENGTH = 5;

using Choice = MessageDialog::Choice;
using Button = Libraries::Pad::Button;

void SettingsPage::Run(s32 user_id) {
    m_user_id = user_id;
    m_dirty = false;
    auto& settings = Settings::GetInstance();
    auto saved_user = m_saved_user.find(user_id);
    m_user =
        saved_user != m_saved_user.end() ? saved_user->second : settings.GetUserSettings(user_id);
    m_server = m_saved_server.value_or(settings.GetServerSettings());

    std::vector<Field> user_fields = {
        {"shadNet", FieldType::Bool, &m_user.shadnet_enabled},
        {"NPID", FieldType::Text, &m_user.shadnet_npid},
        {"Password", FieldType::Password, &m_user.shadnet_password},
        {"Appear offline", FieldType::Bool, &m_user.shadnet_appear_offline},
    };
    std::vector<Field> server_fields = {
        {"Server", FieldType::Url, &m_server.shadnet_server},
        {"WebApi server", FieldType::Url, &m_server.shadnet_webapi_server},
        {"P2P port", FieldType::Port, &m_server.shadnet_p2p_port},
        {"UPnP", FieldType::Bool, &m_server.shadnet_enable_upnp},
    };

    while (true) {
        Choice choice = m_dialog.Show(FormatSummary(), ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS,
                                      "Account settings", "Server settings");
        if (choice == Choice::Button1) {
            EditFields(user_fields);
        } else if (choice == Choice::Button2) {
            EditFields(server_fields);
        } else {
            break;
        }
    }

    if (m_dirty) {
        Choice choice = m_dialog.Show("Save changes?", ORBIS_MSG_DIALOG_BUTTON_TYPE_YESNO);
        if (choice == Choice::Button1) {
            if (settings.Save(user_id, m_user, m_server)) {
                m_saved_user[user_id] = m_user;
                m_saved_server = m_server;
                m_dialog.Show("Settings saved.\nRestart the game to apply them.",
                              ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
            } else {
                m_dialog.Show("Failed to save settings.", ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
            }
        }
    }
}

std::optional<std::string> SettingsPage::ShowKeyboard(const char* title, const std::string& value,
                                                      FieldType type) {
    const OrbisImeType ime_type = type == FieldType::Port  ? ORBIS_TYPE_NUMBER
                                  : type == FieldType::Url ? ORBIS_TYPE_TYPE_URL
                                                           : ORBIS_TYPE_BASIC_LATIN;
    return m_dialog.ShowKeyboard(title, value, ime_type, type == FieldType::Password,
                                 type == FieldType::Port ? MAX_PORT_LENGTH : MAX_TEXT_LENGTH);
}

std::string SettingsPage::FormatValue(const Field& field, bool use_default) {
    const auto value_of = [use_default](const auto* setting) {
        return use_default ? setting->default_value : setting->get();
    };
    if (auto* setting = std::get_if<Setting<bool>*>(&field.value)) {
        return value_of(*setting) ? "Enabled" : "Disabled";
    }
    if (auto* setting = std::get_if<Setting<s32>*>(&field.value)) {
        return std::to_string(value_of(*setting));
    }
    std::string value = value_of(std::get<Setting<std::string>*>(field.value));
    if (field.type == FieldType::Password) {
        return value.empty() ? "Not set" : "********";
    }
    return value.empty() ? "Not set" : value;
}

std::string SettingsPage::FormatSummary() {
    return fmt::format("shadNet Settings\n\n"
                       "shadNet: {}\nNPID: {}\nPassword: {}\nAppear offline: {}\n\n"
                       "Server: {}\nWebApi server: {}\nP2P port: {}\nUPnP: {}",
                       m_user.shadnet_enabled.get() ? "Enabled" : "Disabled",
                       m_user.shadnet_npid.get().empty() ? "Not set" : m_user.shadnet_npid.get(),
                       m_user.shadnet_password.get().empty() ? "Not set" : "********",
                       m_user.shadnet_appear_offline.get() ? "Enabled" : "Disabled",
                       m_server.shadnet_server.get(), m_server.shadnet_webapi_server.get(),
                       m_server.shadnet_p2p_port.get(),
                       m_server.shadnet_enable_upnp.get() ? "Enabled" : "Disabled");
}

void SettingsPage::EditFields(std::vector<Field> fields) {
    u64 index = 0;
    while (true) {
        Field& field = fields[index];
        std::string msg = fmt::format("{}: {}\n\n({}/{})\nPress L1 or R1 to navigate", field.name,
                                      FormatValue(field), index + 1, fields.size());
        Choice choice = m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS, "Change", "Reset",
                                      Button::L1 | Button::R1);
        if (choice == Choice::Button1) {
            EditField(field);
        } else if (choice == Choice::Button2) {
            ResetField(field);
        } else if (choice == Choice::L1) {
            index = (index + fields.size() - 1) % fields.size();
        } else if (choice == Choice::R1) {
            index = (index + 1) % fields.size();
        } else {
            return;
        }
    }
}

void SettingsPage::ResetField(Field& field) {
    std::visit(
        [this, &field](auto* setting) {
            if (setting->get() == setting->default_value) {
                return;
            }
            const std::string msg =
                fmt::format("Reset {} to {}?", field.name, FormatValue(field, true));
            if (m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_YESNO) != Choice::Button1) {
                return;
            }
            setting->set(setting->default_value);
            m_dirty = true;
        },
        field.value);
}

void SettingsPage::EditField(Field& field) {
    if (auto* setting = std::get_if<Setting<bool>*>(&field.value)) {
        (*setting)->set(!(*setting)->get());
        m_dirty = true;
        return;
    }

    if (auto* setting = std::get_if<Setting<s32>*>(&field.value)) {
        auto text = ShowKeyboard(field.name, std::to_string((*setting)->get()), field.type);
        if (!text) {
            return;
        }
        s32 port{};
        auto [end, error] = std::from_chars(text->data(), text->data() + text->size(), port);
        if (error != std::errc{} || end != text->data() + text->size() || port < 1 ||
            port > 65535) {
            m_dialog.Show("Invalid port, it must be a number from 1 to 65535.",
                          ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
            return;
        }
        (*setting)->set(port);
        m_dirty = true;
        return;
    }

    auto* setting = std::get<Setting<std::string>*>(field.value);
    std::string current = field.type == FieldType::Password ? "" : setting->get();
    if (auto text = ShowKeyboard(field.name, current, field.type)) {
        setting->set(*text);
        m_dirty = true;
    }
}

} // namespace ShadNet
