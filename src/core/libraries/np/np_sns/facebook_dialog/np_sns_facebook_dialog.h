// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/common_dialog/common_dialog.h"
#include "core/libraries/np/np_sns/np_sns_facebook.h"

namespace Libraries::Np::NpSns::FacebookDialog {

enum OrbisNpSnsFacebookDialogMode : s32 {
    ORBIS_NP_SNS_FACEBOOK_DIALOG_MODE_ACCESS_TOKEN = 0,
};

struct OrbisNpSnsFacebookDialogParam {
    CommonDialog::OrbisCommonDialogBaseParam base_param;
    u64 size;
    OrbisNpSnsFacebookDialogMode mode;
    OrbisNpSnsFacebookAccessTokenParam access_token_param;
    u8 reserved[28];
};
static_assert(sizeof(OrbisNpSnsFacebookDialogParam) == 0x498);

struct OrbisNpSnsFacebookDialogResult {
    OrbisNpSnsFacebookAccessTokenResult access_token_result;
    s32 result;
    u8 reserved[36];
};
static_assert(sizeof(OrbisNpSnsFacebookDialogResult) == 0x1058);

s32 sceNpSnsFacebookDialogInitialize();
s32 sceNpSnsFacebookDialogOpen(const OrbisNpSnsFacebookDialogParam* param);
CommonDialog::OrbisCommonDialogStatus sceNpSnsFacebookDialogUpdateStatus();
CommonDialog::OrbisCommonDialogStatus sceNpSnsFacebookDialogGetStatus();
s32 sceNpSnsFacebookDialogGetResult(OrbisNpSnsFacebookDialogResult* result);
s32 sceNpSnsFacebookDialogClose();
s32 sceNpSnsFacebookDialogTerminate();
void RegisterHooks();

} // namespace Libraries::Np::NpSns::FacebookDialog
