// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/CommonDialog.h>
#include <orbis/MsgDialog.h>
#include <orbis/Pad.h>
#include <orbis/UserService.h>
#include <orbis/libkernel.h>

#include "common/logging/log.h"
#include "shadnet/dialogs/message_dialog.h"

namespace ShadNet {

static constexpr u32 DIALOG_POLL_INTERVAL = 16000;

static constexpr s32 COMMON_DIALOG_ERROR_ALREADY_SYSTEM_INITIALIZED = 0x80B80002;
static constexpr s32 COMMON_DIALOG_ERROR_ALREADY_INITIALIZED = 0x80B80004;

bool MessageDialog::Begin(s32 user_id) {
    m_user_id = user_id;
    m_aborted = false;

    s32 result = sceCommonDialogInitialize();
    if (result < 0 && result != COMMON_DIALOG_ERROR_ALREADY_SYSTEM_INITIALIZED) {
        LOG_ERROR(shadNet, "sceCommonDialogInitialize failed with error {:#x}",
                  static_cast<u32>(result));
        return false;
    }

    result = sceMsgDialogInitialize();
    if (result == COMMON_DIALOG_ERROR_ALREADY_INITIALIZED) {
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
                                          const char* button1, const char* button2) {
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

    bool closed = false;
    while (sceMsgDialogUpdateStatus() != ORBIS_COMMON_DIALOG_STATUS_FINISHED) {
        if (m_aborted && !closed) {
            sceMsgDialogClose();
            closed = true;
        }
        sceKernelUsleep(DIALOG_POLL_INTERVAL);
    }
    if (m_aborted) {
        return Choice::Canceled;
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

MessageDialog::Choice MessageDialog::ShowMulti(const std::string& msg, const char* choice1,
                                               const char* choice2, const char* choice3,
                                               const char* choice4) {
    if (m_aborted) {
        return Choice::Canceled;
    }
    if (sceMsgDialogGetStatus() == ORBIS_COMMON_DIALOG_STATUS_RUNNING) {
        LOG_ERROR(shadNet, "game already has a message dialog open");
        return Choice::Failed;
    }

#define S(str) str ? str : ""
    std::string text =
        fmt::format("{}\n{}\n{}\n{}\n{}", msg, S(choice1), S(choice2), S(choice3), S(choice4));
#undef S

    OrbisMsgDialogUserMessageParam user_msg_param{};
    user_msg_param.buttonType = ORBIS_MSG_DIALOG_BUTTON_TYPE_WAIT_CANCEL;
    user_msg_param.msg = text.c_str();

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

    OrbisPadData pad_data{};

    s32 button_selection = 0;
    bool closed = false;
    while (sceMsgDialogUpdateStatus() != ORBIS_COMMON_DIALOG_STATUS_FINISHED) {
        // int scePadGetDataInternal(int index, OrbisPadData* data);
        scePadGetDataInternal(0, &pad_data);
        if ((pad_data.buttons & (ORBIS_PAD_BUTTON_UP | ORBIS_PAD_BUTTON_DOWN |
                                 ORBIS_PAD_BUTTON_LEFT | ORBIS_PAD_BUTTON_RIGHT)) != 0 ||
            false) {
            button_selection = pad_data.buttons;
        }

        if ((m_aborted || button_selection != 0) && !closed) {
            sceMsgDialogClose();
            closed = true;
        }
        sceKernelUsleep(DIALOG_POLL_INTERVAL);
    }

    if (m_aborted && button_selection == 0) {
        return Choice::Canceled;
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
    return (button_selection & ORBIS_PAD_BUTTON_UP) != 0      ? Choice::Button1
           : (button_selection & ORBIS_PAD_BUTTON_LEFT) != 0  ? Choice::Button2
           : (button_selection & ORBIS_PAD_BUTTON_RIGHT) != 0 ? Choice::Button3
           : (button_selection & ORBIS_PAD_BUTTON_DOWN) != 0  ? Choice::Button4
                                                              : Choice::Failed;
}

void MessageDialog::Abort() {
    m_aborted = true;
}

} // namespace ShadNet
