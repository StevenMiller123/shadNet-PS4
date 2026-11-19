// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <chrono>
#include <cstring>
#include <optional>
#include <random>
#include <string>
#include <fmt/format.h>
#include <orbis/Net.h>

#include "common/logging/log.h"
#include "common/plugin_common.h"
#include "core/libraries/network/fake_psn/fake_signaling.h"
#include "core/libraries/network/fake_psn/fake_stun.h"
#include "core/libraries/network/net/net_error.h"

namespace Libraries::Network {

namespace {

enum class Field : u16 {
    MessageType = 0x01,
    SourceConnection = 0x02,
    DestinationConnection = 0x03,
    Nonce = 0x04,
    Candidate = 0x05,
    CandidateKind = 0x06,
    CandidateAddress = 0x07,
    CandidatePort = 0x08,
    CandidateMappedAddress = 0x09,
    CandidateMappedPort = 0x0a,
    TransactionId = 0x11,
    Result = 0x12,
    Unknown14 = 0x14,
};

enum class MessageType : u32 {
    ConnectRequest = 1,
    ConnectResult = 2,
    Disconnect = 3,
    Ack = 4,
};

// Transaction ids have this in their upper bits.
constexpr u32 TransactionIdBase = 0x42000000;

// libSceNpMatching2 echoes connections over UDP, from and to this vport.
constexpr u16 EchoVport = 0xfffe;
constexpr std::string_view EchoSocketName = "SceNpMatching2SigEcho";

// shadNet signals from and to this vport.
constexpr u16 ShadNetVport = 0xffff;

constexpr s32 AnswerPollInterval = 20000;
constexpr u32 MaxTrafficLogged = 32;
constexpr u64 MaxTrafficLogBytes = 64;

enum class EchoType : u8 {
    Ping = 1,
    Pong = 2,
    // Account id versions.
    PingA = 6,
    PongA = 7,
};

constexpr std::array<u8, 4> ShadNetMagic = {'S', 'H', 'A', 'D'};

enum class ShadNetPacketType : u8 {
    EchoPing = 0x05,
    EchoPong = 0x06,
    Handshake = 0x07,
    Control = 0x10,
};

enum class HandshakeKind : u8 {
    Offer = 1,
    Accept = 2,
    Check = 3,
    CheckAck = 4,
};

enum class ControlKind : u8 {
    ActivationRequest = 1,
    ActivationAck = 2,
    Close = 3,
    Established = 4,
};

#pragma pack(push, 1)
struct EchoPacket {
    u8 type;
    std::array<u8, 3> reserved;
    std::array<u8, 0x20> sender;
    std::array<u8, 0x20> receiver;
    std::array<u8, 2> sender_connection;
    std::array<u8, 2> receiver_connection;
    std::array<u8, 8> timestamp;
    std::array<u8, 4> mapped_addr;
    std::array<u8, 2> mapped_port;
    std::array<u8, 2> reserved2;
};
static_assert(sizeof(EchoPacket) == 0x58);

struct ShadNetEcho {
    std::array<u8, 4> magic;
    ShadNetPacketType type;
    u32 connection;
    u64 timestamp;
};
static_assert(sizeof(ShadNetEcho) == 0x11);

struct ShadNetHandshake {
    std::array<u8, 4> magic;
    ShadNetPacketType type;
    HandshakeKind kind;
    std::array<u8, 16> online_id_from;
    std::array<u8, 16> online_id_to;
    u32 from_connection;
    u32 to_connection;
    u32 mapped_addr;
    u16 mapped_port;
    u16 reserved;
    u64 nonce;
};
static_assert(sizeof(ShadNetHandshake) == 0x3e);

struct ShadNetControl {
    std::array<u8, 4> magic;
    ShadNetPacketType type;
    ControlKind kind;
    std::array<u8, 16> online_id_from;
    std::array<u8, 16> online_id_to;
    u32 from_connection;
    u32 mapped_addr;
    u16 mapped_port;
    u16 reason;
};
static_assert(sizeof(ShadNetControl) == 0x32);
#pragma pack(pop)

struct FieldView {
    u16 type;
    std::span<const u8> value;
};

std::vector<FieldView> ParseFields(std::span<const u8> data) {
    std::vector<FieldView> fields;
    u64 offset = 0;
    while (offset + 4 <= data.size()) {
        const u16 type = static_cast<u16>(data[offset] << 8 | data[offset + 1]);
        const u16 size = static_cast<u16>(data[offset + 2] << 8 | data[offset + 3]);
        if (offset + 4 + size > data.size()) {
            break;
        }
        fields.push_back({type, data.subspan(offset + 4, size)});
        offset += 4 + size;
    }
    return fields;
}

std::optional<std::span<const u8>> FindField(const std::vector<FieldView>& fields, Field field) {
    for (const auto& entry : fields) {
        if (entry.type == static_cast<u16>(field)) {
            return entry.value;
        }
    }
    return std::nullopt;
}

std::optional<u32> ReadU32(const std::vector<FieldView>& fields, Field field) {
    const auto value = FindField(fields, field);
    if (!value || value->size() != 4) {
        return std::nullopt;
    }
    return static_cast<u32>((*value)[0] << 24 | (*value)[1] << 16 | (*value)[2] << 8 | (*value)[3]);
}

std::optional<u16> ReadU16(const std::vector<FieldView>& fields, Field field) {
    const auto value = FindField(fields, field);
    if (!value || value->size() != 2) {
        return std::nullopt;
    }
    return static_cast<u16>((*value)[0] << 8 | (*value)[1]);
}

// Copies a field's raw bytes, for values kept in network byte order.
template <typename T>
bool ReadRaw(const std::vector<FieldView>& fields, Field field, T& out) {
    const auto value = FindField(fields, field);
    if (!value || value->size() != sizeof(T)) {
        return false;
    }
    std::memcpy(&out, value->data(), sizeof(T));
    return true;
}

std::string Describe(std::span<const u8> data, u32 depth = 0) {
    std::string out;
    for (const auto& field : ParseFields(data)) {
        out += fmt::format("\n{:{}}{:#x} ({} bytes):", "", depth * 2 + 4, field.type,
                           field.value.size());
        if (field.type == static_cast<u16>(Field::Candidate)) {
            out += Describe(field.value, depth + 1);
            continue;
        }
        for (const u8 byte : field.value) {
            out += fmt::format(" {:02x}", byte);
        }
    }
    return out;
}

std::string HexDump(std::span<const u8> data) {
    std::string out;
    for (const u8 byte : data) {
        out += fmt::format("{:02x} ", byte);
    }
    return out;
}

std::string OnlineIdString(const std::array<u8, 16>& online_id) {
    const auto* chars = reinterpret_cast<const char*>(online_id.data());
    return {chars, strnlen(chars, online_id.size())};
}

void AppendField(std::vector<u8>& out, Field field, std::span<const u8> value) {
    const u16 type = static_cast<u16>(field);
    out.insert(out.end(), {static_cast<u8>(type >> 8), static_cast<u8>(type),
                           static_cast<u8>(value.size() >> 8), static_cast<u8>(value.size())});
    out.insert(out.end(), value.begin(), value.end());
}

void AppendU32(std::vector<u8>& out, Field field, u32 value) {
    const std::array<u8, 4> bytes = {static_cast<u8>(value >> 24), static_cast<u8>(value >> 16),
                                     static_cast<u8>(value >> 8), static_cast<u8>(value)};
    AppendField(out, field, bytes);
}

void AppendU16(std::vector<u8>& out, Field field, u16 value) {
    const std::array<u8, 2> bytes = {static_cast<u8>(value >> 8), static_cast<u8>(value)};
    AppendField(out, field, bytes);
}

std::vector<u8> MakeAck(u32 transaction_id) {
    std::vector<u8> out;
    AppendU32(out, Field::MessageType, static_cast<u32>(MessageType::Ack));
    AppendU32(out, Field::TransactionId, transaction_id);
    AppendU32(out, Field::Result, 0);
    return out;
}

// Answers a ping the way the peer would, telling the console the address it pinged from.
EchoPacket MakePong(const EchoPacket& ping, u32 console_addr, u16 console_port) {
    EchoPacket pong{};
    pong.type = static_cast<u8>(ping.type + 1);
    pong.sender = ping.receiver;
    pong.receiver = ping.sender;
    pong.sender_connection = ping.receiver_connection;
    pong.receiver_connection = ping.sender_connection;
    pong.timestamp = ping.timestamp;

    std::array<u8, 4> addr{};
    std::array<u8, 2> port{};
    std::memcpy(addr.data(), &console_addr, sizeof(console_addr));
    std::memcpy(port.data(), &console_port, sizeof(console_port));
    for (u64 i = 0; i < 2; i++) {
        pong.mapped_addr[i] = addr[i] ^ pong.sender_connection[i];
        pong.mapped_addr[i + 2] = addr[i + 2] ^ pong.receiver_connection[i];
        pong.mapped_port[i] = port[i] ^ pong.sender_connection[i];
    }
    return pong;
}

OrbisNetSockaddrIn MakeSockaddr(u32 addr, u16 port, u16 vport) {
    OrbisNetSockaddrIn sa{};
    sa.sin_len = sizeof(sa);
    sa.sin_family = ORBIS_NET_AF_INET;
    sa.sin_port = port;
    sa.sin_addr = addr;
    sa.sin_vport = sceNetHtons(vport);
    return sa;
}

} // namespace

FakeSignaling& FakeSignaling::Instance() {
    static FakeSignaling instance;
    return instance;
}

u32 FakeSignaling::NextTransactionId() {
    m_transaction = m_transaction == 0xffff ? 1 : m_transaction + 1;
    return TransactionIdBase | m_transaction;
}

void FakeSignaling::OnMemberJoined(u64 room_id, u16 member_id, std::string_view addr, u16 port) {
    Peer peer{};
    const std::string addr_string(addr);
    if (sceNetInetPton(ORBIS_NET_AF_INET, addr_string.c_str(), &peer.addr) != 1) {
        LOG_WARNING(Lib_NpMatching2, "signaling peer {} has no usable address '{}'", member_id,
                    addr);
    }
    peer.port = sceNetHtons(port);

    // Connection ids only need to be unique for the console.
    std::random_device random;
    peer.peer_connection = static_cast<u16>(random() | 1);
    for (u8& byte : peer.nonce) {
        byte = static_cast<u8>(random());
    }

    {
        std::scoped_lock lk{m_mutex};
        m_peers[{room_id, member_id}] = peer;
    }
    LOG_INFO(Lib_NpMatching2, "signaling peer {} in room {} is at {}:{}", member_id, room_id, addr,
             port);
    StartResponder();
}

void FakeSignaling::OnMemberLeft(u64 room_id, u16 member_id) {
    std::scoped_lock lk{m_mutex};
    m_peers.erase({room_id, member_id});
}

void FakeSignaling::OnRoomLeft(u64 room_id) {
    std::scoped_lock lk{m_mutex};
    std::erase_if(m_peers, [&](const auto& entry) { return entry.first.first == room_id; });
}

std::vector<FakeSignaling::Message> FakeSignaling::OnMessage(u64 room_id, u16 src_member_id,
                                                             u16 dst_member_id,
                                                             std::span<const u8> data) {
    const auto fields = ParseFields(data);
    const auto type = ReadU32(fields, Field::MessageType);
    const auto transaction_id = ReadU32(fields, Field::TransactionId);
    LOG_DEBUG(Lib_NpMatching2, "signaling message {} from member {} to member {} in room {}:{}",
              type ? *type : 0, src_member_id, dst_member_id, room_id, Describe(data));
    if (!type || !transaction_id || *type == static_cast<u32>(MessageType::Ack)) {
        return {};
    }

    // Peers acknowledge every message besides acknowledgements
    std::vector<Message> replies;
    replies.push_back({dst_member_id, src_member_id, MakeAck(*transaction_id)});

    std::scoped_lock lk{m_mutex};
    const auto it = m_peers.find({room_id, dst_member_id});
    if (it == m_peers.end()) {
        LOG_WARNING(Lib_NpMatching2, "signaling message for unknown peer {} in room {}",
                    dst_member_id, room_id);
        return replies;
    }
    Peer& peer = it->second;

    switch (static_cast<MessageType>(*type)) {
    case MessageType::ConnectRequest: {
        const auto console_connection = ReadU16(fields, Field::SourceConnection);
        if (!console_connection) {
            break;
        }
        peer.console_connection = *console_connection;

        // Pongs tell the console the address it pinged from
        if (const auto candidate = FindField(fields, Field::Candidate)) {
            const auto candidate_fields = ParseFields(*candidate);
            ReadRaw(candidate_fields, Field::CandidateAddress, peer.console_addr);
            ReadRaw(candidate_fields, Field::CandidatePort, peer.console_port);
        }

        // Peers send their own connection request with an address to reach them at.
        std::vector<u8> request;
        AppendU32(request, Field::MessageType, static_cast<u32>(MessageType::ConnectRequest));
        AppendU32(request, Field::TransactionId, NextTransactionId());
        AppendU16(request, Field::SourceConnection, peer.peer_connection);
        AppendU16(request, Field::DestinationConnection, peer.console_connection);
        AppendField(request, Field::Nonce, peer.nonce);
        AppendU16(request, Field::Unknown14, 1);

        std::vector<u8> candidate;
        AppendU32(candidate, Field::CandidateKind, 1);
        AppendField(candidate, Field::CandidateAddress,
                    {reinterpret_cast<const u8*>(&peer.addr), sizeof(peer.addr)});
        AppendField(candidate, Field::CandidatePort,
                    {reinterpret_cast<const u8*>(&peer.port), sizeof(peer.port)});
        AppendField(request, Field::Candidate, candidate);

        LOG_DEBUG(Lib_NpMatching2, "signaling request from peer {} to member {}:{}", dst_member_id,
                  src_member_id, Describe(request));
        replies.push_back({dst_member_id, src_member_id, std::move(request)});
        break;
    }
    case MessageType::ConnectResult: {
        // The console only activates the connection once the peer sends its result too.
        std::vector<u8> result;
        AppendU32(result, Field::MessageType, static_cast<u32>(MessageType::ConnectResult));
        AppendU32(result, Field::TransactionId, NextTransactionId());
        AppendU16(result, Field::SourceConnection, peer.peer_connection);
        AppendU16(result, Field::DestinationConnection, peer.console_connection);
        AppendField(result, Field::Nonce, peer.nonce);

        std::vector<u8> candidate;
        AppendU32(candidate, Field::CandidateKind, 1);
        AppendField(candidate, Field::CandidateAddress,
                    {reinterpret_cast<const u8*>(&peer.console_addr), sizeof(peer.console_addr)});
        AppendField(candidate, Field::CandidatePort,
                    {reinterpret_cast<const u8*>(&peer.console_port), sizeof(peer.console_port)});
        AppendField(candidate, Field::CandidateMappedAddress,
                    {reinterpret_cast<const u8*>(&peer.addr), sizeof(peer.addr)});
        AppendField(candidate, Field::CandidateMappedPort,
                    {reinterpret_cast<const u8*>(&peer.port), sizeof(peer.port)});
        AppendField(result, Field::Candidate, candidate);

        LOG_DEBUG(Lib_NpMatching2, "signaling result from peer {} to member {}:{}", dst_member_id,
                  src_member_id, Describe(result));
        replies.push_back({dst_member_id, src_member_id, std::move(result)});
        break;
    }
    case MessageType::Disconnect:
        peer.console_connection = 0;
        break;
    default:
        break;
    }
    return replies;
}

void FakeSignaling::OnSocketCreate(s32 socket, const char* name) {
    if (name == nullptr || name != EchoSocketName) {
        return;
    }
    std::scoped_lock lk{m_mutex};
    m_echo_socket = socket;
}

void FakeSignaling::OnSocketClose(s32 socket) {
    bool echo_closed = false;
    {
        std::scoped_lock lk{m_mutex};
        m_answers.erase(socket);
        m_traffic_logged.erase(socket);
        for (auto& [epoll, registrations] : m_epolls) {
            registrations.erase(socket);
        }
        if (socket == m_echo_socket) {
            m_echo_socket = -1;
            echo_closed = true;
        }
    }

    // Close our responder when closing the signaling socket
    if (echo_closed) {
        StopResponder();
    }
}

const FakeSignaling::Peer* FakeSignaling::FindPeerLocked(const OrbisNetSockaddr* addr,
                                                         u32 addr_size) const {
    if (addr == nullptr || addr_size < sizeof(OrbisNetSockaddrIn)) {
        return nullptr;
    }
    const auto* in = reinterpret_cast<const OrbisNetSockaddrIn*>(addr);
    for (const auto& [key, peer] : m_peers) {
        if (peer.addr == in->sin_addr && peer.port == in->sin_port) {
            return &peer;
        }
    }
    return nullptr;
}

std::optional<s32> FakeSignaling::OnSendTo(s32 socket, std::span<const u8> data,
                                           const OrbisNetSockaddr* to, u32 to_size) {
    std::scoped_lock lk{m_mutex};
    if (socket == m_responder_socket) {
        return std::nullopt;
    }
    const Peer* peer = FindPeerLocked(to, to_size);
    EchoPacket ping{};
    if (peer == nullptr || data.size() != sizeof(ping)) {
        LogPeerTrafficLocked("sent", socket, data, to, to_size);
        return std::nullopt;
    }
    std::memcpy(&ping, data.data(), sizeof(ping));
    if (ping.type != static_cast<u8>(EchoType::Ping) &&
        ping.type != static_cast<u8>(EchoType::PingA)) {
        LOG_DEBUG(Lib_NpMatching2, "signaling echo type {} sent on socket {}", ping.type, socket);
        return std::nullopt;
    }

    // Pongs arrive on the socket that pinged
    s32 target = m_echo_socket;
    for (const auto& [epoll, registrations] : m_epolls) {
        if (registrations.contains(socket)) {
            target = socket;
            break;
        }
    }
    if (target < 0) {
        LOG_WARNING(Lib_NpMatching2, "no socket to answer the signaling echo from socket {}",
                    socket);
        return std::nullopt;
    }

    const EchoPacket pong = MakePong(ping, peer->console_addr, peer->console_port);
    const auto* pong_bytes = reinterpret_cast<const u8*>(&pong);
    m_answers[target].push_back({
        .data = {pong_bytes, pong_bytes + sizeof(pong)},
        .from = MakeSockaddr(peer->addr, peer->port, EchoVport),
    });
    LOG_DEBUG(Lib_NpMatching2, "answered signaling echo from socket {} on socket {}", socket,
              target);
    return static_cast<s32>(data.size());
}

std::optional<s32> FakeSignaling::OnRecvFrom(s32 socket, void* buf, u64 size,
                                             OrbisNetSockaddr* from, u32* from_size) {
    std::scoped_lock lk{m_mutex};
    const auto it = m_answers.find(socket);
    if (it == m_answers.end()) {
        return std::nullopt;
    }
    const Answer answer = std::move(it->second.front());
    it->second.pop_front();
    if (it->second.empty()) {
        m_answers.erase(it);
    }

    const u64 copied = std::min<u64>(size, answer.data.size());
    if (buf != nullptr) {
        std::memcpy(buf, answer.data.data(), copied);
    }
    if (from != nullptr && from_size != nullptr) {
        const u32 from_copied = std::min<u32>(*from_size, sizeof(answer.from));
        std::memcpy(from, &answer.from, from_copied);
        *from_size = from_copied;
    }
    return static_cast<s32>(copied);
}

void FakeSignaling::OnReceived(s32 socket, std::span<const u8> data, const OrbisNetSockaddr* from,
                               u32 from_size) {
    std::scoped_lock lk{m_mutex};
    if (socket != m_responder_socket) {
        LogPeerTrafficLocked("received", socket, data, from, from_size);
    }
}

void FakeSignaling::LogPeerTrafficLocked(std::string_view direction, s32 socket,
                                         std::span<const u8> data, const OrbisNetSockaddr* addr,
                                         u32 addr_size) {
    if (addr == nullptr || addr_size < sizeof(OrbisNetSockaddrIn) ||
        addr->sa_family != ORBIS_NET_AF_INET) {
        return;
    }
    const auto* in = reinterpret_cast<const OrbisNetSockaddrIn*>(addr);
    const bool with_peer = std::ranges::any_of(
        m_peers, [&](const auto& entry) { return entry.second.addr == in->sin_addr; });
    if (!with_peer || m_traffic_logged[socket]++ >= MaxTrafficLogged) {
        return;
    }
    LOG_TRACE(Lib_NpMatching2, "peer traffic {} on socket {}, {} bytes, port {} vport {}: {}",
              direction, socket, data.size(), sceNetNtohs(in->sin_port), sceNetNtohs(in->sin_vport),
              HexDump(data.first(std::min<u64>(data.size(), MaxTrafficLogBytes))));
}

void FakeSignaling::OnEpollControl(s32 epoll, s32 op, s32 socket,
                                   const Net::OrbisNetEpollEvent* event) {
    std::scoped_lock lk{m_mutex};
    if (op == Net::ORBIS_NET_EPOLL_CTL_ADD && socket >= 0 && socket == m_echo_socket) {
        m_epolls.try_emplace(epoll);
    }
    const auto it = m_epolls.find(epoll);
    if (it == m_epolls.end()) {
        return;
    }
    switch (op) {
    case Net::ORBIS_NET_EPOLL_CTL_ADD:
    case Net::ORBIS_NET_EPOLL_CTL_MOD:
        if (event != nullptr) {
            it->second[socket] = *event;
        }
        break;
    case Net::ORBIS_NET_EPOLL_CTL_DEL:
        it->second.erase(socket);
        break;
    default:
        break;
    }
}

void FakeSignaling::OnEpollDestroy(s32 epoll) {
    std::scoped_lock lk{m_mutex};
    m_epolls.erase(epoll);
}

s32 FakeSignaling::CollectAnswersLocked(s32 epoll, Net::OrbisNetEpollEvent* events,
                                        s32 max_events) const {
    const auto it = m_epolls.find(epoll);
    if (it == m_epolls.end()) {
        return 0;
    }
    s32 count = 0;
    for (const auto& [socket, registration] : it->second) {
        if (count == max_events) {
            break;
        }
        if ((registration.events & Net::ORBIS_NET_EPOLLIN) == 0 || !m_answers.contains(socket)) {
            continue;
        }
        events[count] = registration;
        events[count].events = Net::ORBIS_NET_EPOLLIN;
        events[count].ident = static_cast<u64>(socket);
        count++;
    }
    return count;
}

std::optional<s32> FakeSignaling::OnEpollWait(s32 epoll, Net::OrbisNetEpollEvent* events,
                                              s32 max_events, s32 timeout,
                                              const std::function<s32(s32 timeout)>& wait) {
    {
        std::scoped_lock lk{m_mutex};
        if (!m_epolls.contains(epoll)) {
            return std::nullopt;
        }
    }
    if (events == nullptr || max_events <= 0) {
        return std::nullopt;
    }

    // Answers don't wake the real wait, so wait in slices and check for them in between.
    const auto start = std::chrono::steady_clock::now();
    for (;;) {
        {
            std::scoped_lock lk{m_mutex};
            if (const s32 count = CollectAnswersLocked(epoll, events, max_events); count != 0) {
                return count;
            }
        }
        s32 slice = AnswerPollInterval;
        if (timeout >= 0) {
            const s64 elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                                    std::chrono::steady_clock::now() - start)
                                    .count();
            slice = static_cast<s32>(std::clamp<s64>(timeout - elapsed, 0, AnswerPollInterval));
        }
        const s32 ret = wait(slice);
        if (ret != 0) {
            return ret;
        }
        if (timeout >= 0 && slice < AnswerPollInterval) {
            std::scoped_lock lk{m_mutex};
            return CollectAnswersLocked(epoll, events, max_events);
        }
    }
}

