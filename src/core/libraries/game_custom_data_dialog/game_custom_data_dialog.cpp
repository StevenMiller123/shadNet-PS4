// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include <iterator>
#include <string>
#include <string_view>
#include <magic_enum/magic_enum.hpp>
#include <orbis/GameCustomDataDialog.h>
#include <orbis/UserService.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/common_dialog/common_dialog_error.h"
#include "core/libraries/game_custom_data_dialog/game_custom_data_dialog.h"
#include "core/libraries/game_custom_data_dialog/game_custom_data_dialog_ui.h"

SHADNET_HOOK_DECLARE(Libraries::GameCustomDataDialog, sceGameCustomDataDialogInitialize);
SHADNET_HOOK_DECLARE(Libraries::GameCustomDataDialog, sceGameCustomDataDialogTerminate);
SHADNET_HOOK_DECLARE(Libraries::GameCustomDataDialog, sceGameCustomDataDialogOpen);
SHADNET_HOOK_DECLARE(Libraries::GameCustomDataDialog, sceGameCustomDataDialogOpenA);
SHADNET_HOOK_DECLARE(Libraries::GameCustomDataDialog, sceGameCustomDataDialogClose);
SHADNET_HOOK_DECLARE(Libraries::GameCustomDataDialog, sceGameCustomDataDialogUpdateStatus);
SHADNET_HOOK_DECLARE(Libraries::GameCustomDataDialog, sceGameCustomDataDialogGetStatus);
SHADNET_HOOK_DECLARE(Libraries::GameCustomDataDialog, sceGameCustomDataDialogGetResult);
SHADNET_HOOK_DECLARE(Libraries::GameCustomDataDialog, sceGameCustomDataDialogGetResultA);

static void RegisterGameCustomDataDialogHooks() {
    SHADNET_HOOK(Libraries::GameCustomDataDialog, sceGameCustomDataDialogInitialize);
    SHADNET_HOOK(Libraries::GameCustomDataDialog, sceGameCustomDataDialogTerminate);
    SHADNET_HOOK(Libraries::GameCustomDataDialog, sceGameCustomDataDialogOpen);
    SHADNET_HOOK(Libraries::GameCustomDataDialog, sceGameCustomDataDialogOpenA);
    SHADNET_HOOK(Libraries::GameCustomDataDialog, sceGameCustomDataDialogClose);
    SHADNET_HOOK(Libraries::GameCustomDataDialog, sceGameCustomDataDialogUpdateStatus);
    SHADNET_HOOK(Libraries::GameCustomDataDialog, sceGameCustomDataDialogGetStatus);
    SHADNET_HOOK(Libraries::GameCustomDataDialog, sceGameCustomDataDialogGetResult);
    SHADNET_HOOK(Libraries::GameCustomDataDialog, sceGameCustomDataDialogGetResultA);
}

