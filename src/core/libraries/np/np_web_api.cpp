// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <deque>
#include <mutex>
#include "core/libraries/np/np_web_api.h"

namespace Libraries::Np::NpWebApi {

static std::recursive_mutex g_global_mutex;
std::mutex g_push_mutex;
std::deque<PushEventInput> g_push_queue;

template <typename Filter>
bool filterMatches(const Filter* filter, const std::string& service_name, bool has_service_name,
                   const std::string& data_type) {
    const bool catch_all =
        filter.np_service_name.empty() && filter->np_service_label == 0xffffffffu;
    if (catch_all) {
        if (has_service_name) {
            return false;
        }
    } else if (!has_service_name || filter->np_service_name != service_name) {
        return false;
    }
    for (const auto& p : filter->filter_params) {
        if (data_type == p.data_type.val) {
            return true;
        }
    }
    return false;
}

void EnqueuePushEvent(const PushEventInput& ev) {
    std::scoped_lock lk{g_push_mutex};
    g_push_queue.push_back(ev);
}

// Runs on the game thread from sceNpCheckCallback
void DrainPushEvents() {
    std::deque<PushEventInput> local;
    {
        std::scoped_lock lk{g_push_mutex};
        if (g_push_queue.empty()) {
            return;
        }
        local.swap(g_push_queue);
    }

    for (const PushEventInput& ev : local) {
        const bool ev_has_service = !ev.np_service_name.empty();
        OrbisNpWebApiPushEventDataType dt{};
        std::snprintf(dt.val, sizeof(dt.val), "%s", ev.data_type.c_str());
        const OrbisNpOnlineId* from_p = ev.has_from ? &ev.from_online_id : nullptr;
        const OrbisNpOnlineId* to_p = ev.has_to ? &ev.to_online_id : nullptr;

        std::scoped_lock gl{g_global_mutex};
        for (auto& [lib_id, context] : g_contexts) {
            if (context == nullptr) {
                continue;
            }
            std::scoped_lock cl{context->context_lock};
            for (auto& [uc_key, uc] : context->user_contexts) {
                if (uc == nullptr) {
                    continue;
                }
                if (uc->user_id != ev.target_user_id) {
                    continue;
                }
                const s32 title_user_ctx_id = uc_key;

                for (auto& [cb_id, cb] : uc->extendedPushEventCallbacks) {
                    if (cb == nullptr) {
                        continue;
                    }
                    void (*raw)() = cb->cb_func_a  ? reinterpret_cast<void (*)()>(cb->cb_func_a)
                                    : cb->cb_func ? reinterpret_cast<void (*)()>(cb->cb_func)
                                                 : nullptr;
                    if (raw == nullptr) {
                        continue;
                    }
                    auto fit = context->extended_push_event_filters.find(cb->filter_id);
                    if (fit == context->extended_push_event_filters.end()) {
                        continue;
                    }
                    const OrbisNpWebApiExtendedPushEventFilter* flt = fit->second;
                    if (!filterMatches(flt, ev.np_service_name, ev_has_service, ev.data_type)) {
                        continue;
                    }
                    std::vector<OrbisNpWebApiExtdPushEventExtdData> exarr;
                    exarr.reserve(ev.extd_data.size());
                    for (auto& [k, v] : ev.extd_data) {
                        OrbisNpWebApiExtdPushEventExtdData e{};
                        std::snprintf(e.extd_data_key.val, sizeof(e.extd_data_key.val), "%s",
                                      k.c_str());
                        e.data = const_cast<char*>(v.data());
                        e.data_len = v.size();
                        exarr.push_back(e);
                    }
                    const char* svc =
                        flt->np_service_name.empty() ? nullptr : flt->np_service_name.c_str();
                    const char* ext_data = ev.data.empty() ? nullptr : ev.data.data();
                    const OrbisNpWebApiExtdPushEventExtdData* ext_arr =
                        exarr.empty() ? nullptr : exarr.data();

                    LOG_INFO(
                        Lib_NpWebApi, "invoking extd cb ctx={:#x} cb_id={} data_type='{}'",
                        title_user_ctx_id, cb_id,
                        ev.data_type); // debug confirm the listener callback fires. to be removed
                    reinterpret_cast<ExtdCbA>(raw)(
                        title_user_ctx_id, cb_id, svc, flt->np_service_label, nullptr, to_p, nullptr,
                        from_p, &dt, ext_data, ev.data.size(), ext_arr, exarr.size(), cb->user_arg);
                }

                // Service push
                for (auto& [cbId, cb] : uc->servicePushEventCallbacks) {
                    if (cb == nullptr || cb->cbFunc == nullptr) {
                        continue;
                    }
                    auto fit = context->servicePushEventFilters.find(cb->filterId);
                    if (fit == context->servicePushEventFilters.end()) {
                        continue;
                    }
                    const OrbisNpWebApiServicePushEventFilter* flt = fit->second;
                    if (!filterMatches(flt, ev.npServiceName, ev_has_service, ev.dataType)) {
                        continue;
                    }
                    const char* svc =
                        flt->npServiceName.empty() ? nullptr : flt->npServiceName.c_str();
                    const char* svc_data = ev.data.empty() ? nullptr : ev.data.data();
                    PushPeerAddress to_peer{};   // notified user (self)
                    PushPeerAddress from_peer{}; // user that caused the event
                    if (ev.hasTo) {
                        to_peer.onlineId = ev.toOnlineId;
                    }
                    if (ev.hasFrom) {
                        from_peer.onlineId = ev.fromOnlineId;
                    }
                    reinterpret_cast<ServiceCb>(reinterpret_cast<void (*)()>(cb->cbFunc))(
                        title_user_ctx_id, cbId, svc, flt->npServiceLabel, &to_peer, &from_peer,
                        &dt, svc_data, ev.data.size(), cb->pUserArg);
                }

                // Basic push
                if (!ev_has_service) {
                    for (auto& [cbId, cb] : uc->pushEventCallbacks) {
                        if (cb == nullptr || cb->cbFunc == nullptr) {
                            continue;
                        }
                        auto fit = context->pushEventFilters.find(cb->filterId);
                        if (fit == context->pushEventFilters.end()) {
                            continue;
                        }
                        const OrbisNpWebApiPushEventFilter* flt = fit->second;
                        bool matched = false;
                        for (const auto& p : flt->filterParams) {
                            if (ev.dataType == p.dataType.val) {
                                matched = true;
                                break;
                            }
                        }
                        if (!matched) {
                            continue;
                        }
                        LOG_INFO(Lib_NpWebApi,
                                 "invoking basic cb ctx={:#x} cbId={} "
                                 "dataType='{}'",
                                 title_user_ctx_id, cbId,
                                 ev.dataType); // debug confirm the listener callback fires. to be
                                               // removed
                        PushPeerAddress to_peer{};   // notified user (self)
                        PushPeerAddress from_peer{}; // user that caused the event
                        if (ev.hasTo) {
                            to_peer.onlineId = ev.toOnlineId;
                        }
                        if (ev.hasFrom) {
                            from_peer.onlineId = ev.fromOnlineId;
                        }
                        const char* p_data = ev.data.empty() ? nullptr : ev.data.data();
                        reinterpret_cast<BasicCb>(reinterpret_cast<void (*)()>(cb->cbFunc))(
                            title_user_ctx_id, cbId, &to_peer, &from_peer, &dt, p_data,
                            ev.data.size(), cb->pUserArg);
                    }
                }
            }
        }
    }
}

} // namespace Libraries::Np::NpWebApi