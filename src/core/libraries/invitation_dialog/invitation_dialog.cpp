// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <magic_enum/magic_enum.hpp>
#include <orbis/InvitationDialog.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/common_dialog/common_dialog_error.h"
#include "core/libraries/invitation_dialog/invitation_dialog.h"
#include "core/libraries/invitation_dialog/invitation_dialog_error.h"
#include "core/libraries/invitation_dialog/invitation_dialog_ui.h"
#include "core/libraries/np/np_handler/np_handler.h"

SHADNET_HOOK_DECLARE(Libraries::InvitationDialog, sceInvitationDialogClose);
SHADNET_HOOK_DECLARE(Libraries::InvitationDialog, sceInvitationDialogGetResult);
SHADNET_HOOK_DECLARE(Libraries::InvitationDialog, sceInvitationDialogGetResultA);
SHADNET_HOOK_DECLARE(Libraries::InvitationDialog, sceInvitationDialogGetStatus);
SHADNET_HOOK_DECLARE(Libraries::InvitationDialog, sceInvitationDialogInitialize);
SHADNET_HOOK_DECLARE(Libraries::InvitationDialog, sceInvitationDialogOpen);
SHADNET_HOOK_DECLARE(Libraries::InvitationDialog, sceInvitationDialogOpenA);
SHADNET_HOOK_DECLARE(Libraries::InvitationDialog, sceInvitationDialogTerminate);
SHADNET_HOOK_DECLARE(Libraries::InvitationDialog, sceInvitationDialogUpdateStatus);

static void RegisterInvitationDialogHooks() {
    SHADNET_HOOK(Libraries::InvitationDialog, sceInvitationDialogClose);
    SHADNET_HOOK(Libraries::InvitationDialog, sceInvitationDialogGetResult);
    SHADNET_HOOK(Libraries::InvitationDialog, sceInvitationDialogGetResultA);
    SHADNET_HOOK(Libraries::InvitationDialog, sceInvitationDialogGetStatus);
    SHADNET_HOOK(Libraries::InvitationDialog, sceInvitationDialogInitialize);
    SHADNET_HOOK(Libraries::InvitationDialog, sceInvitationDialogOpen);
    SHADNET_HOOK(Libraries::InvitationDialog, sceInvitationDialogOpenA);
    SHADNET_HOOK(Libraries::InvitationDialog, sceInvitationDialogTerminate);
    SHADNET_HOOK(Libraries::InvitationDialog, sceInvitationDialogUpdateStatus);
}

