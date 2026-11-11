// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <orbis/CommonDialog.h>
#include <orbis/MsgDialog.h>
#include <orbis/Pad.h>
#include <orbis/libkernel.h>

#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/libraries/common_dialog/common_dialog_error.h"
#include "shadnet/dialogs/message_dialog.h"

namespace ShadNet {

static constexpr u32 DIALOG_POLL_INTERVAL = 16000;
static constexpr s32 PAD_DATA_INDEX_COUNT = 4;

bool MessageDialog::Begin(s32 user_id) {
    m_user_id = user_id;
    m_aborted = false;

    s32 result = sceCommonDialogInitialize();
    if (result < 0 && result != ORBIS_COMMON_DIALOG_ERROR_ALREADY_SYSTEM_INITIALIZED) {
        LOG_ERROR(shadNet, "sceCommonDialogInitialize failed with error {:#x}",
                  static_cast<u32>(result));
        return false;
    }

    result = sceMsgDialogInitialize();
    if (result == ORBIS_COMMON_DIALOG_ERROR_ALREADY_INITIALIZED) {
        m_owns_msg_dialog = false;
    } else if (result < 0) {
        LOG_ERROR(shadNet, "sceMsgDialogInitialize failed with error {:#x}",
                  static_cast<u32>(result));
        return false;
    } else {
        m_owns_msg_dialog = true;
    }
    return true;
}

void MessageDialog::End() {
    if (m_owns_msg_dialog) {
        sceMsgDialogTerminate();
        m_owns_msg_dialog = false;
    }
}

MessageDialog::Choice MessageDialog::Show(const std::string& msg, s32 button_type,
                                          const char* button1, const char* button2,
                                          Libraries::Pad::Button buttons) {
    if (m_aborted) {
        return Choice::Canceled;
    }
    if (sceMsgDialogGetStatus() == ORBIS_COMMON_DIALOG_STATUS_RUNNING) {
        LOG_ERROR(shadNet, "game already has a message dialog open");
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
        LOG_ERROR(shadNet, "sceMsgDialogOpen failed with error {:#x}", static_cast<u32>(result));
        return Choice::Failed;
    }

    u32 active_buttons = 0;
    bool closed = false;
    while (sceMsgDialogUpdateStatus() != ORBIS_COMMON_DIALOG_STATUS_FINISHED) {
        for (s32 i = 0; i < PAD_DATA_INDEX_COUNT; i++) {
            OrbisPadData data{};
            scePadGetDataInternal(i, &data);
            active_buttons |= (data.buttons & u32(buttons));
        }
        if ((m_aborted || active_buttons) && !closed) {
            sceMsgDialogClose();
            closed = true;
        }
        sceKernelUsleep(DIALOG_POLL_INTERVAL);
    }
    if (m_aborted) {
        return Choice::Canceled;
    }
    if (active_buttons) {
        if ((active_buttons & ORBIS_PAD_BUTTON_L1)) {
            return Choice::L1;
        }
        if ((active_buttons & ORBIS_PAD_BUTTON_L2)) {
            return Choice::L2;
        }
        if ((active_buttons & ORBIS_PAD_BUTTON_L3)) {
            return Choice::L3;
        }
        if ((active_buttons & ORBIS_PAD_BUTTON_R1)) {
            return Choice::R1;
        }
        if ((active_buttons & ORBIS_PAD_BUTTON_R2)) {
            return Choice::R2;
        }
        if ((active_buttons & ORBIS_PAD_BUTTON_R3)) {
            return Choice::R3;
        }
        if ((active_buttons & ORBIS_PAD_BUTTON_UP)) {
            return Choice::Up;
        }
        if ((active_buttons & ORBIS_PAD_BUTTON_DOWN)) {
            return Choice::Down;
        }
        if ((active_buttons & ORBIS_PAD_BUTTON_LEFT)) {
            return Choice::Left;
        }
        if ((active_buttons & ORBIS_PAD_BUTTON_RIGHT)) {
            return Choice::Right;
        }
        if ((active_buttons & ORBIS_PAD_BUTTON_CROSS)) {
            return Choice::Cross;
        }
        if ((active_buttons & ORBIS_PAD_BUTTON_CIRCLE)) {
            return Choice::Circle;
        }
        if ((active_buttons & ORBIS_PAD_BUTTON_TRIANGLE)) {
            return Choice::Triangle;
        }
        if ((active_buttons & ORBIS_PAD_BUTTON_SQUARE)) {
            return Choice::Square;
        }
        if ((active_buttons & ORBIS_PAD_BUTTON_OPTIONS)) {
            return Choice::Options;
        }
        if ((active_buttons & ORBIS_PAD_BUTTON_TOUCH_PAD)) {
            return Choice::TouchPad;
        }
    }

    OrbisMsgDialogResult dialog_result{};
    result = sceMsgDialogGetResult(&dialog_result);
    if (result < 0) {
        LOG_ERROR(shadNet, "sceMsgDialogGetResult failed with error {:#x}",
                  static_cast<u32>(result));
        return Choice::Failed;
    }
    if (dialog_result.result == ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED) {
        return Choice::Canceled;
    }
    return dialog_result.buttonId == ORBIS_MSG_DIALOG_BUTTON_ID_BUTTON2 ? Choice::Button2
                                                                        : Choice::Button1;
}

std::optional<std::string> MessageDialog::ShowKeyboard(const char* title, const std::string& value,
                                                       OrbisImeType type, bool password,
                                                       u32 max_length) {
    const std::u16string title_text = Common::UTF8ToUTF16(title);
    std::u16string input_text = Common::UTF8ToUTF16(value);
    input_text.resize(max_length + 1, u'\0');
    input_text[max_length] = u'\0';

    OrbisImeDialogSetting setting{};
    setting.userId = m_user_id;
    setting.type = type;
    setting.option = password ? 0x4 | 0x2 : 0x2;
    setting.maxTextLength = max_length;
    setting.inputTextBuffer = reinterpret_cast<wchar_t*>(input_text.data());
    setting.posx = 1920.0f / 2;
    setting.posy = 1080.0f / 2;
    setting.horizontalAlignment = ORBIS_H_CENTER;
    setting.verticalAlignment = ORBIS_V_CENTER;
    setting.title = reinterpret_cast<const wchar_t*>(title_text.c_str());

    s32 result = sceImeDialogInit(&setting, nullptr);
    if (result < 0) {
        LOG_ERROR(shadNet, "sceImeDialogInit failed with error {:#x}", static_cast<u32>(result));
        return std::nullopt;
    }

    while (sceImeDialogGetStatus() == ORBIS_DIALOG_STATUS_RUNNING) {
        sceKernelUsleep(DIALOG_POLL_INTERVAL);
    }

    OrbisDialogResult dialog_result{};
    result = sceImeDialogGetResult(&dialog_result);
    sceImeDialogTerm();
    if (result < 0) {
        LOG_ERROR(shadNet, "sceImeDialogGetResult failed with error {:#x}",
                  static_cast<u32>(result));
        return std::nullopt;
    }
    if (dialog_result.endstatus != ORBIS_DIALOG_OK) {
        return std::nullopt;
    }

    return Common::UTF16ToUTF8(input_text.c_str());
}

void MessageDialog::Abort() {
    m_aborted = true;
}

} // namespace ShadNet
