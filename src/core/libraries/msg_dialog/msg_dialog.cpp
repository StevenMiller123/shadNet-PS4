// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <condition_variable>
#include <mutex>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/common_dialog/common_dialog_error.h"
#include "core/libraries/msg_dialog/msg_dialog.h"

SHADNET_HOOK_DECLARE(Libraries::MsgDialog, sceMsgDialogInitialize);
SHADNET_HOOK_DECLARE(Libraries::MsgDialog, sceMsgDialogOpen);
SHADNET_HOOK_DECLARE(Libraries::MsgDialog, sceMsgDialogClose);
SHADNET_HOOK_DECLARE(Libraries::MsgDialog, sceMsgDialogTerminate);
SHADNET_HOOK_DECLARE(Libraries::MsgDialog, sceMsgDialogGetResult);
SHADNET_HOOK_DECLARE(Libraries::MsgDialog, sceMsgDialogUpdateStatus);
SHADNET_HOOK_DECLARE(Libraries::MsgDialog, sceMsgDialogGetStatus);

static void RegisterMsgDialogHooks() {
    SHADNET_HOOK(Libraries::MsgDialog, sceMsgDialogInitialize);
    SHADNET_HOOK(Libraries::MsgDialog, sceMsgDialogOpen);
    SHADNET_HOOK(Libraries::MsgDialog, sceMsgDialogClose);
    SHADNET_HOOK(Libraries::MsgDialog, sceMsgDialogTerminate);
    SHADNET_HOOK(Libraries::MsgDialog, sceMsgDialogGetResult);
    SHADNET_HOOK(Libraries::MsgDialog, sceMsgDialogUpdateStatus);
    SHADNET_HOOK(Libraries::MsgDialog, sceMsgDialogGetStatus);
}

namespace Libraries::MsgDialog {

static thread_local bool t_plugin_thread = false;
static std::atomic<bool> g_plugin_owns{false};
static std::mutex g_owner_mutex;
static std::condition_variable g_owner_cv;

void SetPluginThread(bool plugin_thread) {
    t_plugin_thread = plugin_thread;
}

static bool BlockGameCall() {
    // Checks if this is a game thread calling while plugin dialogs are running.
    return !t_plugin_thread && g_plugin_owns.load();
}

s32 sceMsgDialogInitialize() {
    std::unique_lock lock{g_owner_mutex};
    if (!t_plugin_thread && g_plugin_owns) {
        // Wait for the plugin dialog to end
        LOG_WARNING(Lib_MsgDialog, "called while plugin dialogs are running");
        g_owner_cv.wait(lock, [] { return !g_plugin_owns.load(); });
    }
    const s32 ret = SHADNET_HOOK_CONTINUE(sceMsgDialogInitialize);
    if (t_plugin_thread && ret == ORBIS_OK) {
        g_plugin_owns = true;
    }
    LOG_INFO(Lib_MsgDialog, "returned {:#x}", static_cast<u32>(ret));
    return ret;
}

s32 sceMsgDialogOpen(const OrbisMsgDialogParam* param) {
    if (BlockGameCall()) {
        LOG_INFO(Lib_MsgDialog,
                 "called while plugin dialogs are running, returning NOT_INITIALIZED");
        return ORBIS_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    }
    const s32 ret = SHADNET_HOOK_CONTINUE(sceMsgDialogOpen, param);
    if (param != nullptr) {
        LOG_INFO(Lib_MsgDialog, "mode = {}, user_id = {:#x}, returned {:#x}",
                 static_cast<s32>(param->mode), param->userId, static_cast<u32>(ret));
    } else {
        LOG_INFO(Lib_MsgDialog, "param = null, returned {:#x}", static_cast<u32>(ret));
    }
    return ret;
}

s32 sceMsgDialogClose() {
    if (BlockGameCall()) {
        LOG_INFO(Lib_MsgDialog,
                 "called while plugin dialogs are running, returning NOT_INITIALIZED");
        return ORBIS_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    }
    const s32 ret = SHADNET_HOOK_CONTINUE(sceMsgDialogClose);
    LOG_INFO(Lib_MsgDialog, "returned {:#x}", static_cast<u32>(ret));
    return ret;
}

s32 sceMsgDialogTerminate() {
    if (BlockGameCall()) {
        LOG_INFO(Lib_MsgDialog,
                 "called while plugin dialogs are running, returning NOT_INITIALIZED");
        return ORBIS_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    }
    std::unique_lock lock{g_owner_mutex, std::defer_lock};
    if (t_plugin_thread) {
        lock.lock();
    }
    const s32 ret = SHADNET_HOOK_CONTINUE(sceMsgDialogTerminate);
    if (t_plugin_thread && ret == ORBIS_OK && g_plugin_owns) {
        g_plugin_owns = false;
        g_owner_cv.notify_all();
    }
    LOG_INFO(Lib_MsgDialog, "returned {:#x}", static_cast<u32>(ret));
    return ret;
}

s32 sceMsgDialogGetResult(OrbisMsgDialogResult* result) {
    if (BlockGameCall()) {
        return ORBIS_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
    }
    const s32 ret = SHADNET_HOOK_CONTINUE(sceMsgDialogGetResult, result);
    if (result != nullptr) {
        LOG_DEBUG(Lib_MsgDialog, "result = {}, button_id = {}, returned {:#x}", result->result,
                  static_cast<s32>(result->buttonId), static_cast<u32>(ret));
    } else {
        LOG_DEBUG(Lib_MsgDialog, "result = null, returned {:#x}", static_cast<u32>(ret));
    }
    return ret;
}

OrbisCommonDialogStatus sceMsgDialogUpdateStatus() {
    if (BlockGameCall()) {
        return ORBIS_COMMON_DIALOG_STATUS_NONE;
    }
    return SHADNET_HOOK_CONTINUE(sceMsgDialogUpdateStatus);
}

OrbisCommonDialogStatus sceMsgDialogGetStatus() {
    if (BlockGameCall()) {
        return ORBIS_COMMON_DIALOG_STATUS_NONE;
    }
    return SHADNET_HOOK_CONTINUE(sceMsgDialogGetStatus);
}

void RegisterHooks() {
    RegisterMsgDialogHooks();
}

} // namespace Libraries::MsgDialog