namespace Libraries::InvitationDialog {

using CommonDialog::OrbisCommonDialogStatus;

static std::atomic<OrbisCommonDialogStatus> g_status =
    OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE;
static InvitationDialogUi g_dialog_ui;
static DialogState g_state;

static u32 ClampRecipients(u32 count) {
    if (count == 0 || count > ORBIS_INVITATION_DIALOG_ADDRESS_USER_LIST_MAX_SIZE) {
        return ORBIS_INVITATION_DIALOG_ADDRESS_USER_LIST_MAX_SIZE;
    }
    return count;
}

static s32 BeginOpen(s32 mode, s32 user_id, void* callback_arg) {
    if (g_status != OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_INITIALIZED &&
        g_status != OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_FINISHED) {
        LOG_ERROR(Lib_InvitationDialog, "called without initialize, status = {}",
                  magic_enum::enum_name(g_status.load()));
        return ORBIS_COMMON_DIALOG_ERROR_INVALID_STATE;
    }
    if (mode != ORBIS_INVITATION_DIALOG_MODE_SEND && mode != ORBIS_INVITATION_DIALOG_MODE_RECV) {
        LOG_ERROR(Lib_InvitationDialog, "invalid mode {}", mode);
        return ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID;
    }

    g_dialog_ui.Join();
    g_state = DialogState{};
    g_state.mode = mode;
    g_state.callback_arg = callback_arg;
    g_state.user_id = user_id;
    return ORBIS_OK;
}

static void StartDialog() {
    if (g_state.mode == ORBIS_INVITATION_DIALOG_MODE_RECV) {
        LOG_INFO(Lib_InvitationDialog, "mode = recv, user_id = {:#x}", g_state.user_id);
    } else {
        LOG_INFO(
            Lib_InvitationDialog,
            "mode = send, user_id = {:#x}, session_id = '{}', user_editable = {}, recipients = {}",
            g_state.user_id, g_state.session_id, g_state.user_editable,
            g_state.online_ids.size() + g_state.account_ids.size());
    }
    g_status = OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_RUNNING;
    g_dialog_ui.Start(&g_status, &g_state);
}

s32 sceInvitationDialogInitialize() {
    if (g_status != OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE) {
        LOG_ERROR(Lib_InvitationDialog, "already initialized");
        return ORBIS_COMMON_DIALOG_ERROR_ALREADY_INITIALIZED;
    }
    LOG_INFO(Lib_InvitationDialog, "called");
    g_status = OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_INITIALIZED;
    return ORBIS_OK;
}

s32 sceInvitationDialogOpen(const OrbisInvitationDialogParam* param) {
    if (param == nullptr) {
        return ORBIS_COMMON_DIALOG_ERROR_ARG_NULL;
    }
    if (s32 error = BeginOpen(param->mode, param->user_id, param->callback_arg);
        error != ORBIS_OK) {
        return error;
    }

    if (param->mode == ORBIS_INVITATION_DIALOG_MODE_SEND) {
        if (param->data_param == nullptr) {
            return ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID;
        }
        const auto& send = param->data_param->send_info;
        if (send.session_id != nullptr) {
            g_state.session_id = send.session_id->data;
        }
        if (send.user_message != nullptr) {
            g_state.message = send.user_message;
        }
        const auto& address = send.address_param;
        if (address.address_type == ORBIS_INVITATION_DIALOG_ADDRESS_TYPE_USERENABLE) {
            g_state.user_editable = true;
            const auto& addr = address.address_info.user_select_enable_address;
            g_state.max_recipients = ClampRecipients(addr.online_ids_max_count);
        } else if (address.address_type == ORBIS_INVITATION_DIALOG_ADDRESS_TYPE_USERDISABLE) {
            const auto& addr = address.address_info.user_select_disable_address;
            u32 count = std::min<u32>(addr.online_ids_count,
                                      ORBIS_INVITATION_DIALOG_ADDRESS_USER_LIST_MAX_SIZE);
            if (count != 0 && addr.online_ids != nullptr) {
                g_state.online_ids.assign(addr.online_ids, addr.online_ids + count);
            }
        } else {
            LOG_WARNING(Lib_InvitationDialog, "invalid address type {}", address.address_type);
        }
    }

    StartDialog();
    return ORBIS_OK;
}

s32 sceInvitationDialogOpenA(const OrbisInvitationDialogParamA* param) {
    if (param == nullptr) {
        return ORBIS_COMMON_DIALOG_ERROR_ARG_NULL;
    }
    if (s32 error = BeginOpen(param->mode, param->user_id, param->callback_arg);
        error != ORBIS_OK) {
        return error;
    }

    if (param->mode == ORBIS_INVITATION_DIALOG_MODE_SEND) {
        if (param->data_param == nullptr) {
            return ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID;
        }
        const auto& send = param->data_param->send_info;
        if (send.session_id != nullptr) {
            g_state.session_id = send.session_id->data;
        }
        if (send.user_message != nullptr) {
            g_state.message = send.user_message;
        }
        const auto& address = send.address_param;
        if (address.address_type == ORBIS_INVITATION_DIALOG_ADDRESS_TYPE_USERENABLE) {
            g_state.user_editable = true;
            g_state.max_recipients =
                ClampRecipients(address.address_info.user_select_enable_address.user_max_count);
        } else if (address.address_type == ORBIS_INVITATION_DIALOG_ADDRESS_TYPE_USERDISABLE) {
            const auto& addr = address.address_info.user_select_disable_address;
            u32 count = std::min<u32>(addr.account_ids_count,
                                      ORBIS_INVITATION_DIALOG_ADDRESS_USER_LIST_MAX_SIZE);
            if (count != 0 && addr.account_ids != nullptr) {
                g_state.account_ids.assign(addr.account_ids, addr.account_ids + count);
            }
        } else {
            LOG_WARNING(Lib_InvitationDialog, "invalid address type {}", address.address_type);
        }
    }

    StartDialog();
    return ORBIS_OK;
}

s32 sceInvitationDialogGetResult(OrbisInvitationDialogResult* result) {
    if (g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE) {
        return ORBIS_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    }
    if (g_status != OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_FINISHED) {
        return ORBIS_COMMON_DIALOG_ERROR_NOT_FINISHED;
    }
    if (result == nullptr) {
        return ORBIS_COMMON_DIALOG_ERROR_ARG_NULL;
    }
    if (g_state.error_code != 0) {
        return ORBIS_INVITATION_DIALOG_ERROR_BLOCKED_ALL_PLAYERS;
    }

    result->callback_arg = g_state.callback_arg;
    result->error_code = g_state.error_code;
    result->result = g_state.result;
    if (result->sent_online_ids != nullptr) {
        u32 count = std::min<u32>(g_state.sent_online_ids.size(),
                                  ORBIS_INVITATION_DIALOG_ADDRESS_USER_LIST_MAX_SIZE);
        for (u32 i = 0; i < count; i++) {
            auto& online_id = result->sent_online_ids->online_id[i];
            std::memset(&online_id, 0, sizeof(online_id));
            std::strncpy(online_id.data, g_state.sent_online_ids[i].c_str(),
                         sizeof(online_id.data));
        }
        result->sent_online_ids->count = g_state.result == ORBIS_OK ? count : 0;
    }
    return 0;
}

s32 sceInvitationDialogGetResultA(OrbisInvitationDialogResultA* result) {
    if (g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE) {
        return ORBIS_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    }
    if (g_status != OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_FINISHED) {
        return ORBIS_COMMON_DIALOG_ERROR_NOT_FINISHED;
    }
    if (result == nullptr) {
        return ORBIS_COMMON_DIALOG_ERROR_ARG_NULL;
    }
    if (g_state.error_code != 0) {
        return ORBIS_INVITATION_DIALOG_ERROR_BLOCKED_ALL_PLAYERS;
    }

    result->callback_arg = g_state.callback_arg;
    result->error_code = g_state.error_code;
    result->result = g_state.result;
    if (result->sent_users != nullptr) {
        u32 count =
            std::min<u32>(std::max(g_state.sent_online_ids.size(), g_state.sent_account_ids.size()),
                          ORBIS_INVITATION_DIALOG_ADDRESS_USER_LIST_MAX_SIZE);
        for (u32 i = 0; i < count; i++) {
            auto& user = result->sent_users->users[i];
            std::memset(&user, 0, sizeof(user));
            if (i < g_state.sent_online_ids.size()) {
                std::strncpy(user.online_id.data, g_state.sent_online_ids[i].c_str(),
                             sizeof(user.online_id.data));
            }
            if (i < g_state.sent_account_ids.size()) {
                user.account_id = g_state.sent_account_ids[i];
            }
        }
        result->sent_users->count = g_state.result == ORBIS_OK ? count : 0;
    }
    return 0;
}

OrbisCommonDialogStatus sceInvitationDialogGetStatus() {
    return g_status;
}

OrbisCommonDialogStatus sceInvitationDialogUpdateStatus() {
    return g_status;
}

s32 sceInvitationDialogClose() {
    if (g_status != OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_RUNNING) {
        LOG_WARNING(Lib_InvitationDialog, "dialog is not running");
        return ORBIS_COMMON_DIALOG_ERROR_NOT_RUNNING;
    }
    LOG_INFO(Lib_InvitationDialog, "called");
    g_dialog_ui.Close();
    return ORBIS_OK;
}

s32 sceInvitationDialogTerminate() {
    if (g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE) {
        LOG_ERROR(Lib_InvitationDialog, "not initialized");
        return ORBIS_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    }
    if (g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_RUNNING) {
        sceInvitationDialogClose();
    }
    g_dialog_ui.Join();
    g_status = OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE;
    LOG_INFO(Lib_InvitationDialog, "called");
    return ORBIS_OK;
}

bool IsRunning() {
    return g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_RUNNING;
}

void RegisterHooks() {
    return RegisterInvitationDialogHooks();
}
} // namespace Libraries::InvitationDialog
