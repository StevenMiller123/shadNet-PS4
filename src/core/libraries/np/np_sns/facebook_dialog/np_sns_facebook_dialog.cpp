// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <iterator>
#include <mutex>
#include <orbis/CommonDialog.h>
#include <orbis/NpSnsFacebookDialog.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/common_dialog/common_dialog_error.h"
#include "core/libraries/np/np_sns/facebook_dialog/np_sns_facebook_dialog.h"

SHADNET_HOOK_DECLARE(Libraries::Np::NpSns::FacebookDialog, sceNpSnsFacebookDialogInitialize);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSns::FacebookDialog, sceNpSnsFacebookDialogOpen);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSns::FacebookDialog, sceNpSnsFacebookDialogUpdateStatus);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSns::FacebookDialog, sceNpSnsFacebookDialogGetStatus);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSns::FacebookDialog, sceNpSnsFacebookDialogGetResult);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSns::FacebookDialog, sceNpSnsFacebookDialogClose);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSns::FacebookDialog, sceNpSnsFacebookDialogTerminate);

static void RegisterNpSnsFacebookDialogHooks() {
    SHADNET_HOOK(Libraries::Np::NpSns::FacebookDialog, sceNpSnsFacebookDialogInitialize);
    SHADNET_HOOK(Libraries::Np::NpSns::FacebookDialog, sceNpSnsFacebookDialogOpen);
    SHADNET_HOOK(Libraries::Np::NpSns::FacebookDialog, sceNpSnsFacebookDialogUpdateStatus);
    SHADNET_HOOK(Libraries::Np::NpSns::FacebookDialog, sceNpSnsFacebookDialogGetStatus);
    SHADNET_HOOK(Libraries::Np::NpSns::FacebookDialog, sceNpSnsFacebookDialogGetResult);
    SHADNET_HOOK(Libraries::Np::NpSns::FacebookDialog, sceNpSnsFacebookDialogClose);
    SHADNET_HOOK(Libraries::Np::NpSns::FacebookDialog, sceNpSnsFacebookDialogTerminate);
}

namespace Libraries::Np::NpSns::FacebookDialog {

using CommonDialog::OrbisCommonDialogStatus;

static std::mutex g_mtx;
static OrbisCommonDialogStatus g_status = OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE;

template <size_t N>
static bool AllZero(const u8 (&bytes)[N]) {
    return std::all_of(std::begin(bytes), std::end(bytes), [](u8 b) { return b == 0; });
}

s32 sceNpSnsFacebookDialogInitialize() {
    std::scoped_lock lk{g_mtx};
    if (g_status != OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE) {
        return ORBIS_COMMON_DIALOG_ERROR_ALREADY_INITIALIZED;
    }
    if (sceCommonDialogIsUsed()) {
        return ORBIS_COMMON_DIALOG_ERROR_BUSY;
    }
    LOG_INFO(Lib_NpSnsFacebookDialog, "called");
    g_status = OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_INITIALIZED;
    return ORBIS_OK;
}

s32 sceNpSnsFacebookDialogOpen(const OrbisNpSnsFacebookDialogParam* param) {
    if (param == nullptr) {
        return ORBIS_COMMON_DIALOG_ERROR_ARG_NULL;
    }
    std::scoped_lock lk{g_mtx};
    if (param->size != sizeof(OrbisNpSnsFacebookDialogParam) ||
        param->mode != ORBIS_NP_SNS_FACEBOOK_DIALOG_MODE_ACCESS_TOKEN ||
        !AllZero(param->reserved)) {
        return ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID;
    }
    const auto& token_param = param->access_token_param;
    if (token_param.size != sizeof(OrbisNpSnsFacebookAccessTokenParam) ||
        token_param.user_id == -1 || !AllZero(token_param.reserved) || token_param.fb_app_id == 0) {
        return ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID;
    }
    if (g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE) {
        return ORBIS_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    }
    // Stub a cancel, since we're not going to provide any Facebook stuff anytime soon.
    LOG_INFO(Lib_NpSnsFacebookDialog, "(STUBBED) user_id = {:#x}, finishing as canceled",
             token_param.user_id);
    g_status = OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_FINISHED;
    return ORBIS_OK;
}

OrbisCommonDialogStatus sceNpSnsFacebookDialogUpdateStatus() {
    std::scoped_lock lk{g_mtx};
    return g_status;
}

OrbisCommonDialogStatus sceNpSnsFacebookDialogGetStatus() {
    std::scoped_lock lk{g_mtx};
    return g_status;
}

s32 sceNpSnsFacebookDialogGetResult(OrbisNpSnsFacebookDialogResult* result) {
    if (result == nullptr) {
        return ORBIS_COMMON_DIALOG_ERROR_ARG_NULL;
    }
    std::scoped_lock lk{g_mtx};
    if (g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE) {
        return ORBIS_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    }
    if (!AllZero(result->reserved)) {
        return ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID;
    }
    if (g_status != OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_FINISHED) {
        return ORBIS_COMMON_DIALOG_ERROR_NOT_FINISHED;
    }
    result->result = CommonDialog::ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED;
    LOG_INFO(Lib_NpSnsFacebookDialog, "returning USER_CANCELED");
    return ORBIS_OK;
}

s32 sceNpSnsFacebookDialogClose() {
    std::scoped_lock lk{g_mtx};
    if (g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE) {
        return ORBIS_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    }
    return ORBIS_OK;
}

s32 sceNpSnsFacebookDialogTerminate() {
    std::scoped_lock lk{g_mtx};
    if (g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE) {
        return ORBIS_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    }
    LOG_INFO(Lib_NpSnsFacebookDialog, "called");
    g_status = OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE;
    return ORBIS_OK;
}

void RegisterHooks() {
    RegisterNpSnsFacebookDialogHooks();
}

} // namespace Libraries::Np::NpSns::FacebookDialog