namespace Libraries::GameCustomDataDialog {

using CommonDialog::OrbisCommonDialogResult;
using CommonDialog::OrbisCommonDialogStatus;

static constexpr u32 PARAM_SIZE = sizeof(OrbisGameCustomDataDialogParamA);
static constexpr u64 MAX_MESSAGE_BYTES = 2048;
static constexpr u64 MAX_NAME_BYTES = 256;
static constexpr u64 MAX_DETAIL_BYTES = 2048;
static constexpr u32 MAX_THUMBNAIL_SIZE = 160 * 1024;
static constexpr u32 MAX_DATA_SIZE = 1024 * 1024;
static constexpr u32 MAX_MULTI_LANG_STRINGS = 30;
static constexpr u64 MAX_PLATFORM_NAME_BYTES = 16;
static constexpr u64 MAX_LANGUAGE_BYTES = 16;

static std::atomic<OrbisCommonDialogStatus> g_status =
    OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE;
static GameCustomDataDialogUi g_dialog_ui;
static DialogState g_state;

template <typename T>
static bool AllZero(const T& bytes) {
    return std::all_of(std::begin(bytes), std::end(bytes), [](u8 b) { return b == 0; });
}

static std::string BoundedString(const char* s, u64 max) {
    return s == nullptr ? std::string{} : std::string(s, strnlen(s, max));
}

static u64 Utf8Length(std::string_view s) {
    return static_cast<u64>(
        std::count_if(s.begin(), s.end(), [](char c) { return (c & 0xC0) != 0x80; }));
}

static bool IsLoggedIn(UserService::OrbisUserServiceUserId user_id) {
    OrbisUserServiceLoginUserIdList list{};
    if (user_id == -1 || sceUserServiceGetLoginUserIdList(&list) < 0) {
        return false;
    }
    return std::find(std::begin(list.userId), std::end(list.userId), user_id) !=
           std::end(list.userId);
}

template <typename S, typename... Args>
static s32 Reject(s32 error, const S& why, Args&&... args) {
    LOG_ERROR(Lib_GameCustomDataDialog, "open rejected: {} ({:#x})",
              fmt::format(why, std::forward<Args>(args)...), static_cast<u32>(error));
    return error;
}

static bool ValidRecipient(const Np::OrbisNpAccountId& id) {
    return id != 0;
}

static bool ValidRecipient(const Np::OrbisNpOnlineId& id) {
    return strnlen(id.data, sizeof(id.data)) != 0;
}

template <typename Send>
static s32 ValidateCommonSend(const Send& send) {
    if (send.user_message == nullptr) {
        return Reject(ORBIS_COMMON_DIALOG_ERROR_ARG_NULL, "user_message is null");
    }
    const u64 msg_bytes = strnlen(send.user_message, MAX_MESSAGE_BYTES + 1);
    if (msg_bytes > MAX_MESSAGE_BYTES) {
        return Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID, "user_message longer than {} bytes",
                      MAX_MESSAGE_BYTES);
    }
    if (const u64 chars = Utf8Length({send.user_message, msg_bytes});
        chars > ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_USER_MSG_SIZE) {
        return Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID,
                      "user_message has {} characters, max {}", chars,
                      ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_USER_MSG_SIZE);
    }
    if (send.data_name == nullptr) {
        return Reject(ORBIS_COMMON_DIALOG_ERROR_ARG_NULL, "data_name is null");
    }
    const u64 name_bytes = strnlen(send.data_name, MAX_NAME_BYTES + 1);
    if (name_bytes == 0 || name_bytes > MAX_NAME_BYTES) {
        return Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID,
                      "data_name is {} bytes, must be 1..{}", name_bytes, MAX_NAME_BYTES);
    }
    if (send.data_detail == nullptr) {
        return Reject(ORBIS_COMMON_DIALOG_ERROR_ARG_NULL, "data_detail is null");
    }
    const u64 detail_bytes = strnlen(send.data_detail, MAX_DETAIL_BYTES + 1);
    if (detail_bytes == 0 || detail_bytes > MAX_DETAIL_BYTES) {
        return Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID,
                      "data_detail is {} bytes, must be 1..{}", detail_bytes, MAX_DETAIL_BYTES);
    }
    if (send.thumbnail == nullptr) {
        return Reject(ORBIS_COMMON_DIALOG_ERROR_ARG_NULL, "thumbnail is null");
    }
    if (send.thumbnail_size == 0 || send.thumbnail_size > MAX_THUMBNAIL_SIZE) {
        return Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID, "thumbnail_size {} must be 1..{}",
                      send.thumbnail_size, MAX_THUMBNAIL_SIZE);
    }
    return ORBIS_OK;
}