s32 FakeSignaling::SendToShadNet(std::span<const u8> data, const OrbisNetSockaddrIn& to) {
    const s32 socket = StartResponder();
    if (socket < 0) {
        return socket;
    }
    OrbisNetSockaddrIn addr = to;
    addr.sin_vport = sceNetHtons(ShadNetVport);
    return ::sceNetSendto(socket, data.data(), data.size(), 0,
                          reinterpret_cast<const OrbisNetSockaddr*>(&addr), sizeof(addr));
}

s32 FakeSignaling::StartResponder() {
    std::scoped_lock responder_lk{m_responder_mutex};
    {
        std::scoped_lock lk{m_mutex};
        if (m_responder_socket >= 0) {
            return m_responder_socket;
        }
    }

    const s32 socket =
        ::sceNetSocket("shadNetSignaling", ORBIS_NET_AF_INET, Net::ORBIS_NET_SOCK_DGRAM_P2P, 0);
    if (socket < 0) {
        LOG_ERROR(Lib_NpMatching2, "could not create the signaling responder socket, {:#x}",
                  static_cast<u32>(socket));
        return socket;
    }
    const u16 port = sceNetHtons(Net::ORBIS_NET_ADHOC_PORT);
    const OrbisNetSockaddrIn addr = MakeSockaddr(0, port, ShadNetVport);
    s32 ret = 0;
    Common::RetryPrivileged([&] {
        ret = sceNetBind(socket, reinterpret_cast<const OrbisNetSockaddr*>(&addr), sizeof(addr));
        return ret >= 0;
    });
    if (ret < 0) {
        LOG_ERROR(Lib_NpMatching2, "could not bind the signaling responder to port {}, {:#x}",
                  Net::ORBIS_NET_ADHOC_PORT, static_cast<u32>(ret));
        ::sceNetSocketClose(socket);
        return ret;
    }

    {
        std::scoped_lock lk{m_mutex};
        m_responder_socket = socket;
    }
    m_responder =
        std::jthread([this, socket](std::stop_token stop) { ResponderThread(stop, socket); });
    LOG_INFO(Lib_NpMatching2, "answering shadnet signaling on port {}", Net::ORBIS_NET_ADHOC_PORT);
    return socket;
}

