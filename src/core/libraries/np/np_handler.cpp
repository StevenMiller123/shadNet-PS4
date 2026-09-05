// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <magic_enum/magic_enum.hpp>
#include "common/logging/log.h"
#include "np_handler.h"
#include "shadnet/client.h"
#include "shadnet/config.h"
#include "shadnet/server_probe.h"

namespace Libraries::Np {

NpHandler& NpHandler::GetInstance() {
    static NpHandler s_instance;
    return s_instance;
}

// Init procedures
void NpHandler::Initialize() {
    if (m_initialized.exchange(true)) {
        LOG_WARNING(NpHandler, "Initialize called more than once");
        return;
    }

    auto& config = ShadNet::Settings::GetInstance();
    if (!config.IsShadNetEnabled()) {
        LOG_NOTIFICATION(NpHandler, "shadNet is currently disabled");
        m_initialized.exchange(false);
        return;
    }

    // Probe shadNet accessibility
    static std::string server_url = config.GetServerUrl();
    static const u64 colon = server_url.rfind(':');
    if (colon == std::string::npos) {
        LOG_WARNING(NpHandler, "Invalid server url {}", server_url);
        m_initialized.exchange(false);
        return;
    }
    static std::string hostname = server_url.substr(0, colon);
    u16 port{};
    try {
        port = static_cast<u16>(std::stoi(server_url.substr(colon + 1)));
    } catch (const std::exception&) {
        LOG_WARNING(NpHandler, "Invalid server url {}", server_url);
        m_initialized.exchange(false);
        return;
    }

    const ShadNet::ProbeInfo probe = ShadNet::ProbeServer(hostname, port);
    if (probe.result != ShadNet::ProbeResult::Ok) {
        LOG_NOTIFICATION(NpHandler, "Failed to connect to shadNet server, error {}",
                         magic_enum::enum_name(probe.result));
        m_initialized.exchange(false);
    } else {
        LOG_NOTIFICATION(NpHandler, "shadNet server is accessible");
    }

    // Log in the current user.
    Connect(hostname, port, config.GetNpId(), config.GetPassword(), "");
}

bool NpHandler::Connect(const std::string& host, u16 port, const std::string& npid,
                        const std::string& password, const std::string& token) {
    LOG_INFO(NpHandler, "Connecting npid='{}' to {}:{} (timeout {}s)", npid, host, port,
             ShadNet::SHAD_CONNECT_TIMEOUT_MS / 1000);

    auto& config = ShadNet::Settings::GetInstance();
    s32 user_id = 1000;

    // Initialize per-user notification callbacks
    auto client = std::make_shared<ShadNet::ShadNetClient>();
    client->onFriendQuery = [this, user_id](const ShadNet::NotifyFriendQuery& n) {
        OnFriendQuery(user_id, n);
    };
    client->onFriendNew = [this, user_id](const ShadNet::NotifyFriendNew& n) {
        OnFriendNew(user_id, n);
    };
    client->onFriendLost = [this, user_id](const ShadNet::NotifyFriendLost& n) {
        OnFriendLost(user_id, n);
    };
    client->onFriendStatus = [this, user_id](const ShadNet::NotifyFriendStatus& n) {
        OnFriendStatus(user_id, n);
    };
    client->onLoginResult = [this, user_id](const ShadNet::LoginResult& res) {
        OnLoginResult(user_id, res);
    };

    /*
    client->onWebApiPushEvent = [this, user_id](const ShadNet::NotifyWebApiPushEvent& n) {
        OnWebApiPushEvent(user_id, n);
    };
    client->onAsyncReply = [this, user_id](ShadNet::CommandType cmd, u64 pkt_id,
                                           ShadNet::ErrorType err, const std::vector<u8>& body) {
        OnAsyncReply(user_id, cmd, pkt_id, err, body);
    };
    */

    // Set whether the user should appear as offline or online during this session.
    client->SetAppearOffline(config.IsAppearOfflineEnabled());
    client->Start(host, port, npid, password, token);

    // Handle incompatible protocol erros
    const auto handle_protocol_mismatch = [&client](ShadNet::ShadNetState st) {
        if (st != ShadNet::ShadNetState::FailureProtocol)
            return;
        const u32 server_ver = client->GetServerProtocolVersion();
        if (server_ver != 0) {
            LOG_NOTIFICATION(
                NpHandler,
                "shadNet protocol version mismatch (server v{}, emulator v{}); disabling "
                "shadNet for this run",
                server_ver, ShadNet::SHAD_PROTOCOL_VERSION);
        } else {
            LOG_NOTIFICATION(NpHandler,
                             "shadNet protocol error during handshake; disabling shadNet for "
                             "this run");
        }
    };

    const ShadNet::ShadNetState conn_state = client->WaitForConnection();
    if (conn_state != ShadNet::ShadNetState::Ok) {
        LOG_ERROR(NpHandler, "connection failed (state={})", magic_enum::enum_name(conn_state));
        handle_protocol_mismatch(conn_state);
        client->Stop();
        return false;
    }

    const ShadNet::ShadNetState auth_state = client->WaitForAuthenticated();
    if (auth_state != ShadNet::ShadNetState::Ok) {
        LOG_ERROR(NpHandler, "authentication failed (state={})", magic_enum::enum_name(auth_state));
        handle_protocol_mismatch(auth_state);
        client->Stop();
        return false;
    }

    LOG_NOTIFICATION(NpHandler, "{} successfully signed in to shadNet, accountId={}", npid,
                     client->GetUserId());

    // Net::UPnPClient::Instance().SetP2PFeaturesEnabled(client->IsMatching2Enabled());
    // if (client->IsMatching2Enabled() && config.IsUpnpEnabled()) {
    //     Net::UPnPClient::Instance().Start();
    // }

    // NpMatching2::SetMmShadNetClient(client, host, port);

    // Build OrbisNpId
    {
        OrbisNpId np_id{};
        const u64 id_len = std::min<u64>(npid.length(), 16);
        std::lock_guard lock(m_mutex_client);
        if (id_len > 0) {
            std::memcpy(np_id.handle.data, npid.data(), id_len);
        }
        m_np_id = np_id;
        m_client = std::move(client);
    }

    FireStateCallback(user_id, ORBIS_NP_STATE_SIGNED_IN);
    return true;
}

// State callbacks
s32 NpHandler::RegisterStateCallback(StateCallback cb, void* userdata) {
    std::lock_guard lock(m_mutex_cbs);
    const s32 h = m_next_handle++;
    m_state_cbs[h] = {h, std::move(cb), userdata};
    return h;
}

void NpHandler::UnregisterStateCallback(s32 handle) {
    std::lock_guard lock(m_mutex_cbs);
    m_state_cbs.erase(handle);
}

void NpHandler::FireStateCallback(s32 user_id, OrbisNpState state) {
    std::lock_guard lock(m_mutex_cbs);
    for (const auto& e : m_state_cbs) {
        if (e.second.cb)
            e.second.cb(user_id, state);
    }
}

// Friend callbacks
void NpHandler::OnFriendQuery(s32 user_id, const ShadNet::NotifyFriendQuery& n) {
    LOG_NOTIFICATION(NpHandler, "Friend request from {}", n.fromNpid);
    std::lock_guard lock(m_mutex_friend_state);
    auto& st = m_friend_state;
    if (std::find(st.requests_received.begin(), st.requests_received.end(), n.fromNpid) ==
        st.requests_received.end()) {
        st.requests_received.push_back(n.fromNpid);
    }
}

void NpHandler::OnFriendNew(s32 user_id, const ShadNet::NotifyFriendNew& n) {
    LOG_NOTIFICATION(NpHandler, "{} is now your friend", n.npid);
    std::lock_guard lock(m_mutex_friend_state);
    auto& st = m_friend_state;
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
    auto& st = m_friend_state;
    st.friends.erase(std::remove_if(st.friends.begin(), st.friends.end(),
                                    [&](const FriendInfo& f) { return f.npid == n.npid; }),
                     st.friends.end());
}

void NpHandler::OnFriendStatus(s32 user_id, const ShadNet::NotifyFriendStatus& n) {
    if (n.online) {
        LOG_NOTIFICATION(NpHandler, "{} is online", n.npid);
    }
    std::lock_guard lock(m_mutex_friend_state);
    auto& st = m_friend_state;
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
        m_friend_state = std::move(snap);
    }
    LOG_INFO(NpHandler, "{} friends, {} requests received, {} requests sent, {} blocked",
             res.friends.size(), res.requestsReceived.size(), res.requestsSent.size(),
             res.blocked.size());

