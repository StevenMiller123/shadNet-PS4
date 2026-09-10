// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <deque>
#include <absl/container/flat_hash_map.h>
#include <orbis/NpManager.h>
#include <orbis/libkernel.h>
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/plugin_common.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler.h"
#include "core/libraries/np/np_manager.h"

extern "C" {

SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetState);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetNpId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpManager, sceNpGetOnlineId);

void RegisterLibraryHooks() {
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetState);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetNpId);
    SHADNET_HOOK(Libraries::Np::NpManager, sceNpGetOnlineId);
}
}

namespace Libraries::Np::NpManager {

static s32 g_firmware_version = 0;

s32 sceNpGetState(s32 user_id, OrbisNpState* state) {
    LOG_INFO(Lib_NpManager, "called");
    if (user_id == -1 && g_firmware_version >= Common::ElfInfo::FW_900) {
        // FW < 9.00 behavior needs validating.
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!state) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (NpHandler::Instance().IsActive()) {
        *state = ORBIS_NP_STATE_SIGNED_IN;
    } else {
        *state = ORBIS_NP_STATE_SIGNED_OUT;
    }
    return ORBIS_OK;
}

s32 sceNpGetNpId(s32 user_id, OrbisNpId* np_id) {
    LOG_INFO(Lib_NpManager, "called");
    if (user_id == -1) {
        return g_firmware_version >= Common::ElfInfo::FW_900 ? ORBIS_NP_ERROR_INVALID_ARGUMENT
                                                             : ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    if (!np_id) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!NpHandler::Instance().IsActive()) {
        // Not currently connected to shadNet, treat this as signed out.
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    *np_id = NpHandler::Instance().GetNpId(user_id);
    return ORBIS_OK;
}

s32 sceNpGetOnlineId(s32 user_id, OrbisNpOnlineId* online_id) {
    LOG_INFO(Lib_NpManager, "called");
    if (user_id == -1) {
        return g_firmware_version >= Common::ElfInfo::FW_900 ? ORBIS_NP_ERROR_INVALID_ARGUMENT
                                                             : ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    if (!online_id) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!NpHandler::Instance().IsActive()) {
        // Not currently connected to shadNet, treat this as signed out.
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    *online_id = NpHandler::Instance().GetNpId(user_id).handle;
    return ORBIS_OK;
}

// Np callback handling
static absl::flat_hash_map<std::string, std::function<void()>> g_np_callbacks;
static std::mutex g_np_callbacks_mutex;

void RegisterNpCallback(std::string key, std::function<void()> cb) {
    std::scoped_lock lk{g_np_callbacks_mutex};
    LOG_DEBUG(Lib_NpManager, "registering callback processing for {}", key);
    g_np_callbacks.emplace(key, cb);
}

struct PendingNpStateEvent {
    s32 user_id;
    OrbisNpState state;
    OrbisNpId np_id;
    bool has_np_id;
};
static std::deque<PendingNpStateEvent> g_np_state_events;
static std::mutex g_np_state_events_mutex;

static void QueueNpStateEvent(s32 user_id, OrbisNpState state) {
    PendingNpStateEvent event{};
    event.user_id = user_id;
    event.state = state;
    event.has_np_id = state == ORBIS_NP_STATE_SIGNED_IN;
    if (event.has_np_id) {
        event.np_id = Libraries::Np::NpHandler::Instance().GetNpId(user_id);
    }

    std::scoped_lock lk{g_np_state_events_mutex};
    g_np_state_events.emplace_back(event);
}

void RegisterHooks() {
    sceKernelGetCompiledSdkVersion(&g_firmware_version);
    NpHandler::Instance().RegisterStateCallback(
        [](s32 user_id, OrbisNpState state) { QueueNpStateEvent(user_id, state); }, nullptr);

    return RegisterLibraryHooks();
}
} // namespace Libraries::Np::NpManager