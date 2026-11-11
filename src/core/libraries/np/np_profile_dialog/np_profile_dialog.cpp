// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <iterator>
#include <magic_enum/magic_enum.hpp>
#include <orbis/NpProfileDialog.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/common_dialog/common_dialog_error.h"
#include "core/libraries/np/np_profile_dialog/np_profile_dialog_ui.h"

SHADNET_HOOK_DECLARE(Libraries::Np::NpProfileDialog, sceNpProfileDialogClose);
SHADNET_HOOK_DECLARE(Libraries::Np::NpProfileDialog, sceNpProfileDialogGetResult);
SHADNET_HOOK_DECLARE(Libraries::Np::NpProfileDialog, sceNpProfileDialogGetStatus);
SHADNET_HOOK_DECLARE(Libraries::Np::NpProfileDialog, sceNpProfileDialogInitialize);
SHADNET_HOOK_DECLARE(Libraries::Np::NpProfileDialog, sceNpProfileDialogOpen);
SHADNET_HOOK_DECLARE(Libraries::Np::NpProfileDialog, sceNpProfileDialogOpenA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpProfileDialog, sceNpProfileDialogTerminate);
SHADNET_HOOK_DECLARE(Libraries::Np::NpProfileDialog, sceNpProfileDialogUpdateStatus);

static void RegisterNpProfileDialogHooks() {
    SHADNET_HOOK(Libraries::Np::NpProfileDialog, sceNpProfileDialogClose);
    SHADNET_HOOK(Libraries::Np::NpProfileDialog, sceNpProfileDialogGetResult);
    SHADNET_HOOK(Libraries::Np::NpProfileDialog, sceNpProfileDialogGetStatus);
    SHADNET_HOOK(Libraries::Np::NpProfileDialog, sceNpProfileDialogInitialize);
    SHADNET_HOOK(Libraries::Np::NpProfileDialog, sceNpProfileDialogOpen);
    SHADNET_HOOK(Libraries::Np::NpProfileDialog, sceNpProfileDialogOpenA);
    SHADNET_HOOK(Libraries::Np::NpProfileDialog, sceNpProfileDialogTerminate);
    SHADNET_HOOK(Libraries::Np::NpProfileDialog, sceNpProfileDialogUpdateStatus);
}

namespace Libraries::Np::NpProfileDialog {

using CommonDialog::OrbisCommonDialogStatus;

static std::atomic<OrbisCommonDialogStatus> g_status =
    OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE;
static NpProfileDialogUi g_dialog_ui;
static DialogState g_state;

static s32 BeginOpen(s32 mode, s32 user_id, void* user_data) {
    if (g_status != OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_INITIALIZED &&
        g_status != OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_FINISHED) {
        LOG_ERROR(Lib_NpProfileDialog, "called without initialize, status = {}",
                  magic_enum::enum_name(g_status.load()));
        return ORBIS_COMMON_DIALOG_ERROR_INVALID_STATE;
    }
    if (mode < ORBIS_NP_PROFILE_DIALOG_MODE_NORMAL ||
        mode > ORBIS_NP_PROFILE_DIALOG_MODE_GRIEF_REPORT) {
        LOG_ERROR(Lib_NpProfileDialog, "invalid mode {}", mode);
        return ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID;
    }

    g_dialog_ui.Join();
    g_state = DialogState{};
    g_state.mode = mode;
    g_state.user_id = user_id;
    g_state.user_data = user_data;
    return ORBIS_OK;
}

static void StartDialog() {
    g_status = OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_RUNNING;
    g_dialog_ui.Start(&g_status, &g_state);
}

s32 sceNpProfileDialogInitialize() {
    if (g_status != OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE) {
        LOG_ERROR(Lib_NpProfileDialog, "already initialized");
        return ORBIS_COMMON_DIALOG_ERROR_ALREADY_INITIALIZED;
    }
    LOG_INFO(Lib_NpProfileDialog, "called");
    g_status = OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_INITIALIZED;
    return ORBIS_OK;
}

s32 sceNpProfileDialogOpen(const OrbisNpProfileDialogParam* param) {
    if (param == nullptr) {
        return ORBIS_COMMON_DIALOG_ERROR_ARG_NULL;
    }
    if (s32 error = BeginOpen(param->mode, param->user_id, param->user_data); error != ORBIS_OK) {
        return error;
    }
    const auto& online_id = param->target_online_id;
    g_state.online_id.assign(online_id.data, strnlen(online_id.data, sizeof(online_id.data)));
    LOG_INFO(Lib_NpProfileDialog, "mode = {}, user_id = {:#x}, online_id = '{}'", param->mode,
             param->user_id, g_state.online_id);
    StartDialog();
    return ORBIS_OK;
}

s32 sceNpProfileDialogOpenA(const OrbisNpProfileDialogParamA* param) {
    if (param == nullptr) {
        return ORBIS_COMMON_DIALOG_ERROR_ARG_NULL;
    }
    if (s32 error = BeginOpen(param->mode, param->user_id, param->user_data); error != ORBIS_OK) {
        return error;
    }
    g_state.account_id = param->target_account_id;
    LOG_INFO(Lib_NpProfileDialog, "mode = {}, user_id = {:#x}, account_id = {:#x}", param->mode,
             param->user_id, param->target_account_id);
    StartDialog();
    return ORBIS_OK;
}

s32 sceNpProfileDialogGetResult(OrbisNpProfileDialogResult* result) {
    if (result == nullptr) {
        return ORBIS_COMMON_DIALOG_ERROR_ARG_NULL;
    }
    if (std::any_of(std::begin(result->reserved), std::end(result->reserved),
                    [](u8 b) { return b != 0; })) {
        return ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID;
    }
    if (g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE) {
        return ORBIS_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    }
    if (g_status != OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_FINISHED) {
        return ORBIS_COMMON_DIALOG_ERROR_NOT_FINISHED;
    }
    *result = {};
    result->result = ORBIS_OK;
    result->user_action = g_state.result;
    result->user_data = g_state.user_data;
    return result->result;
}

OrbisCommonDialogStatus sceNpProfileDialogGetStatus() {
    return g_status;
}

OrbisCommonDialogStatus sceNpProfileDialogUpdateStatus() {
    return g_status;
}

s32 sceNpProfileDialogClose() {
    if (g_status != OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_RUNNING) {
        LOG_WARNING(Lib_NpProfileDialog, "dialog is not running");
        return ORBIS_COMMON_DIALOG_ERROR_NOT_RUNNING;
    }
    LOG_INFO(Lib_NpProfileDialog, "called");
    g_dialog_ui.Close();
    return ORBIS_OK;
}

s32 sceNpProfileDialogTerminate() {
    if (g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE) {
        LOG_ERROR(Lib_NpProfileDialog, "not initialized");
        return ORBIS_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    }
    if (g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_RUNNING) {
        sceNpProfileDialogClose();
    }
    g_dialog_ui.Join();
    g_status = OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE;
    LOG_INFO(Lib_NpProfileDialog, "called");
    return ORBIS_OK;
}

bool IsRunning() {
    return g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_RUNNING;
}

void RegisterHooks() {
    return RegisterNpProfileDialogHooks();
}
} // namespace Libraries::Np::NpProfileDialog
