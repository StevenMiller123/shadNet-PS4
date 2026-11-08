// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include <orbis/SigninDialog.h>

#include "common/shadnet_hook.h"
#include "core/libraries/signin_dialog/signin_dialog.h"
#include "core/libraries/signin_dialog/signin_dialog_error.h"

SHADNET_HOOK_DECLARE(Libraries::SigninDialog, sceSigninDialogInitialize);
SHADNET_HOOK_DECLARE(Libraries::SigninDialog, sceSigninDialogOpen);
SHADNET_HOOK_DECLARE(Libraries::SigninDialog, sceSigninDialogUpdateStatus);
SHADNET_HOOK_DECLARE(Libraries::SigninDialog, sceSigninDialogGetStatus);
SHADNET_HOOK_DECLARE(Libraries::SigninDialog, sceSigninDialogGetResult);
SHADNET_HOOK_DECLARE(Libraries::SigninDialog, sceSigninDialogClose);
SHADNET_HOOK_DECLARE(Libraries::SigninDialog, sceSigninDialogTerminate);

static void RegisterSigninDialogHooks() {
    SHADNET_HOOK(Libraries::SigninDialog, sceSigninDialogInitialize);
    SHADNET_HOOK(Libraries::SigninDialog, sceSigninDialogOpen);
    SHADNET_HOOK(Libraries::SigninDialog, sceSigninDialogUpdateStatus);
    SHADNET_HOOK(Libraries::SigninDialog, sceSigninDialogGetStatus);
    SHADNET_HOOK(Libraries::SigninDialog, sceSigninDialogGetResult);
    SHADNET_HOOK(Libraries::SigninDialog, sceSigninDialogClose);
    SHADNET_HOOK(Libraries::SigninDialog, sceSigninDialogTerminate);
}

namespace Libraries::SigninDialog {

OrbisSigninDialogStatus g_status = ORBIS_SIGNIN_DIALOG_STATUS_NONE;
std::mutex g_mtx{};

s32 sceSigninDialogInitialize() {
    std::scoped_lock lk{g_mtx};
    if (g_status != ORBIS_SIGNIN_DIALOG_STATUS_NONE) {
        return ORBIS_SIGNIN_DIALOG_ERROR_ALREADY_INITIALIZED;
    }
    g_status = ORBIS_SIGNIN_DIALOG_STATUS_INITIALIZED;
    return 0;
}

s32 sceSigninDialogOpen(const OrbisSigninDialogParam* param) {
    if (!param || param->size != sizeof(*param) || param->reserved[0] != 0 ||
        param->reserved[1] != 0) {
        return ORBIS_SIGNIN_DIALOG_ERROR_PARAM_INVALID;
    }
    std::scoped_lock lk{g_mtx};
    if (g_status == ORBIS_SIGNIN_DIALOG_STATUS_NONE) {
        return ORBIS_SIGNIN_DIALOG_ERROR_NOT_INITIALIZED;
    }
    if (g_status == ORBIS_SIGNIN_DIALOG_STATUS_RUNNING) {
        return ORBIS_SIGNIN_DIALOG_ERROR_INVALID_STATE;
    }
    LOG_INFO(Lib_SigninDialog, "(STUBBED) called");
    g_status = ORBIS_SIGNIN_DIALOG_STATUS_FINISHED;
    return 0;
}

OrbisSigninDialogStatus sceSigninDialogUpdateStatus() {
    std::scoped_lock lk{g_mtx};
    return g_status;
}

OrbisSigninDialogStatus sceSigninDialogGetStatus() {
    std::scoped_lock lk{g_mtx};
    return g_status;
}

s32 sceSigninDialogGetResult(OrbisSigninDialogResult* result) {
    if (!result) {
        return ORBIS_SIGNIN_DIALOG_ERROR_PARAM_INVALID;
    }
    std::scoped_lock lk{g_mtx};
    if (g_status == ORBIS_SIGNIN_DIALOG_STATUS_NONE) {
        return ORBIS_SIGNIN_DIALOG_ERROR_NOT_INITIALIZED;
    }
    if (g_status == ORBIS_SIGNIN_DIALOG_STATUS_RUNNING) {
        return ORBIS_SIGNIN_DIALOG_ERROR_INVALID_STATE;
    }
    result->result = ORBIS_SIGNIN_DIALOG_RESULT_OK;
    return 0;
}

s32 sceSigninDialogClose() {
    std::scoped_lock lk{g_mtx};
    if (g_status == ORBIS_SIGNIN_DIALOG_STATUS_NONE) {
        return ORBIS_SIGNIN_DIALOG_ERROR_NOT_INITIALIZED;
    }
    return 0;
}

s32 sceSigninDialogTerminate() {
    std::scoped_lock lk{g_mtx};
    if (g_status == ORBIS_SIGNIN_DIALOG_STATUS_NONE) {
        return ORBIS_SIGNIN_DIALOG_ERROR_NOT_INITIALIZED;
    }
    g_status = ORBIS_SIGNIN_DIALOG_STATUS_NONE;
    return 0;
}

void RegisterHooks() {
    return RegisterSigninDialogHooks();
}

} // namespace Libraries::SigninDialog