void FakeSignaling::StopResponder() {
    std::scoped_lock responder_lk{m_responder_mutex};
    s32 socket = -1;
    {
        std::scoped_lock lk{m_mutex};
        socket = std::exchange(m_responder_socket, -1);
    }
    if (m_responder.joinable()) {
        m_responder.request_stop();
        m_responder.join();
    }
    if (socket >= 0) {
        ::sceNetSocketClose(socket);
        LOG_INFO(Lib_NpMatching2, "stopped answering shadnet signaling");
    }
}

void FakeSignaling::ResponderThread(std::stop_token stop, s32 socket) {
    std::array<u8, 0x100> buf{};
    while (!stop.stop_requested()) {
        OrbisNetSockaddrIn from{};
        OrbisNetSocklen_t from_size = sizeof(from);
        const s32 ret =
            ::sceNetRecvfrom(socket, buf.data(), buf.size(), Net::ORBIS_NET_MSG_DONTWAIT,
                             reinterpret_cast<OrbisNetSockaddr*>(&from), &from_size);
        if (ret < 0) {
            if (ret != ORBIS_NET_ERROR_EAGAIN) {
                LOG_DEBUG(Lib_NpMatching2, "signaling responder receive failed, {:#x}",
                          static_cast<u32>(ret));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
            continue;
        }
        HandleShadNetPacket(socket, {buf.data(), static_cast<u64>(ret)}, from);
    }
}

void FakeSignaling::HandleShadNetPacket(s32 socket, std::span<const u8> data,
                                        OrbisNetSockaddrIn from) {
    // shadNet answers STUN pings sent from here.
    if (FakeStun::Instance().OnShadNetEcho(data, from)) {
        return;
    }
    if (data.size() <= ShadNetMagic.size() ||
        std::memcmp(data.data(), ShadNetMagic.data(), ShadNetMagic.size()) != 0) {
        return;
    }
    {
        // Peers signal from their own port, only their address has to match.
        std::scoped_lock lk{m_mutex};
        const bool known = std::ranges::any_of(
            m_peers, [&](const auto& entry) { return entry.second.addr == from.sin_addr; });
        if (!known) {
            LOG_DEBUG(Lib_NpMatching2, "shadnet signaling from unknown address {:#x}",
                      from.sin_addr);
            return;
        }
    }

    std::vector<u8> reply;
    const auto set_reply = [&](const auto& packet) {
        const auto* bytes = reinterpret_cast<const u8*>(&packet);
        reply.assign(bytes, bytes + sizeof(packet));
    };

    const auto type = static_cast<ShadNetPacketType>(data[ShadNetMagic.size()]);
    if (type == ShadNetPacketType::EchoPing && data.size() == sizeof(ShadNetEcho)) {
        ShadNetEcho ping{};
        std::memcpy(&ping, data.data(), sizeof(ping));
        ShadNetEcho pong = ping;
        pong.type = ShadNetPacketType::EchoPong;
        set_reply(pong);
    } else if (type == ShadNetPacketType::Handshake && data.size() == sizeof(ShadNetHandshake)) {
        ShadNetHandshake request{};
        std::memcpy(&request, data.data(), sizeof(request));
        LOG_INFO(Lib_NpMatching2, "shadnet handshake {} from {} to {}",
                 static_cast<u8>(request.kind), OnlineIdString(request.online_id_from),
                 OnlineIdString(request.online_id_to));

        ShadNetHandshake answer = request;
        answer.online_id_from = request.online_id_to;
        answer.online_id_to = request.online_id_from;
        answer.from_connection = request.to_connection;
        answer.to_connection = request.from_connection;
        answer.mapped_addr = 0;
        answer.mapped_port = 0;
        switch (request.kind) {
        case HandshakeKind::Offer:
            answer.kind = HandshakeKind::Accept;
            answer.nonce = 0;
            break;
        case HandshakeKind::Check:
            answer.kind = HandshakeKind::CheckAck;
            break;
        default:
            return;
        }
        set_reply(answer);
    } else if (type == ShadNetPacketType::Control && data.size() == sizeof(ShadNetControl)) {
        ShadNetControl request{};
        std::memcpy(&request, data.data(), sizeof(request));
        LOG_INFO(Lib_NpMatching2, "shadnet signaling control {} from {} to {}",
                 static_cast<u8>(request.kind), OnlineIdString(request.online_id_from),
                 OnlineIdString(request.online_id_to));
        if (request.kind != ControlKind::ActivationRequest) {
            return;
        }
        ShadNetControl answer = request;
        answer.kind = ControlKind::ActivationAck;
        answer.online_id_from = request.online_id_to;
        answer.online_id_to = request.online_id_from;
        answer.from_connection = 0;
        answer.mapped_addr = 0;
        answer.mapped_port = 0;
        answer.reason = 0;
        set_reply(answer);
    } else {
        LOG_DEBUG(Lib_NpMatching2, "unhandled shadnet signaling packet: {}", HexDump(data));
        return;
    }

    from.sin_vport = sceNetHtons(ShadNetVport);
    const s32 ret = ::sceNetSendto(socket, reply.data(), reply.size(), 0,
                                   reinterpret_cast<const OrbisNetSockaddr*>(&from), sizeof(from));
    if (ret < 0) {
        LOG_WARNING(Lib_NpMatching2, "could not answer shadnet signaling, {:#x}",
                    static_cast<u32>(ret));
    }
}

} // namespace Libraries::Network
