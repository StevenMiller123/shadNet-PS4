// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <chrono>
#include <thread>
#include <magic_enum/magic_enum.hpp>
#include <orbis/Net.h>

#include "client.h"
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/thread.h"
#include "shadnet.pb.h"

namespace ShadNet {

// Build a u32-LE-prefixed proto blob payload for a request packet.
template <typename T>
static std::vector<u8> MakeProtoPayload(const T& msg) {
    const std::string serialised = msg.SerializeAsString();
    const u32 len = static_cast<u32>(serialised.size());
    std::vector<u8> out(4);
    out[0] = static_cast<u8>(len);
    out[1] = static_cast<u8>(len >> 8);
    out[2] = static_cast<u8>(len >> 16);
    out[3] = static_cast<u8>(len >> 24);
    out.insert(out.end(), serialised.begin(), serialised.end());
    return out;
}

// Read a u32-LE-prefixed proto blob starting at pos in p.
// Returns the raw bytes ready for ParseFromString.
std::string ShadNetClient::ExtractBlob(const std::vector<u8>& p, int pos) {
    if (pos + 4 > static_cast<int>(p.size()))
        return {};
    const u32 len = GetLE32(p.data() + pos);
    pos += 4;
    if (pos + static_cast<int>(len) > static_cast<int>(p.size()))
        return {};
    return std::string(reinterpret_cast<const char*>(p.data() + pos), len);
}

ShadNetClient::ShadNetClient() {
    if (!sem_init(&m_sem_authenticated, 0, 1) || !sem_init(&m_sem_connected, 0, 1)) {
        LOG_WARNING("Failed to init semaphores!");
    }
}

ShadNetClient::~ShadNetClient() {
    Stop();
}

void ShadNetClient::Start(const std::string& host, u16 port, const std::string& npid,
                          const std::string& password, const std::string& token) {
    m_host = host;
    m_port = port;
    m_npid = npid;
    m_password = password;
    m_token = token;
    m_terminate = false;
    m_thread_connect = std::thread(&ShadNetClient::ConnectThread, this);
}

void ShadNetClient::Stop() {
    m_terminate = true;
    try {
        sem_post(&m_sem_connected);
    } catch (...) {
    }
    try {
        sem_post(&m_sem_authenticated);
    } catch (...) {
    }
    {
        std::lock_guard lock(m_mutex_send_queue);
        m_cv_send_queue.notify_all();
    }
    DoDisconnect();
    if (m_thread_connect.joinable())
        m_thread_connect.join();
    if (m_thread_reader.joinable())
        m_thread_reader.join();
    if (m_thread_writer.joinable())
        m_thread_writer.join();
}

ShadNetState ShadNetClient::WaitForConnection() {
    {
        std::lock_guard lock(m_mutex_connected);
        if (m_connected)
            return ShadNetState::Ok;
    }
    sem_wait(&m_sem_connected);
    return m_connected ? ShadNetState::Ok : m_state.load();
}

ShadNetState ShadNetClient::WaitForAuthenticated() {
    {
        std::lock_guard lock(m_mutex_authenticated);
        if (m_authenticated)
            return ShadNetState::Ok;
    }
    sem_wait(&m_sem_authenticated);
    return m_authenticated ? ShadNetState::Ok : m_state.load();
}

bool ShadNetClient::IsConnected() const {
    return m_connected.load();
}
bool ShadNetClient::IsAuthenticated() const {
    return m_authenticated.load();
}
ShadNetState ShadNetClient::GetState() const {
    return m_state.load();
}
const std::string& ShadNetClient::GetAvatarUrl() const {
    return m_avatar_url;
}
u64 ShadNetClient::GetUserId() const {
    return m_user_id;
}
const std::string& ShadNetClient::GetNpid() const {
    return m_npid;
}
u32 ShadNetClient::GetAddrLocal() const {
    return m_addr_local.load();
}
u32 ShadNetClient::GetAddrServer() const {
    return m_addr_server.load();
}
bool ShadNetClient::IsMatching2Enabled() const {
    return m_matching2_enabled.load();
}

u32 ShadNetClient::GetNumFriends() const {
    std::lock_guard lock(m_mutex_friends);
    return static_cast<u32>(m_friends.size());
}

std::optional<std::string> ShadNetClient::GetFriendNpid(u32 index) const {
    std::lock_guard lock(m_mutex_friends);
    if (index >= m_friends.size())
        return std::nullopt;
    return m_friends[index].npid;
}

std::string ShadNetClient::GetBearerToken() const {
    std::lock_guard lock(m_mutex_bearer);
    return m_bearer_token;
}

// Threading
void ShadNetClient::ConnectThread() {
    Common::SetCurrentThreadName("ShadNet:Connect");
    bool connected = false;
    u32 backoff_ms = SHAD_CONNECT_RETRY_BACKOFF_MS;
    for (u32 attempt = 1; attempt <= SHAD_CONNECT_MAX_ATTEMPTS && !m_terminate; ++attempt) {
        if (DoConnect()) {
            connected = true;
            break;
        }
        if (m_state == ShadNetState::FailureProtocol)
            break;
        if (m_terminate || attempt == SHAD_CONNECT_MAX_ATTEMPTS)
            break;
        LOG_WARNING("connect attempt {}/{} to {}:{} failed, retrying in {} ms", attempt,
                    SHAD_CONNECT_MAX_ATTEMPTS, m_host, m_port, backoff_ms);
        // Interruptible backoff: poll m_terminate so Stop() wakes us promptly.
        for (u32 waited = 0; waited < backoff_ms && !m_terminate; waited += 100)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        backoff_ms = std::min<u32>(backoff_ms * 2, 8000);
    }

    if (!connected) {
        // m_state holds the last failure reason from DoConnect.
        sem_post(&m_sem_connected);
        sem_post(&m_sem_authenticated);
        return;
    }

    m_thread_reader = std::thread(&ShadNetClient::ReaderThread, this);
    m_thread_writer = std::thread(&ShadNetClient::WriterThread, this);
    sem_post(&m_sem_connected);

    // Build Login request as protobuf
    shadnet::LoginRequest req;
    req.set_npid(m_npid);
    req.set_password(m_password);
    if (!m_token.empty())
        req.set_token(m_token);
    req.set_title_id(std::string(Common::ElfInfo::Instance().GameSerial()));
    req.set_title_name(std::string(Common::ElfInfo::Instance().Title()));
    // Appear-Offline preference. shadNet handles us as offline for everyone else while set.
    req.set_appear_offline(m_appear_offline);

    const u64 id = m_pkt_counter.fetch_add(1);
    if (!SendAll(BuildPacket(CommandType::Login, id, MakeProtoPayload(req)))) {
        LOG_ERROR("Failed to send login packet");
        m_state = ShadNetState::FailureOther;
        return;
    }
    LOG_INFO("Login packet sent for '{}'", m_npid);
}

void ShadNetClient::ReaderThread() {
    Common::SetCurrentThreadName("ShadNet:Reader");
    while (!m_terminate) {
        u8 hdr[SHAD_HEADER_SIZE];
        if (!RecvN(hdr, SHAD_HEADER_SIZE)) {
            if (!m_terminate)
                LOG_WARNING("header recv failed, disconnecting");
            break;
        }
        const auto ptype = static_cast<PacketType>(hdr[0]);
        const u16 cmd_raw = GetLE16(hdr + 1);
        const u32 total_sz = GetLE32(hdr + 3);
        const u64 pkt_id = GetLE64(hdr + 7);

        if (total_sz < SHAD_HEADER_SIZE || total_sz > SHAD_MAX_PACKET_SIZE) {
            LOG_ERROR("Corrupt packet (total_sz={})", total_sz);
            m_state = ShadNetState::FailureProtocol;
            break;
        }
        std::vector<u8> payload;
        const u32 payload_sz = total_sz - static_cast<u32>(SHAD_HEADER_SIZE);
        if (payload_sz > 0) {
            payload.resize(payload_sz);
            if (!RecvN(payload.data(), payload_sz)) {
                if (!m_terminate)
                    LOG_WARNING("payload recv failed");
                break;
            }
        }
        DispatchPacket(ptype, cmd_raw, pkt_id, payload);
    }

    if (!m_authenticated) {
        if (m_state == ShadNetState::Ok)
            m_state = ShadNetState::FailureOther;
        sem_post(&m_sem_authenticated);
    }
    m_connected = false;
    m_authenticated = false;
    LOG_INFO("exiting");
}

void ShadNetClient::WriterThread() {
    Common::SetCurrentThreadName("ShadNet:Writer");
    while (!m_terminate) {
        std::unique_lock lock(m_mutex_send_queue);
        m_cv_send_queue.wait(lock, [&] { return m_terminate.load() || !m_send_queue.empty(); });
        if (m_terminate)
            break;
        std::vector<std::vector<u8>> batch;
        std::swap(batch, m_send_queue);
        lock.unlock();
        for (auto& pkt : batch) {
            if (!m_connected)
                break;
            if (!SendAll(pkt)) {
                LOG_ERROR("send failed");
                return;
            }
        }
    }
}

// Connect / Disconnect

bool ShadNetClient::DoConnect() {
    m_state = ShadNetState::Ok; // reset; this attempt sets a failure code only on error

    s32 netpool_id = sceNetPoolCreate("shadNet Pool", 0x1000, 0);
    if (netpool_id < 0) {
        LOG_WARNING("Failed to create net pool, error = {:#x}", static_cast<u32>(netpool_id));
        m_state = ShadNetState::FailureConnect;
        return false;
    }

    s32 resolver = sceNetResolverCreate("shadNet Resolver", netpool_id, 0);
    if (resolver < 0) {
        LOG_WARNING("Failed to create resolver, error = {:#x}", static_cast<u32>(resolver));
        m_state = ShadNetState::FailureConnect;
        return false;
    }

    OrbisNetInAddr shadnet_addr{};
    s32 result = sceNetResolverStartNtoa(resolver, m_host.c_str(), &shadnet_addr, 0, 0, 0);
    if (result != 0) {
        LOG_WARNING("DNS resolution failed for '{}', error = {:#x}", m_host.c_str(),
                    static_cast<u32>(result));
        m_state = ShadNetState::FailureConnect;
        return false;
    }

    result = sceNetResolverDestroy(resolver);
    if (result != 0) {
        LOG_WARNING("Failed to destroy resolver, error = {:#x}", static_cast<u32>(result));
    }

    result = sceNetPoolDestroy(netpool_id);
    if (result != 0) {
        LOG_WARNING("Failed to destroy net pool, error = {:#x}", static_cast<u32>(result));
    }

    m_sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sock == SHAD_INVALID_SOCK) {
        m_state = ShadNetState::FailureConnect;
        return false;
    }