template <typename Send>
static s32 ValidatePlatformsAndRecipients(const Send& send) {
    const auto& address = send.address_param;
    if (address.address_type != ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_TYPE_USERDISABLE &&
        address.address_type != ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_TYPE_USERENABLE) {
        return Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID, "unknown address_type {}",
                      address.address_type);
    }
    const auto& platforms = send.available_platform;
    if (platforms.count == 0) {
        return Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID, "available_platform.count is 0");
    }
    if (platforms.platform_name == nullptr) {
        return Reject(ORBIS_COMMON_DIALOG_ERROR_ARG_NULL,
                      "available_platform.platform_name is null");
    }
    for (u32 i = 0; i < platforms.count; i++) {
        const char* name = platforms.platform_name[i];
        if (name == nullptr) {
            return Reject(ORBIS_COMMON_DIALOG_ERROR_ARG_NULL,
                          "available_platform.platform_name[{}] is null", i);
        }
        if (strnlen(name, MAX_PLATFORM_NAME_BYTES) == 0) {
            return Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID,
                          "available_platform.platform_name[{}] is empty", i);
        }
    }
    if (address.address_type == ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_TYPE_USERENABLE) {
        const u32 max = address.address_info.user_select_enable_address.user_max_count;
        if (max == 0 || max > ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_USER_LIST_MAX_SIZE) {
            return Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID,
                          "user_max_count {} must be 1..{}", max,
                          ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_USER_LIST_MAX_SIZE);
        }
    } else {
        const auto& fixed = address.address_info.user_select_disable_address;
        u32 count;
        const auto* ids = [&] {
            if constexpr (requires { fixed.account_ids; }) {
                count = fixed.account_ids_count;
                return fixed.account_ids;
            } else {
                count = fixed.online_ids_count;
                return fixed.online_ids;
            }
        }();
        if (count == 0 || count > ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_USER_LIST_MAX_SIZE) {
            return Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID,
                          "recipient count {} must be 1..{}", count,
                          ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_USER_LIST_MAX_SIZE);
        }
        if (ids == nullptr) {
            return Reject(ORBIS_COMMON_DIALOG_ERROR_ARG_NULL, "recipient list is null");
        }
        for (u32 i = 0; i < count; i++) {
            if (!ValidRecipient(ids[i])) {
                return Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID, "recipient[{}] is empty", i);
            }
        }
    }
    if (send.data_name_multi_lang_num > MAX_MULTI_LANG_STRINGS ||
        send.data_detail_multi_lang_num > MAX_MULTI_LANG_STRINGS) {
        return Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID,
                      "too many translations (data_name {}, data_detail {}), max {}",
                      send.data_name_multi_lang_num, send.data_detail_multi_lang_num,
                      MAX_MULTI_LANG_STRINGS);
    }
    return ORBIS_OK;
}

template <typename Param>
static s32 ValidateParam(const Param* param) {
    if (param == nullptr) {
        return Reject(ORBIS_COMMON_DIALOG_ERROR_ARG_NULL, "param is null");
    }
    if (param->size != PARAM_SIZE) {
        return Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID,
                      "param->size is {:#x}, expected {:#x}", param->size, PARAM_SIZE);
    }
    if (!AllZero(param->reserved)) {
        return Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID, "param->reserved is not zeroed");
    }
    if (param->data_param == nullptr) {
        return Reject(ORBIS_COMMON_DIALOG_ERROR_ARG_NULL, "param->data_param is null");
    }
    if (!IsLoggedIn(param->user_id)) {
        return Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID, "user {:#x} is not logged in",
                      param->user_id);
    }
    const auto& data = *param->data_param;
    switch (param->mode) {
    case ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_RECV:
        return AllZero(data.recv_info.reserved) ? ORBIS_OK
                                                : Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID,
                                                         "recv_info.reserved is not zeroed");
    case ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_SEND: {
        const auto& send = data.send_info;
        if (const s32 error = ValidateCommonSend(send); error != ORBIS_OK) {
            return error;
        }
        if (send.data == nullptr) {
            return Reject(ORBIS_COMMON_DIALOG_ERROR_ARG_NULL, "data is null");
        }
        if (send.data_size == 0 || send.data_size > MAX_DATA_SIZE) {
            return Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID, "data_size {} must be 1..{}",
                          send.data_size, MAX_DATA_SIZE);
        }
        return ValidatePlatformsAndRecipients(send);
    }
    case ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_SEND_URL: {
        const auto& send = data.send_url_info;
        if (const s32 error = ValidateCommonSend(send); error != ORBIS_OK) {
            return error;
        }
        if (send.data_url == nullptr) {
            return Reject(ORBIS_COMMON_DIALOG_ERROR_ARG_NULL, "data_url is null");
        }
        const u64 url_bytes =
            strnlen(send.data_url, ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_URL_LENGTH + 1);
        if (url_bytes == 0 || url_bytes > ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_URL_LENGTH) {
            return Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID,
                          "data_url is {} bytes, must be 1..{}", url_bytes,
                          ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_URL_LENGTH);
        }
        return ValidatePlatformsAndRecipients(send);
    }
    default:
        return Reject(ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID, "unknown mode {}", param->mode);
    }
}

