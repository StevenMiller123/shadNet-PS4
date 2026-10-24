// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <magic_enum/magic_enum.hpp>
#include <orbis/UserService.h>
#include "common/logging/log.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_web_api/np_web_api.h"
#include "shadnet/client.h"
#include "shadnet/config.h"
#include "shadnet/server_probe.h"

namespace Libraries::Np {

void NpHandler::Initialize() {
    if (m_initialized.exchange(true)) {
        LOG_WARNING(NpHandler, "Initialize called more than once");
        return;
    }

    // Probe shadNet accessibility
    auto& config = ShadNet::Settings::GetInstance();
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
        LOG_INFO(NpHandler, "Failed to connect to shadNet server, error {}",
                 magic_enum::enum_name(probe.result));
        m_initialized.exchange(false);
    } else {
        LOG_INFO(NpHandler, "shadNet server is accessible");
    }

    // Log in any logged in users
    OrbisUserServiceLoginUserIdList user_list{};
    s32 result = sceUserServiceGetLoginUserIdList(&user_list);
    if (result != 0) {
        LOG_ERROR(Config, "Failed to retrieve logged in users: {:#x}", (u32)result);
        m_initialized.exchange(false);
        return;
    }

    for (s32 i = 0; i < ORBIS_USER_SERVICE_MAX_LOGIN_USERS; i++) {
        s32 user_id = user_list.userId[i];
        if (user_id == ORBIS_USER_SERVICE_USER_ID_INVALID) {
            break;
        }

        if (!config.IsShadNetEnabled(user_id)) {
            LOG_NOTIFICATION(NpHandler, "user {} has shadNet disabled", user_id);
            m_initialized.exchange(false);
            continue;
        }

        ConnectUser(user_id, hostname, port, config.GetNpId(user_id), config.GetPassword(user_id));
    }

    if (m_clients.size() == 0) {
        LOG_NOTIFICATION(NpHandler, "No users connected to shadNet");
        return;
    }

    StartWorker();
}

bool NpHandler::ConnectUserById(s32 user_id) {
    if (IsSignedIn(user_id)) {
        LOG_WARNING(NpHandler, "user {} is already signed in", user_id);
        return true;
    }

    auto& config = ShadNet::Settings::GetInstance();
    static std::string server_url = config.GetServerUrl();
    static const u64 colon = server_url.rfind(':');
    if (colon == std::string::npos) {
        LOG_WARNING(NpHandler, "Invalid server url {}", server_url);
        return false;
    }
    static std::string hostname = server_url.substr(0, colon);
    u16 port{};
    try {
        port = static_cast<u16>(std::stoi(server_url.substr(colon + 1)));
    } catch (const std::exception&) {
        LOG_WARNING(NpHandler, "Invalid server url {}", server_url);
        return false;
    }

    if (!config.IsShadNetEnabled(user_id)) {
        LOG_NOTIFICATION(NpHandler, "user {} has shadNet disabled", user_id);
        return false;
    }
    return ConnectUser(user_id, hostname, port, config.GetNpId(user_id),
                       config.GetPassword(user_id));
}

bool NpHandler::ConnectUser(s32 user_id, const std::string& host, u16 port, const std::string& npid,
                            const std::string& password) {
    LOG_INFO(NpHandler, "Connecting npid='{}' to {}:{} (timeout {}s)", npid, host, port,
             ShadNet::SHAD_CONNECT_TIMEOUT_MS / 1000);

    auto& config = ShadNet::Settings::GetInstance();

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
    client->onWebApiPushEvent = [this, user_id](const ShadNet::NotifyWebApiPushEvent& n) {
        OnWebApiPushEvent(user_id, n);
    };
    client->onAsyncReply = [this, user_id](ShadNet::CommandType cmd, u64 pkt_id,
                                           ShadNet::ErrorType err, const std::vector<u8>& body) {
        OnAsyncReply(user_id, cmd, pkt_id, err, body);
    };

    // Set whether the user should appear as offline or online during this session.
    client->SetAppearOffline(config.IsAppearOfflineEnabled(user_id));
    client->Start(host, port, npid, password, "");

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
        std::lock_guard lock(m_mutex_clients);
        if (id_len > 0) {
            std::memcpy(np_id.handle.data, npid.data(), id_len);
        }
        m_np_ids[user_id] = np_id;
        m_clients[user_id] = std::move(client);
    }

    FireStateCallback(user_id, ORBIS_NP_STATE_SIGNED_IN);
    return true;
}

