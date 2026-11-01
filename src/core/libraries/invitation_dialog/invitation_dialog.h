// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/common_dialog/common_dialog.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/user_service/user_service.h"

namespace Libraries::InvitationDialog {

constexpr s32 ORBIS_INVITATION_DIALOG_ADDRESS_USER_LIST_MAX_SIZE = 16;
constexpr s32 ORBIS_INVITATION_DIALOG_MAX_USER_MSG_SIZE = 512;

struct OrbisInvitationDialogUserList {
    u32 count;
    struct {
        Np::OrbisNpOnlineId online_id;
        Np::OrbisNpAccountId account_id;
    } users[ORBIS_INVITATION_DIALOG_ADDRESS_USER_LIST_MAX_SIZE];
};

struct OrbisInvitationDialogOnlineIdList {
    u32 count;
    Np::OrbisNpOnlineId online_id[ORBIS_INVITATION_DIALOG_ADDRESS_USER_LIST_MAX_SIZE];
};

struct OrbisInvitationDialogResultA {
    void* callback_arg;
    s32 error_code;
    CommonDialog::OrbisCommonDialogResult result;
    OrbisInvitationDialogUserList* sent_users;
    u8 reserved[32];
};

struct OrbisInvitationDialogResult {
    void* callback_arg;
    s32 error_code;
    CommonDialog::OrbisCommonDialogResult result;
    OrbisInvitationDialogOnlineIdList* sent_online_ids;
    u8 reserved[32];
};

constexpr s32 ORBIS_INVITATION_DIALOG_MODE_INVALID = 0;
constexpr s32 ORBIS_INVITATION_DIALOG_MODE_SEND = 1;
constexpr s32 ORBIS_INVITATION_DIALOG_MODE_RECV = 2;

union OrbisInvitationDialogAddressInfoA {
    struct {
        const Np::OrbisNpAccountId* account_ids;
        u32 account_ids_count;
        s32 : 32;
    } user_select_disable_address;
    struct {
        u32 user_max_count;
    } user_select_enable_address;
};

union OrbisInvitationDialogAddressInfo {
    struct {
        const Np::OrbisNpOnlineId* online_ids;
        u32 online_ids_count;
    } user_select_disable_address;
    struct {
        u32 online_ids_max_count;
    } user_select_enable_address;
};

constexpr s32 ORBIS_INVITATION_DIALOG_ADDRESS_TYPE_INVALID = 0;
constexpr s32 ORBIS_INVITATION_DIALOG_ADDRESS_TYPE_USERDISABLE = 1;
constexpr s32 ORBIS_INVITATION_DIALOG_ADDRESS_TYPE_USERENABLE = 2;

struct OrbisInvitationDialogAddressParam {
    s32 address_type;
    OrbisInvitationDialogAddressInfo address_info;
};

struct OrbisInvitationDialogAddressParamA {
    s32 address_type;
    s32 reserved;
    OrbisInvitationDialogAddressInfoA address_info;
};

union OrbisInvitationDialogDataParam {
    struct {
        const char* user_message;
        const Np::OrbisNpSessionId* session_id;
        OrbisInvitationDialogAddressParam address_param;
    } send_info;
    struct {
        u8 reserved[64];
    } recv_info;
};

union OrbisInvitationDialogDataParamA {
    struct {
        const char* user_message;
        const Np::OrbisNpSessionId* session_id;
        OrbisInvitationDialogAddressParamA address_param;
    } send_info;
    struct {
        u8 reserved[64];
    } recv_info;
};

struct OrbisInvitationDialogParam {
    CommonDialog::OrbisCommonDialogBaseParam base_param;
    u32 size;
    s32 mode;
    UserService::OrbisUserServiceUserId user_id;
    void* callback_arg;
    OrbisInvitationDialogDataParam* data_param;
    u8 reserved[64];
};

struct OrbisInvitationDialogParamA {
    CommonDialog::OrbisCommonDialogBaseParam base_param;
    u32 size;
    s32 mode;
    UserService::OrbisUserServiceUserId user_id;
    s32 reserved1;
    void* callback_arg;
    const OrbisInvitationDialogDataParamA* data_param;
    u8 reserved2[64];
};

s32 sceInvitationDialogClose();
s32 sceInvitationDialogGetResult(OrbisInvitationDialogResult* result);
s32 sceInvitationDialogGetResultA(OrbisInvitationDialogResultA* result);
CommonDialog::OrbisCommonDialogStatus sceInvitationDialogGetStatus();
s32 sceInvitationDialogInitialize();
s32 sceInvitationDialogOpen(const OrbisInvitationDialogParam* param);
s32 sceInvitationDialogOpenA(const OrbisInvitationDialogParamA* param);
s32 sceInvitationDialogTerminate();
CommonDialog::OrbisCommonDialogStatus sceInvitationDialogUpdateStatus();

bool IsRunning();

void RegisterHooks();
} // namespace Libraries::InvitationDialog
