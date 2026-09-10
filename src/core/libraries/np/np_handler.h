// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <absl/container/flat_hash_map.h>
#include <orbis/NpManager.h>
#include "common/singleton.h"
#include "common/types.h"
#include "shadnet/client.h"

namespace Libraries::Np {

class NpHandler {
public:
    static NpHandler& Instance() {
        return *Common::Singleton<NpHandler>::Instance();
    }

    void Initialize();

    bool IsActive() {
        return m_initialized;
    };

    OrbisNpId& GetNpId(s32 user_id) {
        return m_np_ids[user_id];
    };

    std::string GetBearerToken(s32 user_id) const;

    // State callbacks
    using StateCallback = void (*)(s32 user_id, OrbisNpState state);
    s32 RegisterStateCallback(StateCallback cb, void* userdata);
    void UnregisterStateCallback(s32 handle);

private:
    bool ConnectUser(s32 user_id, const std::string& host, u16 port, const std::string& npid,
                     const std::string& password);

    void FireStateCallback(s32 user_id, OrbisNpState state);

    void OnFriendQuery(s32 user_id, const ShadNet::NotifyFriendQuery& n);
    void OnFriendNew(s32 user_id, const ShadNet::NotifyFriendNew& n);
    void OnFriendLost(s32 user_id, const ShadNet::NotifyFriendLost& n);
    void OnFriendStatus(s32 user_id, const ShadNet::NotifyFriendStatus& n);
    void OnWebApiPushEvent(s32 user_id, const ShadNet::NotifyWebApiPushEvent& n);
    // void OnAsyncReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id, ShadNet::ErrorType
    // error, const std::vector<u8>& body);
    void OnLoginResult(s32 user_id, const ShadNet::LoginResult& res);

    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_worker_running{false};

    // Client
    mutable std::mutex m_mutex_clients;
    absl::flat_hash_map<s32, std::shared_ptr<ShadNet::ShadNetClient>> m_clients;
    absl::flat_hash_map<s32, OrbisNpId> m_np_ids;

    // State callbacks
    struct CbEntry {
        s32 handle;
        StateCallback cb;
        void* userdata;
    };
    mutable std::mutex m_mutex_cbs;
    absl::flat_hash_map<s32, CbEntry> m_state_cbs;
    std::atomic<s32> m_next_handle;

    // Friend state
    struct FriendInfo {
        std::string npid;
        bool online = false;
    };
    struct FriendListSnapshot {
        std::vector<FriendInfo> friends;
        std::vector<std::string> requests_received;
        std::vector<std::string> requests_sent;
        std::vector<std::string> blocked;
    };
    mutable std::mutex m_mutex_friend_state;
    absl::flat_hash_map<s32, FriendListSnapshot> m_friend_states;

    // Invitations
    struct PendingInvitation {
        std::string session_id;
        std::string invitation_id;
        std::string from_npid;
        std::string to_npid;
        int64_t valid_until = 0;
    };
    mutable std::mutex m_mutex_pending_invites;
    absl::flat_hash_map<s32, std::vector<PendingInvitation>> m_pending_invites;
};

} // namespace Libraries::Np