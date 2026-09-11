// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include <absl/container/flat_hash_map.h>
#include "common/logging/log.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler.h"
#include "core/libraries/np/np_manager/np_callbacks.h"
#include "core/libraries/np/np_manager/np_manager.h"

namespace Libraries::Np::NpManager {

static std::mutex g_np_callbacks_mutex;
static std::mutex g_np_state_events_mutex;
static std::mutex g_np_state_callbacks_mutex;

struct NpReachabilityStateCallback {
    OrbisNpReachabilityStateCallback func;
    void* userdata;
};
NpReachabilityStateCallback g_np_reachability_state_cb;
static absl::flat_hash_map<s32, OrbisNpReachabilityState> g_np_reachability_state_last;

s32 sceNpRegisterNpReachabilityStateCallback(OrbisNpReachabilityStateCallback callback,
                                             void* userdata) {
    if (!callback) {
        LOG_ERROR(Lib_NpManager, "null callback");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{g_np_state_callbacks_mutex};
    if (g_np_reachability_state_cb.func) {
        LOG_ERROR(Lib_NpManager, "callback already registered");
        return ORBIS_NP_ERROR_CALLBACK_ALREADY_REGISTERED;
    }

    LOG_INFO(Lib_NpManager, "registering callback");
    g_np_reachability_state_cb.func = callback;
    g_np_reachability_state_cb.userdata = userdata;
    g_np_reachability_state_last.clear();
    return ORBIS_OK;
}

s32 sceNpUnregisterNpReachabilityStateCallback() {
    std::scoped_lock lk{g_np_state_callbacks_mutex};
    if (!g_np_reachability_state_cb.func) {
        LOG_ERROR(Lib_NpManager, "no callback registered");
        return ORBIS_NP_ERROR_CALLBACK_NOT_REGISTERED;
    }

    LOG_INFO(Lib_NpManager, "clearing callback");
    g_np_reachability_state_cb.func = nullptr;
    g_np_reachability_state_cb.userdata = nullptr;
    g_np_reachability_state_last.clear();
    return ORBIS_OK;
}

struct NpStateCallbackForToolkit {
    OrbisNpStateCallbackForToolkit func;
    void* userdata;
};
NpStateCallbackForToolkit g_np_state_for_toolkit_cb;

s32 sceNpRegisterStateCallbackForToolkit(OrbisNpStateCallbackForToolkit callback, void* userdata) {
    if (!callback) {
        LOG_ERROR(Lib_NpManager, "null callback");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{g_np_state_callbacks_mutex};
    if (g_np_state_for_toolkit_cb.func) {
        LOG_ERROR(Lib_NpManager, "callback already registered");
        return ORBIS_NP_ERROR_CALLBACK_ALREADY_REGISTERED;
    }

    LOG_INFO(Lib_NpManager, "registering callback");
    g_np_state_for_toolkit_cb.func = callback;
    g_np_state_for_toolkit_cb.userdata = userdata;
    return ORBIS_OK;
}

s32 sceNpUnregisterStateCallbackForToolkit() {
    std::scoped_lock lk{g_np_state_callbacks_mutex};
    if (!g_np_state_for_toolkit_cb.func) {
        LOG_ERROR(Lib_NpManager, "no callback registered");
        return ORBIS_NP_ERROR_CALLBACK_NOT_REGISTERED;
    }

    LOG_INFO(Lib_NpManager, "clearing callback");
    g_np_state_for_toolkit_cb.func = nullptr;
    g_np_state_for_toolkit_cb.userdata = nullptr;
    return ORBIS_OK;
}

struct NpStateCallback {
    OrbisNpStateCallback func;
    void* userdata;
};
NpStateCallback g_np_state_cb;

s32 sceNpRegisterStateCallback(OrbisNpStateCallback callback, void* userdata) {
    if (!callback) {
        LOG_ERROR(Lib_NpManager, "null callback");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{g_np_state_callbacks_mutex};
    if (g_np_state_cb.func) {
        LOG_ERROR(Lib_NpManager, "callback already registered");
        return ORBIS_NP_ERROR_CALLBACK_ALREADY_REGISTERED;
    }

    LOG_INFO(Lib_NpManager, "registering callback");
    g_np_state_cb.func = callback;
    g_np_state_cb.userdata = userdata;
    return ORBIS_OK;
}

s32 sceNpUnregisterStateCallback() {
    std::scoped_lock lk{g_np_state_callbacks_mutex};
    if (!g_np_state_cb.func) {
        LOG_ERROR(Lib_NpManager, "no callback registered");
        return ORBIS_NP_ERROR_CALLBACK_NOT_REGISTERED;
    }

    LOG_INFO(Lib_NpManager, "clearing callback");
    g_np_state_cb.func = nullptr;
    g_np_state_cb.userdata = nullptr;
    return ORBIS_OK;
}

struct NpStateCallbackA {
    OrbisNpStateCallbackA func;
    void* userdata;
    bool in_use;
};
static std::array<NpStateCallbackA, 8> g_np_state_a_cbs{};

s32 sceNpRegisterStateCallbackA(OrbisNpStateCallbackA callback, void* userdata) {
    if (!callback) {
        LOG_ERROR(Lib_NpManager, "null callback");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{g_np_state_callbacks_mutex};
    for (const auto& cb_entry : g_np_state_a_cbs) {
        if (cb_entry.in_use && cb_entry.func == callback) {
            LOG_ERROR(Lib_NpManager, "callback for {:#x} is already registered",
                      reinterpret_cast<u64>(callback));
            return ORBIS_NP_ERROR_CALLBACK_ALREADY_REGISTERED;
        }
    }
    for (u64 i = 0; i < g_np_state_a_cbs.size(); i++) {
        auto& cb_entry = g_np_state_a_cbs[i];
        if (cb_entry.in_use) {
            continue;
        }
        cb_entry.func = callback;
        cb_entry.userdata = userdata;
        cb_entry.in_use = true;
        LOG_INFO(Lib_NpManager, "registered callback {:#x} to id {}",
                 reinterpret_cast<u64>(callback), i + 1);
        return static_cast<s32>(i + 1);
    }
    LOG_ERROR(Lib_NpManager, "too many registered callbacks");
    return ORBIS_NP_ERROR_CALLBACK_MAX;
}

s32 sceNpUnregisterStateCallbackA(s32 callback_id) {
    if (callback_id <= 0 || callback_id > g_np_state_a_cbs.size()) {
        LOG_ERROR(Lib_NpManager, "invalid callback id {}", callback_id);
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{g_np_state_callbacks_mutex};
    auto& cb_entry = g_np_state_a_cbs[callback_id - 1];
    if (!cb_entry.in_use) {
        LOG_ERROR(Lib_NpManager, "callback id {} is not registered", callback_id);
        return ORBIS_NP_ERROR_CALLBACK_NOT_REGISTERED;
    }
    cb_entry = {};
    LOG_INFO(Lib_NpManager, "unregistered callback with id {}", callback_id);
    return ORBIS_OK;
}

struct PendingNpStateEvent {
    s32 user_id;
    OrbisNpState state;
    OrbisNpId np_id;
    bool has_np_id;
};
static std::deque<PendingNpStateEvent> g_np_state_events;

void QueueNpStateEvent(s32 user_id, OrbisNpState state) {
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

static absl::flat_hash_map<std::string, std::function<void()>> g_np_callbacks;

void RegisterNpCallback(std::string key, std::function<void()> cb) {
    std::scoped_lock lk{g_np_callbacks_mutex};
    LOG_DEBUG(Lib_NpManager, "registering callback processing for {}", key);
    g_np_callbacks.emplace(key, cb);
}

void UpdateNpStateFromEvent(s32 event_type, s32 user_id) {
    auto& np_handler = NpHandler::Instance();
    if (!np_handler.IsActive()) {
        QueueNpStateEvent(user_id, ORBIS_NP_STATE_SIGNED_OUT);
        return;
    }

    switch (event_type) {
    case 0:
        np_handler.OnUserLoggedIn(user_id);
        break;
    case 1:
        np_handler.OnUserLoggedOut(user_id);
        break;
    default:
        LOG_ERROR(Lib_NpManager, "unhandled event type {}", event_type);
        break;
    }
}

static void DispatchPendingNpStateCallbacks() {
    std::deque<PendingNpStateEvent> pending_events;
    NpStateCallback legacy_callback{};
    NpStateCallbackForToolkit toolkit_callback{};
    NpReachabilityStateCallback reachability_callback{};
    std::array<NpStateCallbackA, 8> callbacks;
    std::vector<std::pair<s32, OrbisNpReachabilityState>> reachability_changes;
    {
        std::scoped_lock lk{g_np_state_events_mutex, g_np_state_callbacks_mutex};
        if (g_np_state_events.empty()) {
            return;
        }
        pending_events.swap(g_np_state_events);
        legacy_callback = g_np_state_cb;
        toolkit_callback = g_np_state_for_toolkit_cb;
        callbacks = g_np_state_a_cbs;
        reachability_callback = g_np_reachability_state_cb;

        if (reachability_callback.func) {
            for (const auto& event : pending_events) {
                const OrbisNpReachabilityState reach = event.state == ORBIS_NP_STATE_SIGNED_IN
                                                           ? OrbisNpReachabilityState::Reachable
                                                           : OrbisNpReachabilityState::Unavailable;
                auto it = g_np_reachability_state_last.find(event.user_id);
                if (it == g_np_reachability_state_last.end() || it->second != reach) {
                    g_np_reachability_state_last[event.user_id] = reach;
                    reachability_changes.emplace_back(event.user_id, reach);
                }
            }
        }
    }

    for (auto& event : pending_events) {
        if (legacy_callback.func) {
            legacy_callback.func(event.user_id, event.state,
                                 event.has_np_id ? &event.np_id : nullptr,
                                 legacy_callback.userdata);
        }

        for (const auto& entry : callbacks) {
            if (entry.in_use && entry.func) {
                entry.func(event.user_id, event.state, entry.userdata);
            }
        }

        if (toolkit_callback.func) {
            toolkit_callback.func(event.user_id, event.state, toolkit_callback.userdata);
        }
    }

    for (const auto& [user_id, reach] : reachability_changes) {
        reachability_callback.func(user_id, reach, reachability_callback.userdata);
    }
}

s32 sceNpCheckCallback() {
    DispatchPendingNpStateCallbacks();
    std::scoped_lock lk{g_np_callbacks_mutex};
    for (auto& [key, cb] : g_np_callbacks) {
        cb();
    }
    return ORBIS_OK;
}

s32 sceNpCheckCallbackForLib() {
    DispatchPendingNpStateCallbacks();
    return ORBIS_OK;
}

} // namespace Libraries::Np::NpManager