    s32 optval = 1;
    result = ::setsockopt(m_sock, SOL_SOCKET, SO_NBIO, reinterpret_cast<void*>(&optval),
                          static_cast<socklen_t>(sizeof(optval)));
    if (result != 0) {
        LOG_WARNING("Failed to mark socket as non-blocking");
        m_state = ShadNetState::FailureConnect;
        return false;
    }

    OrbisNetSockaddrIn addr{};
    addr.sin_len = sizeof(OrbisNetSockaddrIn);
    addr.sin_family = AF_INET;
    addr.sin_addr = shadnet_addr.s_addr;
    addr.sin_port = htons(m_port);
    const int cr =
        ::connect(m_sock, reinterpret_cast<sockaddr*>(&addr), static_cast<socklen_t>(addr.sin_len));

    bool connected = false;
    const bool in_progress = (cr < 0 && errno == EINPROGRESS);
    if (cr == 0 || in_progress) {
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(m_sock, &wfds);
        struct timeval tv{static_cast<long>(SHAD_CONNECT_TIMEOUT_MS / 1000),
                          static_cast<long>((SHAD_CONNECT_TIMEOUT_MS % 1000) * 1000)};
        if (::select(static_cast<int>(m_sock) + 1, nullptr, &wfds, nullptr, &tv) > 0) {
            int err = 0;
            socklen_t len = sizeof(err);
            ::getsockopt(m_sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&err), &len);
            connected = (err == 0);
        }
    }
    if (!connected) {
        LOG_ERROR("connect() timed out or failed for {}:{}", m_host, m_port);
        SHAD_CLOSE(m_sock);
        m_sock = SHAD_INVALID_SOCK;
        m_state = ShadNetState::FailureConnect;
        return false;
    }

    // Restore blocking mode for RecvN / SendAll
    optval = 0;
    result = ::setsockopt(m_sock, SOL_SOCKET, SO_NBIO, reinterpret_cast<void*>(&optval),
                          static_cast<socklen_t>(sizeof(optval)));
    if (result != 0) {
        LOG_WARNING("Failed to mark socket as blocking");
    }

    struct sockaddr_in local{};
    socklen_t alen = sizeof(local);
    if (::getsockname(m_sock, reinterpret_cast<struct sockaddr*>(&local), &alen) == 0)
        m_addr_local.store(local.sin_addr.s_addr);

    struct sockaddr_in peer{};
    socklen_t plen = sizeof(peer);
    if (::getpeername(m_sock, reinterpret_cast<struct sockaddr*>(&peer), &plen) == 0)
        m_addr_server.store(peer.sin_addr.s_addr);

    LOG_INFO("TCP connected to {}:{}", m_host, m_port);

    // Apply receive timeout for the ServerInfo handshake.
    // Cleared after success so ReaderThread's RecvN blocks indefinitely as intended.
    struct timeval so_rcv{static_cast<long>(SHAD_CONNECT_TIMEOUT_MS / 1000),
                          static_cast<long>((SHAD_CONNECT_TIMEOUT_MS % 1000) * 1000)};
    ::setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&so_rcv),
                 sizeof(so_rcv));

    // ServerInfo handshake
    u8 hdr[SHAD_HEADER_SIZE];
    if (!RecvN(hdr, SHAD_HEADER_SIZE)) {
        LOG_ERROR("Timeout reading ServerInfo header");
        DoDisconnect();
        m_state = ShadNetState::FailureServerInfo;
        return false;
    }
    if (static_cast<PacketType>(hdr[0]) != PacketType::ServerInfo) {
        LOG_ERROR("Expected ServerInfo, got packet type {:02x}", hdr[0]);
        DoDisconnect();
        m_state = ShadNetState::FailureServerInfo;
        return false;
    }
    const u32 total_sz = GetLE32(hdr + 3);
    const u32 payload_sz = (total_sz > SHAD_HEADER_SIZE) ? total_sz - SHAD_HEADER_SIZE : 0;
    std::vector<u8> si_payload(payload_sz);
    if (payload_sz > 0 && !RecvN(si_payload.data(), payload_sz)) {
        LOG_ERROR("Timeout reading ServerInfo payload");
        DoDisconnect();
        m_state = ShadNetState::FailureServerInfo;
        return false;
    }
    if (payload_sz >= 4) {
        const u32 server_ver = GetLE32(si_payload.data());
        m_server_protocol_version.store(server_ver);
        if (server_ver != SHAD_PROTOCOL_VERSION) {
            LOG_ERROR("Protocol version mismatch server={} client={}", server_ver,
                      SHAD_PROTOCOL_VERSION);
            DoDisconnect();
            m_state = ShadNetState::FailureProtocol;
            return false;
        }
    }
    LOG_INFO("ServerInfo OK (protocol v{})", SHAD_PROTOCOL_VERSION);

    // Clear the receive timeout ReaderThread handles the socket from here.
    struct timeval no_timeout{0, 0};
    ::setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&no_timeout),
                 sizeof(no_timeout));

    m_connected = true;
    return true;
}

