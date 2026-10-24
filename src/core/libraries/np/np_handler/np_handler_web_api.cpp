// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include "common/logging/log.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_web_api/np_web_api.h"

namespace Libraries::Np {

void NpHandler::OnWebApiPushEvent(s32 user_id, const ShadNet::NotifyWebApiPushEvent& n) {
    LOG_INFO(NpHandler, "svc='{}' type='{}' bytes={}", n.npServiceName, n.dataType, n.data.size());
    NpWebApi::PushEventInput ev;
    ev.targetUserId = user_id;
    ev.npServiceName = n.npServiceName;
    ev.npServiceLabel = n.npServiceLabel;
    ev.dataType = n.dataType;
    ev.data = n.data;
    if (!n.fromNpid.empty()) {
        ev.hasFrom = true;
        u64 copy_len = std::min<u64>(n.fromNpid.length(), 16);
        std::memcpy(ev.fromOnlineId.data, n.fromNpid.data(), copy_len);
    }
    if (!n.toNpid.empty()) {
        ev.hasTo = true;
        u64 copy_len = std::min<u64>(n.toNpid.length(), 16);
        std::memcpy(ev.toOnlineId.data, n.toNpid.data(), copy_len);
    }
    ev.extdData = n.extdData;
    ev.toAccountId = n.toAccountId != 0 ? n.toAccountId : GetAccountId(user_id);
    ev.fromAccountId = n.fromAccountId;
    if (ev.fromAccountId == 0) {
        for (const auto& kv : n.extdData) {
            if (kv.first == "fromAccountId") {
                ev.fromAccountId = std::strtoull(kv.second.c_str(), nullptr, 10);
                break;
            }
        }
    }
    if (ev.toAccountId != 0) {
        ev.hasTo = true;
        if (n.toNpid.empty()) {
            std::lock_guard lock(m_mutex_clients);
            if (const auto it = m_np_ids.find(user_id); it != m_np_ids.end()) {
                std::memcpy(ev.toOnlineId.data, it->second.handle.data, sizeof(ev.toOnlineId.data));
            }
        }
    }
    NpWebApi::EnqueuePushEvent(ev);

    if (n.npServiceName == "sessionInvitation") {
        std::string session_id, invitation_id;
        int64_t valid_until = 0;
        for (const auto& kv : n.extdData) {
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
                v.push_back({session_id, invitation_id, n.fromNpid, n.toNpid, valid_until});
            }
            // TODO: Rework this to be a popup message dialog or something
            // ImGui::InvitationPrompt::Push(user_id, invitation_id, session_id, n.fromNpid);
        }
    }
}

} // namespace Libraries::Np