template <typename Send>
static void CaptureSend(const Send& send) {
    auto& state = g_state;
    state.data_name = BoundedString(send.data_name, MAX_NAME_BYTES);
    state.data_detail = BoundedString(send.data_detail, MAX_DETAIL_BYTES);
    state.message = BoundedString(send.user_message, MAX_MESSAGE_BYTES);
    state.expire_minutes = send.expire_minutes;
    for (u32 i = 0; i < send.available_platform.count; i++) {
        state.platforms.push_back(
            BoundedString(send.available_platform.platform_name[i], MAX_PLATFORM_NAME_BYTES));
    }
    if constexpr (requires { send.data_url; }) {
        state.data_url = BoundedString(send.data_url, ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_URL_LENGTH);
        state.with_item_id = send.with_item_id;
    } else {
        const auto* data = static_cast<const u8*>(send.data);
        state.game_data.assign(data, data + send.data_size);
    }
    const auto* thumbnail = static_cast<const u8*>(send.thumbnail);
    state.thumbnail.assign(thumbnail, thumbnail + send.thumbnail_size);

    const auto translations = [](const OrbisGameCustomDataMultiLanguageString* list, u32 count,
                                 u64 max, auto& out) {
        for (u32 i = 0; list != nullptr && i < count; i++) {
            if (list[i].language != nullptr && list[i].str != nullptr) {
                out.emplace_back(BoundedString(list[i].language, MAX_LANGUAGE_BYTES),
                                 BoundedString(list[i].str, max));
            }
        }
    };
    translations(send.data_name_multi_lang, send.data_name_multi_lang_num, MAX_NAME_BYTES,
                 state.localized_names);
    translations(send.data_detail_multi_lang, send.data_detail_multi_lang_num, MAX_DETAIL_BYTES,
                 state.localized_details);

    const auto& address = send.address_param;
    if (address.address_type == ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_TYPE_USERENABLE) {
        state.user_editable = true;
        state.user_max_count = address.address_info.user_select_enable_address.user_max_count;
        return;
    }
    const auto& fixed = address.address_info.user_select_disable_address;
    if constexpr (requires { fixed.account_ids; }) {
        state.fixed_account_ids.assign(fixed.account_ids,
                                       fixed.account_ids + fixed.account_ids_count);
    } else {
        for (u32 i = 0; i < fixed.online_ids_count; i++) {
            const auto& id = fixed.online_ids[i].data;
            state.fixed_online_ids.emplace_back(id, strnlen(id, sizeof(id)));
        }
    }
}

template <typename Param>
static s32 OpenImpl(const Param* param) {
    if (g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE) {
        LOG_ERROR(Lib_GameCustomDataDialog, "called before initialize");
        return ORBIS_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    }
    if (const s32 error = ValidateParam(param); error != ORBIS_OK) {
        return error;
    }
    if (g_status != OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_INITIALIZED &&
        g_status != OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_FINISHED) {
        LOG_ERROR(Lib_GameCustomDataDialog, "dialog is already {}",
                  magic_enum::enum_name(g_status.load()));
        return ORBIS_COMMON_DIALOG_ERROR_INVALID_STATE;
    }

    g_dialog_ui.Join();
    g_state = DialogState{};
    g_state.mode = param->mode;
    g_state.user_id = param->user_id;
    g_state.callback_arg = param->callback_arg;
    const auto& data = *param->data_param;
    if (param->mode == ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_SEND) {
        CaptureSend(data.send_info);
    } else if (param->mode == ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_SEND_URL) {
        CaptureSend(data.send_url_info);
    }
    LOG_INFO(Lib_GameCustomDataDialog,
             "mode = {}, user_id = {:#x}, data_name = '{}', game_data = {} bytes, url = '{}'",
             param->mode, param->user_id, g_state.data_name, g_state.game_data.size(),
             g_state.data_url);

    g_status = OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_RUNNING;
    g_dialog_ui.Start(&g_status, &g_state);
    return ORBIS_OK;
}

