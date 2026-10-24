// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include "common/logging/log.h"
#include "core/libraries/np/np_handler/np_handler.h"

namespace Libraries::Np {

void NpHandler::OnFriendQuery(s32 user_id, const ShadNet::NotifyFriendQuery& n) {
    LOG_NOTIFICATION(NpHandler, "Friend request from {}", n.fromNpid);
    std::lock_guard lock(m_mutex_friend_state);
    auto& st = m_friend_states[user_id];
    if (std::find(st.requests_received.begin(), st.requests_received.end(), n.fromNpid) ==
        st.requests_received.end()) {
        st.requests_received.push_back(n.fromNpid);
    }
}

void NpHandler::OnFriendNew(s32 user_id, const ShadNet::NotifyFriendNew& n) {
    LOG_NOTIFICATION(NpHandler, "{} is now your friend", n.npid);
    std::lock_guard lock(m_mutex_friend_state);
    auto& st = m_friend_states[user_id];
    auto it = std::find_if(st.friends.begin(), st.friends.end(),
                           [&](const FriendInfo& f) { return f.npid == n.npid; });
    if (it == st.friends.end()) {
        st.friends.push_back({n.npid, n.online});
    } else {
        it->online = n.online;
    }
    const auto drop = [&](std::vector<std::string>& v) {
        v.erase(std::remove(v.begin(), v.end(), n.npid), v.end());
    };
    drop(st.requests_received);
    drop(st.requests_sent);
}

void NpHandler::OnFriendLost(s32 user_id, const ShadNet::NotifyFriendLost& n) {
    LOG_NOTIFICATION(NpHandler, "{} removed you as a friend", n.npid);
    std::lock_guard lock(m_mutex_friend_state);
    auto& st = m_friend_states[user_id];
    st.friends.erase(std::remove_if(st.friends.begin(), st.friends.end(),
                                    [&](const FriendInfo& f) { return f.npid == n.npid; }),
                     st.friends.end());
}

void NpHandler::OnFriendStatus(s32 user_id, const ShadNet::NotifyFriendStatus& n) {
    if (n.online) {
        LOG_NOTIFICATION(NpHandler, "{} is online", n.npid);
    }
    std::lock_guard lock(m_mutex_friend_state);
    auto& st = m_friend_states[user_id];
    auto it = std::find_if(st.friends.begin(), st.friends.end(),
                           [&](const FriendInfo& f) { return f.npid == n.npid; });
    if (it != st.friends.end()) {
        it->online = n.online;
    } else {
        st.friends.push_back({n.npid, n.online});
    }
}

void NpHandler::OnLoginResult(s32 user_id, const ShadNet::LoginResult& res) {
    if (res.error != ShadNet::ErrorType::NoError) {
        return;
    }
    FriendListSnapshot snap;
    snap.friends.reserve(res.friends.size());
    for (const auto& f : res.friends) {
        snap.friends.push_back({f.npid, f.online});
    }
    snap.requests_sent = res.requestsSent;
    snap.requests_received = res.requestsReceived;
    snap.blocked = res.blocked;
    {
        std::lock_guard lock(m_mutex_friend_state);
        m_friend_states[user_id] = std::move(snap);
    }
    LOG_INFO(NpHandler, "{} friends, {} requests received, {} requests sent, {} blocked",
             res.friends.size(), res.requestsReceived.size(), res.requestsSent.size(),
             res.blocked.size());

    // Send notification for any pending requests
    if (!res.requestsReceived.empty()) {
        LOG_NOTIFICATION(NpHandler, "{} pending friend request(s)", res.requestsReceived.size());
    }
}

}; // namespace Libraries::Np