    // Send notification for any pending requests
    if (!res.requestsReceived.empty()) {
        LOG_NOTIFICATION(NpHandler, "{} pending friend request(s)", res.requestsReceived.size());
    }
}

// WebApi Push Event
/*
void NpHandler::OnWebApiPushEvent(s32 user_id, const ShadNet::NotifyWebApiPushEvent& n) {
    LOG_INFO(NpHandler, "WebApiPushEvent svc='{}' type='{}' bytes={}", n.npServiceName, n.dataType,
             n.data.size());
    NpWebApi::PushEventInput ev;
    ev.targetUserId = user_id;
    ev.npServiceName = n.npServiceName;
    ev.npServiceLabel = n.npServiceLabel;
    ev.dataType = n.dataType;
    ev.data = n.data;
    if (!n.fromNpid.empty()) {
        ev.hasFrom = true;

        SetNpOnlineId(ev.fromOnlineId, n.fromNpid);
    }
    if (!n.toNpid.empty()) {
        ev.hasTo = true;
        SetNpOnlineId(ev.toOnlineId, n.toNpid);
    }
    ev.extdData = n.extdData; // extended-data (key,value) pairs -> dispatched as pExtdData
    NpWebApi::EnqueuePushEvent(ev);

    // Also surface a SESSION_INVITATION system-service event for titles that watch it instead of
    // (or in addition to) the WebAPI push callback
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
                auto& v = m_pending_invites;
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
*/

/*
void NpHandler::OnAsyncReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id,
                             ShadNet::ErrorType error, const std::vector<u8>& body) {
    const auto cmd_val = static_cast<u16>(cmd);
    if (cmd_val >= 100 && cmd_val <= 200) {
        NpMatching2::OnMatchingReply(cmd, pkt_id, error, body);
    } else if (cmd_val >= 201 && cmd_val <= 300) {
        OnTusReply(user_id, cmd, pkt_id, error, body);
    } else if (cmd_val >= 301 && cmd_val <= 400) {
        OnTrophyReply(user_id, cmd, pkt_id, error, body);
    } else if (cmd == ShadNet::CommandType::LookupOnlineId) {
        OnLookupReply(user_id, cmd, pkt_id, error, body);
    } else {
        OnScoreReply(user_id, cmd, pkt_id, error, body);
    }
}
*/

} // namespace Libraries::Np