template <typename ResultT>
static s32 GetResultImpl(ResultT* result) {
    if (result == nullptr) {
        return ORBIS_COMMON_DIALOG_ERROR_ARG_NULL;
    }
    if (!AllZero(result->reserved)) {
        return ORBIS_COMMON_DIALOG_ERROR_PARAM_INVALID;
    }
    if (g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE) {
        return ORBIS_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    }
    if (g_status != OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_FINISHED) {
        return ORBIS_COMMON_DIALOG_ERROR_NOT_FINISHED;
    }

    const bool sent = g_state.result == OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_OK;
    const u64 count = sent ? std::min<u64>(std::max(g_state.sent_online_ids.size(),
                                                    g_state.sent_account_ids.size()),
                                           ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_USER_LIST_MAX_SIZE)
                           : 0;
    result->callback_arg = g_state.callback_arg;
    result->error_code = 0;
    result->result = g_state.result;
    const auto copy_online_id = [](Np::OrbisNpOnlineId& dst, const std::string& src) {
        std::strncpy(dst.data, src.c_str(), sizeof(dst.data));
    };
    if constexpr (requires { result->sent_users; }) {
        if (result->sent_users != nullptr) {
            std::memset(result->sent_users, 0, sizeof(*result->sent_users));
            result->sent_users->count = static_cast<u32>(count);
            for (u64 i = 0; i < count; i++) {
                auto& user = result->sent_users->users[i];
                if (i < g_state.sent_online_ids.size()) {
                    copy_online_id(user.online_id, g_state.sent_online_ids[i]);
                }
                if (i < g_state.sent_account_ids.size()) {
                    user.account_id = g_state.sent_account_ids[i];
                }
            }
        }
    } else if (result->sent_online_ids != nullptr) {
        std::memset(result->sent_online_ids, 0, sizeof(*result->sent_online_ids));
        const u64 online = std::min<u64>(count, g_state.sent_online_ids.size());
        result->sent_online_ids->count = static_cast<u32>(online);
        for (u64 i = 0; i < online; i++) {
            copy_online_id(result->sent_online_ids->online_id[i], g_state.sent_online_ids[i]);
        }
    }
    LOG_INFO(Lib_GameCustomDataDialog, "result = {}, {} recipient(s)",
             magic_enum::enum_name(g_state.result), count);
    return result->error_code;
}

s32 sceGameCustomDataDialogInitialize() {
    if (g_status != OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE) {
        LOG_ERROR(Lib_GameCustomDataDialog, "already initialized");
        return ORBIS_COMMON_DIALOG_ERROR_ALREADY_INITIALIZED;
    }
    LOG_INFO(Lib_GameCustomDataDialog, "called");
    g_status = OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_INITIALIZED;
    return ORBIS_OK;
}

s32 sceGameCustomDataDialogTerminate() {
    if (g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE) {
        LOG_ERROR(Lib_GameCustomDataDialog, "not initialized");
        return ORBIS_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    }
    if (g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_RUNNING) {
        g_dialog_ui.Finish(OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED);
        g_dialog_ui.Close();
    }
    g_dialog_ui.Join();
    g_status = OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE;
    LOG_INFO(Lib_GameCustomDataDialog, "called");
    return ORBIS_OK;
}

s32 sceGameCustomDataDialogOpen(const OrbisGameCustomDataDialogParam* param) {
    return OpenImpl(param);
}

s32 sceGameCustomDataDialogOpenA(const OrbisGameCustomDataDialogParamA* param) {
    return OpenImpl(param);
}

s32 sceGameCustomDataDialogClose() {
    if (g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_NONE) {
        return ORBIS_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    }
    if (g_status != OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_RUNNING) {
        return ORBIS_COMMON_DIALOG_ERROR_NOT_RUNNING;
    }
    LOG_INFO(Lib_GameCustomDataDialog, "called");
    g_dialog_ui.Close();
    return ORBIS_OK;
}

OrbisCommonDialogStatus sceGameCustomDataDialogUpdateStatus() {
    return g_status;
}

OrbisCommonDialogStatus sceGameCustomDataDialogGetStatus() {
    return g_status;
}

s32 sceGameCustomDataDialogGetResult(OrbisGameCustomDataDialogResult* result) {
    return GetResultImpl(result);
}

s32 sceGameCustomDataDialogGetResultA(OrbisGameCustomDataDialogResultA* result) {
    return GetResultImpl(result);
}

bool IsRunning() {
    return g_status == OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_RUNNING;
}

void RegisterHooks() {
    return RegisterGameCustomDataDialogHooks();
}
} // namespace Libraries::GameCustomDataDialog
