// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>

#include "common/types.h"
#include "core/libraries/common_dialog/common_dialog.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/user_service/user_service.h"

namespace Libraries::GameCustomDataDialog {

constexpr u32 ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_USER_LIST_MAX_SIZE = 16;
constexpr u32 ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_USER_MSG_SIZE = 512;
constexpr u32 ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_URL_LENGTH = 1024;

constexpr s32 ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_INVALID = 0;
constexpr s32 ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_SEND = 1;
constexpr s32 ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_RECV = 2;
constexpr s32 ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_SEND_URL = 3;

constexpr s32 ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_TYPE_INVALID = 0;
constexpr s32 ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_TYPE_USERDISABLE = 1;
constexpr s32 ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_TYPE_USERENABLE = 2;

union OrbisGameCustomDataDialogAddressInfoA {
    struct {
        const Np::OrbisNpAccountId* account_ids;
        u32 account_ids_count;
        s32 : 32;
    } user_select_disable_address;
    struct {
        u32 user_max_count;
    } user_select_enable_address;
};

union OrbisGameCustomDataDialogAddressInfo {
    struct {
        const Np::OrbisNpOnlineId* online_ids;
        u32 online_ids_count;
        s32 : 32;
    } user_select_disable_address;
    struct {
        u32 user_max_count;
    } user_select_enable_address;
};

struct OrbisGameCustomDataDialogAddressParamA {
    s32 address_type;
    s32 : 32;
    OrbisGameCustomDataDialogAddressInfoA address_info;
};

struct OrbisGameCustomDataDialogAddressParam {
    s32 address_type;
    s32 : 32;
    OrbisGameCustomDataDialogAddressInfo address_info;
};

struct OrbisGameCustomDataDialogPlatformInfo {
    u32 count;
    s32 : 32;
    const char** platform_name;
};

struct OrbisGameCustomDataMultiLanguageString {
    const char* language;
    const char* str;
};

template <typename AddressParam>
union OrbisGameCustomDataDialogDataParamT {
    struct {
        const char* user_message;
        const char* data_name;
        const OrbisGameCustomDataMultiLanguageString* data_name_multi_lang;
        u32 data_name_multi_lang_num;
        s32 : 32;
        const char* data_detail;
        const OrbisGameCustomDataMultiLanguageString* data_detail_multi_lang;
        u32 data_detail_multi_lang_num;
        s32 : 32;
        void* thumbnail;
        u32 thumbnail_size;
        s32 : 32;
        void* data;
        u32 data_size;
        u8 padding[4];
        s32 expire_minutes;
        s32 : 32;
        OrbisGameCustomDataDialogPlatformInfo available_platform;
        AddressParam address_param;
    } send_info;
    struct {
        const char* user_message;
        const char* data_name;
        const OrbisGameCustomDataMultiLanguageString* data_name_multi_lang;
        u32 data_name_multi_lang_num;
        s32 : 32;
        const char* data_detail;
        const OrbisGameCustomDataMultiLanguageString* data_detail_multi_lang;
        u32 data_detail_multi_lang_num;
        s32 : 32;
        void* thumbnail;
        u32 thumbnail_size;
        s32 : 32;
        const char* data_url;
        bool with_item_id;
        u8 padding[7];
        s32 expire_minutes;
        s32 : 32;
        OrbisGameCustomDataDialogPlatformInfo available_platform;
        AddressParam address_param;
    } send_url_info;
    struct {
        u8 reserved[160];
    } recv_info;
};

using OrbisGameCustomDataDialogDataParamA =
    OrbisGameCustomDataDialogDataParamT<OrbisGameCustomDataDialogAddressParamA>;
using OrbisGameCustomDataDialogDataParam =
    OrbisGameCustomDataDialogDataParamT<OrbisGameCustomDataDialogAddressParam>;

template <typename DataParam>
struct OrbisGameCustomDataDialogParamT {
    CommonDialog::OrbisCommonDialogBaseParam base_param;
    u32 size;
    s32 mode;
    UserService::OrbisUserServiceUserId user_id;
    s32 : 32;
    void* callback_arg;
    const DataParam* data_param;
    u8 reserved[64];
};

using OrbisGameCustomDataDialogParamA =
    OrbisGameCustomDataDialogParamT<OrbisGameCustomDataDialogDataParamA>;
using OrbisGameCustomDataDialogParam =
    OrbisGameCustomDataDialogParamT<OrbisGameCustomDataDialogDataParam>;

struct OrbisGameCustomDataDialogUserList {
    u32 count;
    struct {
        Np::OrbisNpOnlineId online_id;
        Np::OrbisNpAccountId account_id;
    } users[ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_USER_LIST_MAX_SIZE];
};

struct OrbisGameCustomDataDialogOnlineIdList {
    u32 count;
    Np::OrbisNpOnlineId online_id[ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_USER_LIST_MAX_SIZE];
};

struct OrbisGameCustomDataDialogResultA {
    void* callback_arg;
    s32 error_code;
    CommonDialog::OrbisCommonDialogResult result;
    OrbisGameCustomDataDialogUserList* sent_users;
    u8 reserved[32];
};

struct OrbisGameCustomDataDialogResult {
    void* callback_arg;
    s32 error_code;
    CommonDialog::OrbisCommonDialogResult result;
    OrbisGameCustomDataDialogOnlineIdList* sent_online_ids;
    u8 reserved[32];
};

static_assert(sizeof(OrbisGameCustomDataDialogParamA) == 0x90);
static_assert(sizeof(OrbisGameCustomDataDialogParam) == 0x90);
static_assert(offsetof(OrbisGameCustomDataDialogResultA, reserved) == 0x18);
static_assert(offsetof(OrbisGameCustomDataDialogResult, reserved) == 0x18);

using OrbisNpGameCustomDataId = u64;

struct OrbisNpGameCustomDataEventParam {
    OrbisNpGameCustomDataId item_id;
    Np::OrbisNpOnlineId online_id;
    UserService::OrbisUserServiceUserId user_id;
};

s32 sceGameCustomDataDialogInitialize();
s32 sceGameCustomDataDialogTerminate();
s32 sceGameCustomDataDialogOpen(const OrbisGameCustomDataDialogParam* param);
s32 sceGameCustomDataDialogOpenA(const OrbisGameCustomDataDialogParamA* param);
s32 sceGameCustomDataDialogClose();
CommonDialog::OrbisCommonDialogStatus sceGameCustomDataDialogUpdateStatus();
CommonDialog::OrbisCommonDialogStatus sceGameCustomDataDialogGetStatus();
s32 sceGameCustomDataDialogGetResult(OrbisGameCustomDataDialogResult* result);
s32 sceGameCustomDataDialogGetResultA(OrbisGameCustomDataDialogResultA* result);

bool IsRunning();

void RegisterHooks();
} // namespace Libraries::GameCustomDataDialog