void NpHandler::DisconnectUser(s32 user_id) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end())
            return;
        client = std::move(it->second);
        m_clients.erase(it);
    }
    {
        std::lock_guard lock(m_mutex_friend_state);
        m_friend_states.erase(user_id);
    }
    client->Stop();
    // Nothing that needs to be marked as failing yet. Update this later.
    // FailPendingRequests(user_id, ORBIS_NP_ERROR_SIGNED_OUT);
    FireStateCallback(user_id, ORBIS_NP_STATE_SIGNED_OUT);
    LOG_NOTIFICATION(NpHandler, "user {} disconnected", user_id);
}

void NpHandler::StartWorker() {
    bool expected = false;
    if (m_worker_running.compare_exchange_strong(expected, true)) {
        m_worker_thread = std::thread(&NpHandler::WorkerThread, this);
    }
}

void NpHandler::OnUserLoggedIn(s32 user_id) {
    if (ConnectUserById(user_id)) {
        StartWorker();
    }
}

void NpHandler::OnUserLoggedOut(s32 user_id) {
    {
        std::lock_guard lock(m_mutex_clients);
        m_reconnects.erase(user_id);
    }
    DisconnectUser(user_id);
}

void NpHandler::WorkerThread() {
    while (m_worker_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Collect ids of dropped clients
        std::vector<s32> dropped;
        {
            std::lock_guard lock(m_mutex_clients);
            for (auto& [user_id, client] : m_clients) {
                if (!client->IsConnected())
                    dropped.push_back(user_id);
            }
        }

        for (s32 user_id : dropped) {
            LOG_WARNING(NpHandler, "connection lost for user_id {}", user_id);
            DisconnectUser(user_id);
            MarkForReconnect(user_id);
        }
        TryReconnect();
    }
}

void NpHandler::MarkForReconnect(s32 user_id) {
    if (!IsActive()) {
        return;
    }
    std::lock_guard lock(m_mutex_clients);
    auto& st = m_reconnects[user_id];
    st.backoff = std::chrono::milliseconds(2000);
    st.next_attempt = std::chrono::steady_clock::now() + st.backoff;
}

void NpHandler::TryReconnect() {
    std::vector<s32> due;
    {
        std::lock_guard lock(m_mutex_clients);
        for (auto& [user_id, st] : m_reconnects) {
            if (m_clients.count(user_id) || std::chrono::steady_clock::now() >= st.next_attempt) {
                due.push_back(user_id);
            }
        }
    }

    for (s32 user_id : due) {
        if (!m_worker_running) {
            return;
        }
        if (!IsActive()) {
            std::lock_guard lock(m_mutex_clients);
            m_reconnects.erase(user_id);
            continue;
        }

        const bool ok = ConnectUserById(user_id);
        std::lock_guard lock(m_mutex_clients);
        if (ok || m_clients.count(user_id)) {
            m_reconnects.erase(user_id);
            LOG_INFO(NpHandler, "user_id {} reconnected to shadNet", user_id);
        } else if (auto it = m_reconnects.find(user_id); it != m_reconnects.end()) {
            it->second.backoff = std::min(it->second.backoff * 2, std::chrono::milliseconds(30000));
            it->second.next_attempt = std::chrono::steady_clock::now() + it->second.backoff;
            LOG_DEBUG(NpHandler, "failed to reconnect user_id {}, retrying in {}ms", user_id,
                      it->second.backoff.count());
        }
    }
}

std::string NpHandler::GetBearerToken(s32 user_id) const {
    std::lock_guard lock(m_mutex_clients);
    auto it = m_clients.find(user_id);
    return it != m_clients.end() ? it->second->GetBearerToken() : std::string{};
}

u64 NpHandler::GetAccountId(s32 user_id) const {
    std::lock_guard lock(m_mutex_clients);
    auto it = m_clients.find(user_id);
    return it != m_clients.end() ? static_cast<u64>(it->second->GetUserId()) : 0;
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
        if (e.second.cb) {
            e.second.cb(user_id, state);
        }
    }
}

void NpHandler::OnAsyncReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id,
                             ShadNet::ErrorType error, const std::vector<u8>& body) {
    const auto cmd_val = static_cast<u16>(cmd);
    LOG_INFO(NpHandler, "Unsupported async command {} received", cmd_val);
    /*
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
    */
}

}; // namespace Libraries::Np