void ShadNetClient::DoDisconnect() {
    if (m_sock != SHAD_INVALID_SOCK) {
        ::shutdown(m_sock, SHUT_RDWR);
        SHAD_CLOSE(m_sock);
        m_sock = SHAD_INVALID_SOCK;
    }
}

bool ShadNetClient::RecvN(u8* buf, u32 n) {
    u32 received = 0;
    while (received < n) {
        if (m_terminate)
            return false;
        const int r = static_cast<int>(::recv(m_sock, reinterpret_cast<char*>(buf + received),
                                              static_cast<int>(n - received), 0));
        if (r <= 0)
            return false;
        received += static_cast<u32>(r);
    }
    return true;
}

bool ShadNetClient::SendAll(const std::vector<u8>& data) {
    std::lock_guard lock(m_mutex_send_direct);
    int sent = 0;
    const int total = static_cast<int>(data.size());
    while (sent < total) {
        const int r = static_cast<int>(
            ::send(m_sock, reinterpret_cast<const char*>(data.data() + sent), total - sent, 0));
        if (r < 0) {
            LOG_ERROR("send() failed");
            return false;
        }
        sent += r;
    }
    return true;
}

std::vector<u8> ShadNetClient::BuildPacket(CommandType cmd, u64 id,
                                           const std::vector<u8>& payload) const {
    const u32 total = static_cast<u32>(SHAD_HEADER_SIZE + payload.size());
    std::vector<u8> out(SHAD_HEADER_SIZE);
    out[0] = static_cast<u8>(PacketType::Request);
    PutLE16(out, 1, static_cast<u16>(cmd));
    PutLE32(out, 3, total);
    PutLE64(out, 7, id);
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

u64 ShadNetClient::SubmitRequest(CommandType cmd, const std::vector<u8>& payload) {
    const u64 pkt_id = m_pkt_counter.fetch_add(1);
    auto pkt = BuildPacket(cmd, pkt_id, payload);
    {
        std::lock_guard lock(m_mutex_send_queue);
        m_send_queue.push_back(std::move(pkt));
    }
    m_cv_send_queue.notify_all();
    return pkt_id;
}

u64 ShadNetClient::AddFriend(const std::string& npid) {
    shadnet::FriendCommandRequest req;
    req.set_npid(npid);
    return SubmitRequest(CommandType::AddFriend, MakeProtoPayload(req));
}

u64 ShadNetClient::RemoveFriend(const std::string& npid) {
    shadnet::FriendCommandRequest req;
    req.set_npid(npid);
    return SubmitRequest(CommandType::RemoveFriend, MakeProtoPayload(req));
}

u64 ShadNetClient::AddBlock(const std::string& npid) {
    shadnet::FriendCommandRequest req;
    req.set_npid(npid);
    return SubmitRequest(CommandType::AddBlock, MakeProtoPayload(req));
}

u64 ShadNetClient::RemoveBlock(const std::string& npid) {
    shadnet::FriendCommandRequest req;
    req.set_npid(npid);
    return SubmitRequest(CommandType::RemoveBlock, MakeProtoPayload(req));
}

u64 ShadNetClient::SetAppearOffline(bool enable) {
    m_appear_offline = enable; // cache so the (re)login packet carries the current state
    if (!IsAuthenticated())
        return 0; // not connected yet -> login will carry the cached value
    shadnet::SetAppearOfflineRequest req;
    req.set_appear_offline(enable);
    return SubmitRequest(CommandType::SetAppearOffline, MakeProtoPayload(req));
}

bool ShadNetClient::RequestServerFeatures() {
    const u64 pkt_id = m_pkt_counter.fetch_add(1);
    std::vector<u8> empty_payload;
    std::vector<u8> pkt = BuildPacket(CommandType::GetServerFeatures, pkt_id, empty_payload);
    {
        std::lock_guard lock(m_mutex_send_queue);
        m_send_queue.push_back(std::move(pkt));
    }
    m_cv_send_queue.notify_one();
    LOG_DEBUG("request fired pkt_id={}", pkt_id);
    return true;
}

// Packet dispatch

void ShadNetClient::DispatchPacket(PacketType type, u16 cmd_raw, u64 pkt_id,
                                   const std::vector<u8>& payload) {
    switch (type) {
    case PacketType::Reply:
        switch (static_cast<CommandType>(cmd_raw)) {
        case CommandType::Login:
            HandleLoginReply(payload);
            break;
        case CommandType::GetToken:
            HandleGetTokenReply(payload);
            break;
        case CommandType::GetServerFeatures:
            HandleServerFeaturesReply(payload);
            break;
        default:
            if (onAsyncReply) {
                // Every reply body starts with an ErrorType byte.
                ErrorType err =
                    payload.empty() ? ErrorType::Malformed : static_cast<ErrorType>(payload[0]);
                std::vector<u8> body;
                if (payload.size() > 1) {
                    body.assign(payload.begin() + 1, payload.end());
                }
                onAsyncReply(static_cast<CommandType>(cmd_raw), pkt_id, err, body);
            } else {
                LOG_DEBUG("Unhandled reply cmd={} pkt_id={}", cmd_raw, pkt_id);
            }
            break;
        }
        break;
    case PacketType::Notification:
        HandleNotification(cmd_raw, payload);
        break;
    case PacketType::ServerInfo:
        LOG_DEBUG("ServerInfo update received");
        break;
    case PacketType::Request:
        LOG_WARNING("Unexpected Request from server");
        break;
    }
}

// Login reply

void ShadNetClient::HandleLoginReply(const std::vector<u8>& payload) {
    LoginResult res;

    if (payload.empty()) {
        res.error = ErrorType::Malformed;
        LOG_ERROR("Empty payload");
    } else {
        res.error = static_cast<ErrorType>(payload[0]);

        if (res.error == ErrorType::NoError) {
            // payload[0]   = ErrorType byte
            // payload[1..] = u32 LE blob size + LoginReply proto bytes
            shadnet::LoginReply pb;
            const std::string blob = ExtractBlob(payload, 1);
            if (!blob.empty() && pb.ParseFromString(blob)) {
                res.avatarUrl = pb.avatar_url();
                res.userId = pb.user_id();
                for (const auto& f : pb.friends()) {
                    FriendEntry fe;
                    fe.npid = f.npid();
                    fe.online = f.online();
                    res.friends.push_back(std::move(fe));
                }
                for (const auto& n : pb.friend_requests_sent())
                    res.requestsSent.push_back(n);
                for (const auto& n : pb.friend_requests_received())
                    res.requestsReceived.push_back(n);
                for (const auto& n : pb.blocked())
                    res.blocked.push_back(n);

                m_avatar_url = res.avatarUrl;
                m_user_id = res.userId;
                {
                    std::lock_guard lock(m_mutex_friends);
                    m_friends = res.friends;
                }
                m_authenticated = true;
                LOG_INFO("Logged in npid='{}' userId={} friends={}", m_npid, m_user_id,
                         m_friends.size());

                const u64 pkt_id = m_pkt_counter.fetch_add(1);
                std::vector<u8> empty_payload;
                std::vector<u8> pkt = BuildPacket(CommandType::GetToken, pkt_id, empty_payload);
                {
                    std::lock_guard lock(m_mutex_send_queue);
                    m_send_queue.push_back(std::move(pkt));
                }
                m_cv_send_queue.notify_one();
                LOG_DEBUG("GetToken request fired pkt_id={}", pkt_id);
            } else {
                res.error = ErrorType::Malformed;
                LOG_ERROR("Failed to parse LoginReply proto");
                m_state = ShadNetState::FailureProtocol;
                sem_post(&m_sem_authenticated);
                DoDisconnect();
            }
        } else {
            switch (res.error) {
            case ErrorType::LoginAlreadyLoggedIn:
                m_state = ShadNetState::FailureAlreadyIn;
                break;
            case ErrorType::LoginInvalidUsername:
                m_state = ShadNetState::FailureUsername;
                break;
            case ErrorType::LoginInvalidPassword:
                m_state = ShadNetState::FailurePassword;
                break;
            case ErrorType::LoginInvalidToken:
                m_state = ShadNetState::FailureToken;
                break;
            default:
                m_state = ShadNetState::FailureAuth;
                break;
            }
            LOG_ERROR("Login rejected, error code {}", static_cast<u8>(res.error));
            sem_post(&m_sem_authenticated);
            DoDisconnect();
        }
    }

    if (onLoginResult)
        onLoginResult(res);
}

void ShadNetClient::HandleGetTokenReply(const std::vector<u8>& payload) {
    if (payload.empty()) {
        LOG_ERROR("Empty reply");
        RequestServerFeatures();
        return;
    }
    const ErrorType err = static_cast<ErrorType>(payload[0]);
    if (err != ErrorType::NoError) {
        LOG_WARNING("returned error {} - WebAPI calls will be unauthenticated",
                    magic_enum::enum_name(err));
        RequestServerFeatures();
        return;
    }
    shadnet::GetTokenReply pb;
    const std::string blob = ExtractBlob(payload, 1);
    if (blob.empty() || !pb.ParseFromString(blob)) {
        LOG_ERROR("Failed to parse proto");
        RequestServerFeatures();
        return;
    }
    {
        std::lock_guard lock(m_mutex_bearer);
        m_bearer_token = pb.token();
    }
    LOG_INFO("Bearer token captured ({} chars) for accountID {} canonical npid '{}'",
             pb.token().size(), pb.user_id(), pb.npid());
    RequestServerFeatures();
}

void ShadNetClient::HandleServerFeaturesReply(const std::vector<u8>& payload) {
    bool matching2_enabled = false;
    bool parsed = false;

    if (!payload.empty()) {
        const ErrorType err = static_cast<ErrorType>(payload[0]);
        if (err == ErrorType::NoError) {
            shadnet::ServerFeaturesReply pb;
            const std::string blob = ExtractBlob(payload, 1);
            if (!blob.empty() && pb.ParseFromString(blob)) {
                matching2_enabled = pb.matching2_enabled();
                parsed = true;
            }
        } else {
            LOG_WARNING("returned error {} - assuming Matching2 disabled", static_cast<int>(err));
        }
    }

    m_matching2_enabled.store(matching2_enabled);
    m_server_features_received.store(parsed);
    LOG_INFO("Server features: matching2_enabled={}{}", matching2_enabled ? "true" : "false",
             parsed ? "" : " (defaulted)");
    sem_post(&m_sem_authenticated);
}

// Notifications

void ShadNetClient::HandleNotification(u16 cmd_raw, const std::vector<u8>& payload) {
    // Notification payload = u32 LE blob size + proto bytes
    const std::string blob = ExtractBlob(payload, 0);
    if (blob.empty() &&
        static_cast<NotificationType>(cmd_raw) != NotificationType::WebApiPushEvent) {
        // WebApiPushEvent is multi-field,its first field (service name) may be empty
        // legitimately, so it parses its own payload below rather than relying on blob.
        LOG_WARNING("Empty payload type={}", cmd_raw);
        return;
    }

    switch (static_cast<NotificationType>(cmd_raw)) {
    case NotificationType::FriendQuery: {
        shadnet::NotifyFriendQuery pb;
        if (!pb.ParseFromString(blob)) {
            LOG_WARNING("FriendQuery parse error");
            break;
        }
        NotifyFriendQuery n;
        n.fromNpid = pb.from_npid();
        LOG_DEBUG("FriendQuery from '{}'", n.fromNpid);
        if (onFriendQuery)
            onFriendQuery(n);
        break;
    }
    case NotificationType::FriendNew: {
        shadnet::NotifyFriendNew pb;
        if (!pb.ParseFromString(blob)) {
            LOG_WARNING("FriendNew parse error");
            break;
        }
        NotifyFriendNew n;
        n.npid = pb.npid();
        n.online = pb.online();
        LOG_DEBUG("FriendNew '{}' ({})", n.npid, n.online ? "online" : "offline");
        if (onFriendNew)
            onFriendNew(n);
        break;
    }
    case NotificationType::FriendLost: {
        shadnet::NotifyFriendLost pb;
        if (!pb.ParseFromString(blob)) {
            LOG_WARNING("FriendLost parse error");
            break;
        }
        NotifyFriendLost n;
        n.npid = pb.npid();
        LOG_DEBUG("FriendLost '{}'", n.npid);
        if (onFriendLost)
            onFriendLost(n);
        break;
    }
    case NotificationType::FriendStatus: {
        shadnet::NotifyFriendStatus pb;
        if (!pb.ParseFromString(blob)) {
            LOG_WARNING("FriendStatus parse error");
            break;
        }
        NotifyFriendStatus n;
        n.npid = pb.npid();
        n.online = pb.online();
        n.timestamp = pb.timestamp();
        LOG_DEBUG("FriendStatus '{}' is {}", n.npid, n.online ? "online" : "offline");
        if (onFriendStatus)
            onFriendStatus(n);
        break;
    }
    case NotificationType::RoomEvent: {
        shadnet::NotifyRoomEvent pb;
        if (!pb.ParseFromString(blob)) {
            LOG_WARNING("RoomEvent parse error");
            break;
        }
        NotifyRoomEvent n;
        n.ctx_id = pb.ctx_id();
        n.room_id = pb.room_id();
        n.event = pb.event();
        n.event_cause = pb.event_cause();
        n.error_code = pb.error_code();
        n.flags = pb.flags();
        n.has_passwd_mask = pb.has_passwd_mask();
        n.passwd_slot_mask = pb.passwd_slot_mask();
        if (pb.has_member()) {
            const auto& m = pb.member();
            n.member_npid = m.npid();
            n.member_account_id = m.account_id();
            n.member_platform = m.platform();
            n.member_id = m.member_id();
            n.member_team_id = m.team_id();
            n.member_is_owner = m.is_owner();
            n.member_join_date = m.join_date();
            n.member_nat_type = m.nat_type();
            n.member_flag_attr = m.flag_attr();
            n.member_group_id = m.group_id();
            n.member_addr = m.addr();
            n.member_port = m.port();
            for (const auto& a : m.bin_attrs_internal()) {
                MatchingBinAttr ba;
                ba.attr_id = a.attr_id();
                ba.update_date = a.update_date();
                ba.update_member_id = a.update_member_id();
                ba.data.assign(a.data().begin(), a.data().end());
                n.member_bin_attrs.push_back(std::move(ba));
            }
        }
        for (const auto& a : pb.bin_attrs()) {
            MatchingBinAttr ba;
            ba.attr_id = a.attr_id();
            ba.update_date = a.update_date();
            ba.update_member_id = a.update_member_id();
            ba.data.assign(a.data().begin(), a.data().end());
            n.bin_attrs.push_back(std::move(ba));
        }
        LOG_DEBUG("RoomEvent room_id={} event={:#x} cause={}", n.room_id, n.event, n.event_cause);
        if (onRoomEvent)
            onRoomEvent(n);
        break;
    }
    case NotificationType::RoomMessage: {
        shadnet::NotifyRoomMessage pb;
        if (!pb.ParseFromString(blob)) {
            break;
        }
        NotifyRoomMessage n;
        n.ctx_id = pb.ctx_id();
        n.room_id = pb.room_id();
        n.src_member_id = pb.src_member_id();
        n.event = pb.event();
        n.cast_type = pb.cast_type();
        for (const u32 member_id : pb.dst_member_ids()) {
            n.dst_member_ids.push_back(member_id);
        }
        n.src_npid = pb.src_npid();
        n.src_account_id = pb.src_account_id();
        n.src_platform = pb.src_platform();
        n.msg.assign(pb.msg().begin(), pb.msg().end());
        if (onRoomMessage)
            onRoomMessage(n);
        break;
    }
    case NotificationType::WebApiPushEvent: {
        // Length-prefixed fields: npServiceName, npServiceLabel(u32 LE), dataType,
        // data, fromNpid, toNpid. ExtractBlob returns {} past the end, so a short
        // payload degrades to empty trailing fields rather than reading OOB.
        NotifyWebApiPushEvent n;
        int off = 0;
        n.npServiceName = ExtractBlob(payload, off);
        off += 4 + static_cast<int>(n.npServiceName.size());
        if (off + 4 <= static_cast<int>(payload.size())) {
            n.npServiceLabel = GetLE32(payload.data() + off);
            off += 4;
        }
        n.dataType = ExtractBlob(payload, off);
        off += 4 + static_cast<int>(n.dataType.size());
        n.data = ExtractBlob(payload, off);
        off += 4 + static_cast<int>(n.data.size());
        n.fromNpid = ExtractBlob(payload, off);
        off += 4 + static_cast<int>(n.fromNpid.size());
        n.toNpid = ExtractBlob(payload, off);
        off += 4 + static_cast<int>(n.toNpid.size());
        // Optional extended-data section appended after toNpid: u32 LE count, then
        // (blob key, blob value) per pair. Absent on older servers -> no bytes left ->
        // zero pairs. Length-guarded throughout; count capped to avoid runaway on a
        // malformed packet.
        if (off + 4 <= static_cast<int>(payload.size())) {
            const u32 count = GetLE32(payload.data() + off);
            off += 4;
            for (u32 i = 0; i < count && i < 256; ++i) {
                if (off + 4 > static_cast<int>(payload.size()))
                    break;
                std::string key = ExtractBlob(payload, off);
                off += 4 + static_cast<int>(key.size());
                if (off + 4 > static_cast<int>(payload.size()))
                    break;
                std::string val = ExtractBlob(payload, off);
                off += 4 + static_cast<int>(val.size());
                n.extdData.emplace_back(std::move(key), std::move(val));
            }
        }
        LOG_INFO("WebApiPushEvent svc='{}' type='{}' from='{}' bytes={} extd={}", n.npServiceName,
                 n.dataType, n.fromNpid, n.data.size(), n.extdData.size());
        if (onWebApiPushEvent)
            onWebApiPushEvent(n);
        break;
    }
    default:
        LOG_DEBUG("Unknown notification type {}", cmd_raw);
        break;
    }
}

void ShadNetClient::PutLE16(std::vector<u8>& b, size_t off, u16 v) {
    b[off] = static_cast<u8>(v);
    b[off + 1] = static_cast<u8>(v >> 8);
}
void ShadNetClient::PutLE32(std::vector<u8>& b, size_t off, u32 v) {
    b[off] = static_cast<u8>(v);
    b[off + 1] = static_cast<u8>(v >> 8);
    b[off + 2] = static_cast<u8>(v >> 16);
    b[off + 3] = static_cast<u8>(v >> 24);
}
void ShadNetClient::PutLE64(std::vector<u8>& b, size_t off, u64 v) {
    for (int i = 0; i < 8; ++i)
        b[off + i] = static_cast<u8>(v >> (8 * i));
}
u16 ShadNetClient::GetLE16(const u8* p) {
    return static_cast<u16>(p[0]) | (static_cast<u16>(p[1]) << 8);
}
u32 ShadNetClient::GetLE32(const u8* p) {
    return static_cast<u32>(p[0]) | (static_cast<u32>(p[1]) << 8) | (static_cast<u32>(p[2]) << 16) |
           (static_cast<u32>(p[3]) << 24);
}
u64 ShadNetClient::GetLE64(const u8* p) {
    u64 v = 0;
    for (int i = 0; i < 8; ++i)
        v |= static_cast<u64>(p[i]) << (8 * i);
    return v;
}

} // namespace ShadNet
