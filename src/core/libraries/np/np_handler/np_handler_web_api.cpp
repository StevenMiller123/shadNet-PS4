// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include "common/logging/log.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_web_api/np_web_api.h"

namespace Libraries::Np {

void NpHandler::OnWebApiPushEvent(s32 user_id, const ShadNet::NotifyWebApiPushEvent& n) {
    LOG_INFO(NpHandler, "svc='{}' type='{}' bytes={}", n.np_service_name, n.data_type,
             n.data.size());
    NpWebApi::PushEventInput ev;
    ev.target_user_id = user_id;
    ev.np_service_name = n.np_service_name;
    ev.np_service_label = n.np_service_label;
    ev.data_type = n.data_type;
    ev.data = n.data;
    if (!n.from_npid.empty()) {
        ev.has_from = true;
        u64 copy_len = std::min<u64>(n.from_npid.length(), 16);
        std::memcpy(ev.from_online_id.data, n.from_npid.data(), copy_len);
    }
    if (!n.to_npid.empty()) {
        ev.has_to = true;
        u64 copy_len = std::min<u64>(n.to_npid.length(), 16);
        std::memcpy(ev.to_online_id.data, n.to_npid.data(), copy_len);
    }
    ev.extd_data = n.extd_data;
    ev.to_account_id = n.to_account_id != 0 ? n.to_account_id : GetAccountId(user_id);
    ev.from_account_id = n.from_account_id;
    if (ev.from_account_id == 0) {
        for (const auto& kv : n.extd_data) {
            if (kv.first == "fromAccountId") {
                ev.from_account_id = std::strtoull(kv.second.c_str(), nullptr, 10);
                break;
            }
        }
    }
    if (ev.to_account_id != 0) {
        ev.has_to = true;
        if (n.to_npid.empty()) {
            std::lock_guard lock(m_mutex_clients);
            if (const auto it = m_np_ids.find(user_id); it != m_np_ids.end()) {
                std::memcpy(ev.to_online_id.data, it->second.handle.data,
                            sizeof(ev.to_online_id.data));
            }
        }
    }
    NpWebApi::EnqueuePushEvent(ev);

    if (n.np_service_name == "sessionInvitation") {
        std::string session_id, invitation_id;
        s64 valid_until = 0;
        for (const auto& kv : n.extd_data) {
            if (kv.first == "sessionId") {
                session_id = kv.second;
            } else if (kv.first == "invitationId") {
                invitation_id = kv.second;
            } else if (kv.first == "validUntil") {
                valid_until = std::strtoll(kv.second.c_str(), nullptr, 10);
            }
        }
        if (!session_id.empty()) {
            {
                std::lock_guard lk(m_mutex_pending_invites);
                auto& v = m_pending_invites[user_id];
                v.erase(std::remove_if(v.begin(), v.end(),
                                       [&](const PendingInvitation& p) {
                                           return p.invitation_id == invitation_id;
                                       }),
                        v.end());
                v.push_back({session_id, invitation_id, n.from_npid, n.to_npid, valid_until});
            }
            // TODO: Rework this to be a popup message dialog or something
            // ImGui::InvitationPrompt::Push(user_id, invitation_id, session_id, n.from_npid);
        }
    }
}

} // namespace Libraries::Np
