// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <magic_enum/magic_enum.hpp>
#include <orbis/UserService.h>

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_matching2/np_matching2_mm.h"
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
        return;
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
            continue;
        }

        ConnectUser(user_id, hostname, port, config.GetNpId(user_id), config.GetPassword(user_id));
    }

    if (m_clients.size() == 0) {
        LOG_NOTIFICATION(NpHandler, "No users connected to shadNet");
        m_initialized.exchange(false);
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
    client->onWebApiPushEvent = [this, user_id](const ShadNet::NotifyWebApiPushEvent& n) {
        OnWebApiPushEvent(user_id, n);
    };
    client->onServiceUnavailable = [this, user_id](const ShadNet::NotifyServiceUnavailable& n) {
        OnServiceUnavailable(user_id, n);
    };
    client->onClientRejected = [this, user_id](const ShadNet::NotifyClientRejected& n) {
        OnClientRejected(user_id, n);
    };
    client->onAsyncReply = [this, user_id](ShadNet::CommandType cmd, u64 pkt_id,
                                           ShadNet::ErrorType err, const std::vector<u8>& body) {
        OnAsyncReply(user_id, cmd, pkt_id, err, body);
    };
    client->onLoginResult = [this, user_id](const ShadNet::LoginResult& res) {
        OnLoginResult(user_id, res);
    };

    // Set whether the user should appear as offline or online during this session.
    client->SetAppearOffline(config.IsAppearOfflineEnabled(user_id));
    client->Start(host, port, npid, password, "");
    const ShadNet::ShadNetState conn_state = client->GetState();

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

    NpMatching2::SetMmShadNetClient(client, host, port);

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

    FireStateCallback(user_id, OrbisNpState::SignedIn);
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

    FailPendingRequests(user_id, ORBIS_NP_ERROR_SIGNED_OUT);
    FireStateCallback(user_id, OrbisNpState::SignedOut);
    LOG_NOTIFICATION(NpHandler, "user {} disconnected", user_id);
}

