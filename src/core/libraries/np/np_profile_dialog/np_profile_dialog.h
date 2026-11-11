// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/common_dialog/common_dialog.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/user_service/user_service.h"

namespace Libraries::Np::NpProfileDialog {

constexpr s32 ORBIS_NP_PROFILE_DIALOG_MODE_INVALID = 0;
constexpr s32 ORBIS_NP_PROFILE_DIALOG_MODE_NORMAL = 1;
constexpr s32 ORBIS_NP_PROFILE_DIALOG_MODE_FRIEND_REQUEST = 2;
constexpr s32 ORBIS_NP_PROFILE_DIALOG_MODE_ADD_TO_BLOCK_LIST = 3;
constexpr s32 ORBIS_NP_PROFILE_DIALOG_MODE_GRIEF_REPORT = 4;

constexpr s32 ORBIS_NP_PROFILE_DIALOG_MENU_GRIEF_REPORT_ITEM_INVALID = 0x00000000;
constexpr s32 ORBIS_NP_PROFILE_DIALOG_MENU_GRIEF_REPORT_ITEM_ONLINE_ID = 0x00000001;
constexpr s32 ORBIS_NP_PROFILE_DIALOG_MENU_GRIEF_REPORT_ITEM_NAME = 0x00000002;
constexpr s32 ORBIS_NP_PROFILE_DIALOG_MENU_GRIEF_REPORT_ITEM_PICTURE = 0x00000004;
constexpr s32 ORBIS_NP_PROFILE_DIALOG_MENU_GRIEF_REPORT_ITEM_ABOUT_ME = 0x00000008;

struct OrbisNpProfileDialogParam {
    CommonDialog::OrbisCommonDialogBaseParam base_param;
    u64 size;
    s32 mode;
    UserService::OrbisUserServiceUserId user_id;
    OrbisNpOnlineId target_online_id;
    void* user_data;
    u8 reserved[32];
};

struct OrbisNpProfileGriefReportParam {
    s32 report_item;
    u8 reserved[28];
};

struct OrbisNpProfileDialogParamA {
    CommonDialog::OrbisCommonDialogBaseParam base_param;
    u64 size;
    s32 mode;
    UserService::OrbisUserServiceUserId user_id;
    OrbisNpAccountId target_account_id;
    s32 : 32;
    void* user_data;
    union {
        u8 reserved[32];
        OrbisNpProfileGriefReportParam grief_report_param;
    };
};

struct OrbisNpProfileDialogResult {
    s32 result;
    CommonDialog::OrbisCommonDialogResult user_action;
    void* user_data;
    u8 reserved[32];
};

s32 sceNpProfileDialogClose();
s32 sceNpProfileDialogGetResult(OrbisNpProfileDialogResult* result);
CommonDialog::OrbisCommonDialogStatus sceNpProfileDialogGetStatus();
s32 sceNpProfileDialogInitialize();
s32 sceNpProfileDialogOpen(const OrbisNpProfileDialogParam* param);
s32 sceNpProfileDialogOpenA(const OrbisNpProfileDialogParamA* param);
s32 sceNpProfileDialogTerminate();
CommonDialog::OrbisCommonDialogStatus sceNpProfileDialogUpdateStatus();

bool IsRunning();

void RegisterHooks();
} // namespace Libraries::Np::NpProfileDialog
