// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/user_service/user_service.h"

namespace Libraries::SigninDialog {

enum OrbisSigninDialogResultType : s32 {
    ORBIS_SIGNIN_DIALOG_RESULT_OK = 0,
    ORBIS_SIGNIN_DIALOG_RESULT_USER_CANCELED = 1,
};

enum OrbisSigninDialogStatus : s32 {
    ORBIS_SIGNIN_DIALOG_STATUS_NONE = 0,
    ORBIS_SIGNIN_DIALOG_STATUS_INITIALIZED = 1,
    ORBIS_SIGNIN_DIALOG_STATUS_RUNNING = 2,
    ORBIS_SIGNIN_DIALOG_STATUS_FINISHED = 3,
};

struct OrbisSigninDialogResult {
    OrbisSigninDialogResultType result;
    s32 reserved[3];
};

struct OrbisSigninDialogParam {
    s32 size;
    UserService::OrbisUserServiceUserId user_id;
    s32 reserved[2];
};

s32 sceSigninDialogInitialize();
s32 sceSigninDialogOpen(const OrbisSigninDialogParam* param);
OrbisSigninDialogStatus sceSigninDialogUpdateStatus();
OrbisSigninDialogStatus sceSigninDialogGetStatus();
s32 sceSigninDialogGetResult(OrbisSigninDialogResult* result);
s32 sceSigninDialogClose();
s32 sceSigninDialogTerminate();
void RegisterHooks();

} // namespace Libraries::SigninDialog