void NpHandler::FailPendingRequests(s32 user_id, s32 error_code) {
    u64 failed = 0;
    /* - libSceNpTus is not implemented yet
    {
        std::lock_guard lock(m_mutex_pending_tus);
        for (auto it = m_pending_tus.begin(); it != m_pending_tus.end();) {
            if (it->second.user_id == user_id) {
                if (it->second.req) {
                    it->second.req->SetResult(error_code);
                }
                it = m_pending_tus.erase(it);
                ++failed;
            } else {
                ++it;
            }
        }
    }
    */
    {
        std::lock_guard lock(m_mutex_pending_score);
        for (auto it = m_pending_score.begin(); it != m_pending_score.end();) {
            if (it->second.user_id == user_id) {
                if (it->second.req) {
                    it->second.req->SetResult(error_code);
                }
                it = m_pending_score.erase(it);
                ++failed;
            } else {
                ++it;
            }
        }
    }
    {
        std::lock_guard lock(m_mutex_pending_auth);
        for (auto it = m_pending_auth.begin(); it != m_pending_auth.end();) {
            if (it->second.user_id == user_id) {
                if (it->second.req) {
                    it->second.req->SetResult(error_code);
                }
                it = m_pending_auth.erase(it);
                ++failed;
            } else {
                ++it;
            }
        }
    }
    /* - libSceNpLookup is not implemented yet
    {
        std::lock_guard lock(m_mutex_pending_lookup);
        for (auto it = m_pending_lookup.begin(); it != m_pending_lookup.end();) {
            if (it->second.user_id == user_id) {
                if (it->second.on_result) {
                    it->second.on_result(error_code, 0, {});
                }
                it = m_pending_lookup.erase(it);
                ++failed;
            } else {
                ++it;
            }
        }
    }
    */
    if (failed > 0) {
        LOG_WARNING(NpHandler, "user_id={} failed {} pending request(s) with {:#x}", user_id,
                    failed, static_cast<u32>(error_code));
    }
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
                if (!client->IsConnected()) {
                    dropped.push_back(user_id);
                }
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

s32 NpHandler::GetUserIdByAccountId(u64 account_id) const {
    std::lock_guard lock(m_mutex_clients);
    for (auto& [uid, client] : m_clients) {
        if (static_cast<u64>(client->GetUserId()) == account_id) {
            return uid;
        }
    }
    return -1;
}

s32 NpHandler::GetUserIdByOnlineId(const OrbisNpOnlineId& online_id) const {
    std::lock_guard lock(m_mutex_clients);
    for (auto& [uid, np_id] : m_np_ids) {
        if (std::strncmp(np_id.handle.data, online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH) == 0) {
            return uid;
        }
    }
    return -1;
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

bool NpHandler::IsValidNpCommId(const std::string& id) {
    return !id.empty() && std::any_of(id.begin(), id.end(), [](char c) { return c != '\0'; });
}

std::string NpHandler::GetNpCommId(s32 service_label) const {
    // TODO complete guess of how commid is mapping to service_label.
    constexpr u64 COM_ID_LEN = 12;
    const auto& ids = Common::ElfInfo::Instance().GetNpCommIds();
    std::string com_id;
    if (!ids.empty()) {
        const u64 idx = (service_label >= 0 && static_cast<u64>(service_label) < ids.size())
                            ? static_cast<u64>(service_label)
                            : 0;
        com_id = ids[idx];
        if (static_cast<u64>(service_label) >= ids.size()) {
            LOG_WARNING(NpHandler, "unexpected service_label {}", service_label);
        }
    }
    if (com_id.size() < COM_ID_LEN) {
        com_id.resize(COM_ID_LEN, '\0');
    } else if (com_id.size() > COM_ID_LEN) {
        com_id.resize(COM_ID_LEN);
    }
    return com_id;
}

void NpHandler::OnAsyncReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id,
                             ShadNet::ErrorType error, const std::vector<u8>& body) {
    u16 cmd_val = static_cast<u16>(cmd);
    if (cmd_val >= 30 && cmd_val < 100) {
        OnScoreReply(user_id, cmd, pkt_id, error, body);
    } else if (cmd_val >= 100 && cmd_val < 201) {
        NpMatching2::OnMatching2Reply(cmd, pkt_id, error, body);
    } else if (cmd == ShadNet::CommandType::GetAuthorizationCode) {
        OnAuthReply(user_id, cmd, pkt_id, error, body);
    } else if (cmd != ShadNet::CommandType::SetClientVersion) {
        LOG_INFO(NpHandler, "Unsupported async command {} received", magic_enum::enum_name(cmd));
    }

    /*
    } else if (cmd_val >= 201 && cmd_val <= 300) {
        OnTusReply(user_id, cmd, pkt_id, error, body);
    } else if (cmd_val >= 301 && cmd_val <= 400) {
        OnTrophyReply(user_id, cmd, pkt_id, error, body);
    } else if (cmd == ShadNet::CommandType::LookupOnlineId) {
        OnLookupReply(user_id, cmd, pkt_id, error, body);
    }
    */
}

void NpHandler::OnClientRejected(s32 user_id, const ShadNet::NotifyClientRejected& n) {
    {
        std::lock_guard lock(m_mutex_clients);
        m_reconnects.erase(user_id);
        m_initialized.exchange(false);
    }
    const std::string text =
        n.message.empty() ? std::string("The shadNet server doesn't accept this shadPS4 build. "
                                        "Online features are off for this session.")
                          : n.message + " Online features are off for this session.";
    LOG_NOTIFICATION(NpHandler, "{}", text);
}

void NpHandler::OnServiceUnavailable(s32 user_id, const ShadNet::NotifyServiceUnavailable& n) {
    using N = ShadNet::NotifyServiceUnavailable;
    if (n.service != N::ServiceMatching2) {
        LOG_INFO(NpHandler, "user_id={} server refused service {} (reason {})", user_id, n.service,
                 n.reason);
        return;
    }
    std::string text;
    switch (n.reason) {
    case N::ReasonServerFull:
        text = n.limit > 0 ? fmt::format("Online matchmaking is full right now ({}/{} players). "
                                         "Try again in a few minutes.",
                                         n.inUse, n.limit)
                           : "Online matchmaking is full right now. Try again in a few minutes.";
        break;
    case N::ReasonNotAllowed:
        text = "Your shadNet account isn't allowed to use online matchmaking.";
        break;
    case N::ReasonDisabled:
        text = "Online matchmaking is turned off on this shadNet server.";
        break;
    default:
        text = "The shadNet server refused online matchmaking.";
        break;
    }
    LOG_NOTIFICATION(NpHandler, "matchmaking unavailable for {}: {}", user_id, text);
}

}; // namespace Libraries::Np