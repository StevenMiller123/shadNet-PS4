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
        LOG_WARNING("Initialize called more than once");
        return;
    }

    auto& config = ShadNet::Settings::GetInstance();
    if (!config.IsShadNetEnabled()) {
        LOG_NOTIFICATION("shadNet is currently disabled");
        m_initialized.exchange(false);
        return;
    }

    // Probe shadNet accessibility
    static std::string server_url = config.GetServerUrl();
    static const u64 colon = server_url.rfind(':');
    if (colon == std::string::npos) {
        LOG_WARNING("Invalid server url {}", server_url);
        m_initialized.exchange(false);
        return;
    }
    static std::string hostname = server_url.substr(0, colon);
    u16 port{};
    try {
        port = static_cast<u16>(std::stoi(server_url.substr(colon + 1)));
    } catch (const std::exception&) {
        LOG_WARNING("Invalid server url {}", server_url);
        m_initialized.exchange(false);
        return;
    }

    const ShadNet::ProbeInfo probe = ShadNet::ProbeServer(hostname, port);
    if (probe.result != ShadNet::ProbeResult::Ok) {
        LOG_NOTIFICATION("Failed to connect to shadNet server, error {}",
                         magic_enum::enum_name(probe.result));
        m_initialized.exchange(false);
    } else {
        LOG_NOTIFICATION("shadNet server is accessible");
    }

    // Log in the current user.
    Connect(hostname, port, config.GetNpId(), config.GetPassword(), "");
}

bool NpHandler::Connect(const std::string& host, u16 port, const std::string& npid,
                        const std::string& password, const std::string& token) {
    LOG_INFO("Connecting npid='{}' to {}:{} (timeout {}s)", npid, host, port,
             ShadNet::SHAD_CONNECT_TIMEOUT_MS / 1000);

    auto& config = ShadNet::Settings::GetInstance();
    s32 user_id = 1000;

    // Initialize per-user notification callbacks
    auto client = std::make_shared<ShadNet::ShadNetClient>();
    /*
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
    client->onAsyncReply = [this, user_id](ShadNet::CommandType cmd, u64 pkt_id,
                                           ShadNet::ErrorType err, const std::vector<u8>& body) {
        OnAsyncReply(user_id, cmd, pkt_id, err, body);
    };
    client->onLoginResult = [this, user_id](const ShadNet::LoginResult& res) {
        OnLoginResult(user_id, res);
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
                "shadNet protocol version mismatch (server v{}, emulator v{}); disabling "
                "shadNet for this run",
                server_ver, ShadNet::SHAD_PROTOCOL_VERSION);
        } else {
            LOG_NOTIFICATION("shadNet protocol error during handshake; disabling shadNet for "
                             "this run");
        }
    };

    const ShadNet::ShadNetState conn_state = client->WaitForConnection();
    if (conn_state != ShadNet::ShadNetState::Ok) {
        LOG_ERROR("connection failed (state={})", magic_enum::enum_name(conn_state));
        handle_protocol_mismatch(conn_state);
        client->Stop();
        return false;
    }

    const ShadNet::ShadNetState auth_state = client->WaitForAuthenticated();
    if (auth_state != ShadNet::ShadNetState::Ok) {
        LOG_ERROR("authentication failed (state={})", magic_enum::enum_name(auth_state));
        handle_protocol_mismatch(auth_state);
        client->Stop();
        return false;
    }

    LOG_INFO("signed in npid='{}' accountId={}", npid, client->GetUserId());

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

} // namespace Libraries::Np
