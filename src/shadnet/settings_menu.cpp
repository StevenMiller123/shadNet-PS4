// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <charconv>
#include <cstring>
#include <orbis/CommonDialog.h>
#include <orbis/ImeDialog.h>
#include <orbis/MsgDialog.h>
#include <orbis/Pad.h>
#include <orbis/libkernel.h>

#include "common/logging/log.h"
#include "shadnet/settings_menu.h"

namespace ShadNet {

static constexpr u32 MENU_COMBO = ORBIS_PAD_BUTTON_L3 | ORBIS_PAD_BUTTON_R3;
static constexpr u64 MENU_COMBO_HOLD_TIME = 2000000;
static constexpr u32 DIALOG_POLL_INTERVAL = 16000;
static constexpr u32 MAX_TEXT_LENGTH = 128;

static constexpr s32 COMMON_DIALOG_ERROR_ALREADY_SYSTEM_INITIALIZED = 0x80B80002;
static constexpr s32 COMMON_DIALOG_ERROR_ALREADY_INITIALIZED = 0x80B80004;

SettingsMenu& SettingsMenu::GetInstance() {
    static SettingsMenu s_instance;
    return s_instance;
}

void SettingsMenu::OnPadInput(s32 user_id, u32 buttons) {
    std::scoped_lock lk{m_input_mutex};

    if (Settings::GetInstance().IsFirstRun() && !m_first_run_handled) {
        // Open the settings on first boot so new users can sign in.
        m_first_run_handled = true;
        Open(user_id);

        // Wait for the menu thread to finish up.
        // Once it's done, tell the user how to access settings in the future.
        m_thread.join();
        LOG_NOTIFICATION(shadNet, "Hold L3 + R3 to reopen the settings menu while ingame");
        return;
    }

    // Open the settings menu after the right button combo is held
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

void SettingsMenu::Open(s32 user_id) {
    if (m_open.exchange(true)) {
        return;
    }
    if (m_thread.joinable()) {
        m_thread.join();
    }
    LOG_INFO(Config, "opening for user {:#x}", user_id);
    m_thread = std::thread(&SettingsMenu::MenuThread, this, user_id);
}

void SettingsMenu::MenuThread(s32 user_id) {
    m_user_id = user_id;
    m_dirty = false;
    auto& settings = Settings::GetInstance();
    auto saved_user = m_saved_user.find(user_id);
    m_user =
        saved_user != m_saved_user.end() ? saved_user->second : settings.GetUserSettings(user_id);
    m_server = m_saved_server.value_or(settings.GetServerSettings());

    if (!BeginDialogs()) {
        m_open = false;
        return;
    }

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
        // {"UPnP", FieldType::Bool, &m_server.shadnet_enable_upnp},
    };

    while (true) {
        Choice choice = ShowMessage(FormatSummary(), ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS,
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
        Choice choice = ShowMessage("Save changes?", ORBIS_MSG_DIALOG_BUTTON_TYPE_YESNO);
        if (choice == Choice::Button1) {
            if (settings.Save(user_id, m_user, m_server)) {
                m_saved_user[user_id] = m_user;
                m_saved_server = m_server;
                ShowMessage("Settings saved.\nRestart the game to apply them.",
                            ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
            } else {
                ShowMessage("Failed to save settings.", ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
            }
        }
    }

    EndDialogs();
    LOG_INFO(Config, "closed");
    m_open = false;
}

bool SettingsMenu::BeginDialogs() {
    s32 result = sceCommonDialogInitialize();
    if (result < 0 && result != COMMON_DIALOG_ERROR_ALREADY_SYSTEM_INITIALIZED) {
        LOG_ERROR(Config, "sceCommonDialogInitialize failed with error {:#x}",
                  static_cast<u32>(result));
        return false;
    }

    result = sceMsgDialogInitialize();
    if (result == COMMON_DIALOG_ERROR_ALREADY_INITIALIZED) {
        m_owns_msg_dialog = false;
    } else if (result < 0) {
        LOG_ERROR(Config, "sceMsgDialogInitialize failed with error {:#x}",
                  static_cast<u32>(result));
        return false;
    } else {
        m_owns_msg_dialog = true;
    }
    return true;
}

void SettingsMenu::EndDialogs() {
    if (m_owns_msg_dialog) {
        sceMsgDialogTerminate();
        m_owns_msg_dialog = false;
    }
}

SettingsMenu::Choice SettingsMenu::ShowMessage(const std::string& msg, s32 button_type,
                                               const char* button1, const char* button2) {
    if (sceMsgDialogGetStatus() == ORBIS_COMMON_DIALOG_STATUS_RUNNING) {
        LOG_ERROR(Config, "game already has a message dialog open");
        return Choice::Failed;
    }

    OrbisMsgDialogButtonsParam buttons_param{};
    buttons_param.msg1 = button1;
    buttons_param.msg2 = button2;

    OrbisMsgDialogUserMessageParam user_msg_param{};
    user_msg_param.buttonType = static_cast<OrbisMsgDialogButtonType>(button_type);
    user_msg_param.msg = msg.c_str();
    if (button_type == ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS) {
        user_msg_param.buttonsParam = &buttons_param;
    }

    OrbisMsgDialogParam param{};
    param.baseParam.size = sizeof(OrbisCommonDialogBaseParam);
    param.baseParam.magic = static_cast<u32>(ORBIS_COMMON_DIALOG_MAGIC_NUMBER +
                                             reinterpret_cast<u64>(&param.baseParam));
    param.size = sizeof(OrbisMsgDialogParam);
    param.mode = ORBIS_MSG_DIALOG_MODE_USER_MSG;
    param.userMsgParam = &user_msg_param;
    param.userId = m_user_id;

    s32 result = sceMsgDialogOpen(&param);
    if (result < 0) {
        LOG_ERROR(Config, "sceMsgDialogOpen failed with error {:#x}", static_cast<u32>(result));
        return Choice::Failed;
    }

    while (sceMsgDialogUpdateStatus() != ORBIS_COMMON_DIALOG_STATUS_FINISHED) {
        sceKernelUsleep(DIALOG_POLL_INTERVAL);
    }

    OrbisMsgDialogResult dialog_result{};
    result = sceMsgDialogGetResult(&dialog_result);
    if (result < 0) {
        LOG_ERROR(Config, "sceMsgDialogGetResult failed with error {:#x}",
                  static_cast<u32>(result));
        return Choice::Failed;
    }
    if (dialog_result.result == ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED) {
        return Choice::Canceled;
    }
    return dialog_result.buttonId == ORBIS_MSG_DIALOG_BUTTON_ID_BUTTON2 ? Choice::Button2
                                                                        : Choice::Button1;
}

std::optional<std::string> SettingsMenu::ShowKeyboard(const char* title, const std::string& value,
                                                      FieldType type) {
    std::u16string title_text(title, title + std::strlen(title));
    std::u16string input_text(MAX_TEXT_LENGTH + 1, u'\0');
    for (u64 i = 0; i < value.size() && i < MAX_TEXT_LENGTH; i++) {
        input_text[i] = static_cast<u8>(value[i]);
    }

    OrbisImeDialogSetting setting{};
    setting.userId = m_user_id;
    setting.type = type == FieldType::Port  ? ORBIS_TYPE_NUMBER
                   : type == FieldType::Url ? ORBIS_TYPE_TYPE_URL
                                            : ORBIS_TYPE_BASIC_LATIN;
    setting.option = type == FieldType::Password ? 0x4 | 0x2 : 0x2;
    setting.maxTextLength = type == FieldType::Port ? 5 : MAX_TEXT_LENGTH;
    setting.inputTextBuffer = reinterpret_cast<wchar_t*>(input_text.data());
    setting.posx = 1920.0f / 2;
    setting.posy = 1080.0f / 2;
    setting.horizontalAlignment = ORBIS_H_CENTER;
    setting.verticalAlignment = ORBIS_V_CENTER;
    setting.title = reinterpret_cast<const wchar_t*>(title_text.c_str());

    s32 result = sceImeDialogInit(&setting, nullptr);
    if (result < 0) {
        LOG_ERROR(Config, "sceImeDialogInit failed with error {:#x}", static_cast<u32>(result));
        return std::nullopt;
    }

    while (sceImeDialogGetStatus() == ORBIS_DIALOG_STATUS_RUNNING) {
        sceKernelUsleep(DIALOG_POLL_INTERVAL);
    }

    OrbisDialogResult dialog_result{};
    result = sceImeDialogGetResult(&dialog_result);
    sceImeDialogTerm();
    if (result < 0) {
        LOG_ERROR(Config, "sceImeDialogGetResult failed with error {:#x}",
                  static_cast<u32>(result));
        return std::nullopt;
    }
    if (dialog_result.endstatus != ORBIS_DIALOG_OK) {
        return std::nullopt;
    }

    std::string text{};
    for (char16_t c : input_text) {
        if (c == u'\0') {
            break;
        }
        text.push_back(c < 0x80 ? static_cast<char>(c) : '?');
    }
    return text;
}

std::string SettingsMenu::FormatValue(const Field& field) {
    if (auto* setting = std::get_if<Setting<bool>*>(&field.value)) {
        return (*setting)->get() ? "Enabled" : "Disabled";
    }
    if (auto* setting = std::get_if<Setting<s32>*>(&field.value)) {
        return std::to_string((*setting)->get());
    }
    std::string value = std::get<Setting<std::string>*>(field.value)->get();
    if (field.type == FieldType::Password) {
        return value.empty() ? "Not set" : "********";
    }
    return value.empty() ? "Not set" : value;
}

std::string SettingsMenu::FormatSummary() {
    return fmt::format("shadNet Settings\n\n"
                       "shadNet: {}\nNPID: {}\nPassword: {}\nAppear offline: {}\n\n"
                       "Server: {}\nWebApi server: {}\nP2P port: {}",
                       m_user.shadnet_enabled.get() ? "Enabled" : "Disabled",
                       m_user.shadnet_npid.get().empty() ? "Not set" : m_user.shadnet_npid.get(),
                       m_user.shadnet_password.get().empty() ? "Not set" : "********",
                       m_user.shadnet_appear_offline.get() ? "Enabled" : "Disabled",
                       m_server.shadnet_server.get(), m_server.shadnet_webapi_server.get(),
                       m_server.shadnet_p2p_port.get());
}

void SettingsMenu::EditFields(std::vector<Field> fields) {
    u64 index = 0;
    while (index < fields.size()) {
        Field& field = fields[index];
        std::string msg = fmt::format("{}: {}\n\n({}/{})", field.name, FormatValue(field),
                                      index + 1, fields.size());
        Choice choice = ShowMessage(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS, "Next", "Change");
        if (choice == Choice::Button1) {
            index++;
        } else if (choice == Choice::Button2) {
            EditField(field);
        } else {
            return;
        }
    }
}

void SettingsMenu::EditField(Field& field) {
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
            ShowMessage("Invalid port, it must be a number from 1 to 65535.",
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
