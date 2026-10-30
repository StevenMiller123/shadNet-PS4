// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/Net.h>

#include "common/logging/log.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_matching2/np_matching2_internal.h"
#include "core/libraries/np/np_matching2/np_matching2_signaling.h"
#include "core/libraries/np/np_signaling/np_signaling_handler.h"
#include "core/libraries/np/np_signaling/np_signaling_state.h"
#include "core/libraries/np/np_signaling/np_signaling_transport.h"
#include "core/net_util/net_util.h"

namespace Libraries::Np::NpSignaling {

constexpr s32 kMatching2ConnInactive = 0;
constexpr s32 kMatching2ConnPending = 1;
constexpr s32 kMatching2ConnActive = 2;
constexpr auto kMatching2StepInterval = std::chrono::milliseconds(500);
constexpr auto kMatching2Timeout = std::chrono::seconds(30);
constexpr auto kMatching2EstablishDelay = std::chrono::milliseconds(1000);
constexpr auto kStunPingInterval = std::chrono::seconds(5);

void HandleControlPacket(u32 from_addr, u16 from_port, const SignalingControl& pkt);
void HandleHandshakePacket(u32 from_addr, u16 from_port, const SignalingHandshake& pkt);
std::string PacketOnlineIdToString(const u8* online_id);

bool ShouldConnectToPeer(const NpMatching2::RoomCache& room,
                         NpMatching2::OrbisNpMatching2RoomMemberId self,
                         NpMatching2::OrbisNpMatching2RoomMemberId peer) {
    if (peer == 0 || peer == self) {
        return false;
    }
    if (room.signaling_type == NpMatching2::ORBIS_NP_MATCHING2_SIGNALING_TYPE_NONE) {
        return false;
    }
    if (room.signaling_type == NpMatching2::ORBIS_NP_MATCHING2_SIGNALING_TYPE_STAR &&
        room.signaling_main_member != 0) {
        return self == room.signaling_main_member || peer == room.signaling_main_member;
    }
    return true;
}

bool ResolvePeerEndpoint(const NpMatching2::MemberCache& member, u32* out_addr, u16* out_port) {
    if (member.addr != 0 && member.port != 0) {
        *out_addr = member.addr;
        *out_port = member.port;
        return true;
    }

    const std::string online_id = OnlineIdToString(member.np_id.handle);
    return !online_id.empty() && ResolvePeer(online_id, out_addr, out_port) && *out_addr != 0 &&
           *out_port != 0;
}

bool HasSignalingMagic(const u8* buf, u64 nbytes) {
    return nbytes >= sizeof(kSignalingMagic) + 1 &&
           std::memcmp(buf, kSignalingMagic, sizeof(kSignalingMagic)) == 0;
}

void HandleStunEcho(const StunEcho& echo) {
    {
        SignalingMutexGuard lock;
        for (auto& [ctx_id, ctx] : GetState()->contexts) {
            if (!ctx.active) {
                continue;
            }
            ctx.ext_addr.store(echo.ext_ip);
            ctx.ext_port.store(echo.ext_port);
            ctx.stun_cv.notify_all();
            LOG_DEBUG(Lib_NpSignaling, "ctx_id={} ext_addr={:#x} ext_port={}", ctx_id, echo.ext_ip,
                      sceNetNtohs(echo.ext_port));
        }
    }

    auto* netinfo = Common::Singleton<NetUtil::NetUtilInternal>::Instance();
    netinfo->SetExternalIp(echo.ext_ip);
}

void ProcessSignalingPacket(const void* data, u32 len, u32 from_addr, u16 from_port) {
    const auto* buf = static_cast<const u8*>(data);
    const auto nbytes = static_cast<u64>(len);

    if (nbytes == sizeof(StunEcho)) {
        StunEcho echo{};
        std::memcpy(&echo, buf, sizeof(echo));
        HandleStunEcho(echo);
        return;
    }

    if (!HasSignalingMagic(buf, nbytes)) {
        return;
    }

    const u8 type = buf[4];
    if (nbytes == sizeof(SignalingControl) &&
        type == static_cast<u8>(SignalingPacketType::Control)) {
        SignalingControl ctrl{};
        std::memcpy(&ctrl, buf, sizeof(ctrl));
        HandleControlPacket(from_addr, from_port, ctrl);
        return;
    }

    if (nbytes == sizeof(SignalingEchoPing) &&
        type == static_cast<u8>(SignalingPacketType::EchoPing)) {
        SignalingEchoPing ping{};
        std::memcpy(&ping, buf, sizeof(ping));
        SignalingEchoPong pong{};
        pong.conn_id = ping.conn_id;
        pong.orig_ts_us = ping.send_ts_us;
        SignalingSendTo(&pong, sizeof(pong), from_addr, from_port);
        return;
    }

    if (nbytes == sizeof(SignalingEchoPong) &&
        type == static_cast<u8>(SignalingPacketType::EchoPong)) {
        SignalingEchoPong pong{};
        std::memcpy(&pong, buf, sizeof(pong));
        s32 rtt_us = static_cast<s32>(NowUs() - static_cast<s64>(pong.orig_ts_us));
        if (rtt_us < 0) {
            rtt_us = 0;
        }
        SignalingMutexGuard lock;
        const auto transport_it = GetState()->peer_transports.find(static_cast<s32>(pong.conn_id));
        if (transport_it != GetState()->peer_transports.end()) {
            auto& transport = transport_it->second;
            const u32 idx = transport.probe_sample_write_index % PeerTransport::kProbeSampleCount;
            transport.probe_rtt_samples[idx] = rtt_us;
            transport.probe_sample_write_index = idx + 1;
        }
        return;
    }

    if (nbytes == sizeof(SignalingHandshake) &&
        type == static_cast<u8>(SignalingPacketType::Handshake)) {
        SignalingHandshake hs{};
        std::memcpy(&hs, buf, sizeof(hs));
        HandleHandshakePacket(from_addr, from_port, hs);
        return;
    }
}

void DrainSignalingPackets() {
    static constexpr u32 kBufSize = 256;
    u8 buf[kBufSize];
    for (;;) {
        u32 from_addr = 0;
        u16 from_port = 0;
        const s32 rc = SignalingRecvFrom(buf, kBufSize, &from_addr, &from_port);
        if (rc <= 0) {
            break;
        }

        ProcessSignalingPacket(buf, static_cast<u32>(rc), from_addr, from_port);
    }
}

void DrainControlPackets() {
    static constexpr u32 kBufSize = 256;
    u8 buf[kBufSize];
    for (;;) {
        u32 from_addr = 0;
        u16 from_port = 0;
        const s32 rc = ControlRecvFrom(buf, kBufSize, &from_addr, &from_port);
        if (rc <= 0) {
            break;
        }

        ProcessSignalingPacket(buf, static_cast<u32>(rc), from_addr, from_port);
    }
}

bool PublishSignalingInfo(const OrbisNpOnlineId& online_id) {
    if (!Matching2Enabled() || online_id.data[0] == '\0') {
        return false;
    }

    const u32 server_addr = MmServerAddr();
    const u16 server_udp = MmServerUdpPort();
    if (server_addr == 0 || server_udp == 0) {
        return false;
    }

    StunPing ping{};
    std::memcpy(ping.online_id, online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH);
    ping.local_ip = AdvertisedAddr();

    const s32 rc = SignalingSendTo(&ping, sizeof(ping), server_addr, server_udp);
    LOG_DEBUG(Lib_NpSignaling, "id='{}' server={:#x}:{} local_ip={:#x} rc={}",
              OnlineIdToString(online_id), server_addr, sceNetNtohs(server_udp), ping.local_ip, rc);
    return rc >= 0;
}

u32 PublishSignalingInfo() {
    std::vector<OrbisNpOnlineId> online_ids;
    {
        SignalingMutexGuard lock;
        for (const auto& [ctx_id, ctx] : GetState()->contexts) {
            if (ctx.active) {
                online_ids.push_back(ctx.owner_online_id);
            }
        }
    }

    for (u32 id = 1; id <= NpMatching2::ContextManager::kMaxContexts; ++id) {
        if (auto ctx = NpMatching2::GetContextManager()->Get(
                static_cast<NpMatching2::OrbisNpMatching2ContextId>(id));
            ctx && ctx->started) {
            online_ids.push_back(ctx->online_id);
        }
    }

    std::vector<std::string> sent;
    u32 sent_count = 0;
    for (const OrbisNpOnlineId& online_id : online_ids) {
        const std::string name = OnlineIdToString(online_id);
        if (name.empty() || std::find(sent.begin(), sent.end(), name) != sent.end()) {
            continue;
        }
        sent.push_back(name);
        if (PublishSignalingInfo(online_id)) {
            ++sent_count;
        }
    }
    return sent_count;
}

void SendEchoPings() {
    constexpr s64 kEchoIntervalUs = 1'000'000;

    struct PingTarget {
        s32 transport_id;
        u32 addr;
        u16 port;
        s64 now_us;
    };

    std::vector<PingTarget> targets;
    {
        SignalingMutexGuard lock;
        const s64 now = NowUs();
        for (auto& [transport_id, transport] : GetState()->peer_transports) {
            if (transport.status != ORBIS_NP_SIGNALING_CONN_STATUS_ACTIVE || transport.addr == 0 ||
                transport.port == 0) {
                continue;
            }
            if (transport.last_echo_ping_us != 0 &&
                now - transport.last_echo_ping_us < kEchoIntervalUs) {
                continue;
            }
            transport.last_echo_ping_us = now;
            targets.push_back({transport_id, transport.addr, transport.port, now});
        }
    }

    for (const PingTarget& target : targets) {
        SignalingEchoPing ping{};
        ping.conn_id = static_cast<u32>(target.transport_id);
        ping.send_ts_us = static_cast<u64>(target.now_us);
        SignalingSendTo(&ping, sizeof(ping), target.addr, target.port);
    }
}

u32 AverageRtt(const PeerTransport& transport) {
    u64 total = 0;
    u32 count = 0;
    for (const s32 sample : transport.probe_rtt_samples) {
        if (sample > 0) {
            total += static_cast<u32>(sample);
            ++count;
        }
    }
    return count == 0 ? 0 : static_cast<u32>(total / count);
}

s32 StatusFromState(ConnState state) {
    if (state == ConnState::Established) {
        return ORBIS_NP_SIGNALING_CONN_STATUS_ACTIVE;
    }
    if (state == ConnState::Inactive) {
        return ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE;
    }
    return ORBIS_NP_SIGNALING_CONN_STATUS_PENDING;
}

void SetTransportStateLocked(PeerTransport& transport, ConnState new_state) {
    transport.state = new_state;
    transport.status = StatusFromState(new_state);
}

s32 AllocateTransportIdLocked() {
    if (GetState()->transport_id_seed == 0) {
        GetState()->transport_id_seed = 1;
    }
    for (s32 tried = 0; tried <= 0xffff; ++tried) {
        const u32 candidate = GetState()->transport_id_seed++;
        if (GetState()->transport_id_seed == 0 || GetState()->transport_id_seed > 0x7fff) {
            GetState()->transport_id_seed = 1;
        }
        if (candidate != 0 && GetState()->peer_transports.find(static_cast<s32>(candidate)) ==
                                  GetState()->peer_transports.end()) {
            return static_cast<s32>(candidate);
        }
    }
    return -1;
}

bool TransportMatchesOnlineId(const PeerTransport& transport, std::string_view online_id) {
    return transport.state != ConnState::Inactive &&
           OnlineIdEqualsString(transport.online_id, online_id);
}

s32 FindTransportByOnlineIdLocked(std::string_view online_id) {
    for (const auto& [transport_id, transport] : GetState()->peer_transports) {
        if (TransportMatchesOnlineId(transport, online_id)) {
            return transport_id;
        }
    }
    return 0;
}

PeerTransport* GetTransportLocked(s32 transport_id) {
    const auto it = GetState()->peer_transports.find(transport_id);
    return it == GetState()->peer_transports.end() ? nullptr : &it->second;
}

bool HasSig1ConnectionForTransportLocked(s32 transport_id) {
    for (const auto& [conn_id, ci] : GetState()->connections) {
        if (ci.transport_id == transport_id &&
            ci.status != ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE && !ci.dead_fired) {
            return true;
        }
    }
    return false;
}

void SyncSig1StatusLocked(ConnectionInfo& ci, const PeerTransport& transport) {
    ci.status = transport.status;
}

void SyncSig1ConnectionsForTransportLocked(s32 transport_id) {
    const auto transport_it = GetState()->peer_transports.find(transport_id);
    if (transport_it == GetState()->peer_transports.end()) {
        return;
    }
    for (auto& [conn_id, ci] : GetState()->connections) {
        if (ci.transport_id == transport_id) {
            SyncSig1StatusLocked(ci, transport_it->second);
        }
    }
}

s32 CreateTransportLocked(const OrbisNpOnlineId& local_online_id,
                          const OrbisNpOnlineId& peer_online_id, const OrbisNpId& peer_npid,
                          u32 addr, u16 port) {
    const s32 transport_id = AllocateTransportIdLocked();
    if (transport_id < 0) {
        return transport_id;
    }
    PeerTransport transport{};
    transport.transport_id = transport_id;
    transport.local_online_id = local_online_id;
    transport.online_id = peer_online_id;
    transport.npid = peer_npid;
    transport.addr = addr;
    transport.port = port;
    transport.status = ORBIS_NP_SIGNALING_CONN_STATUS_PENDING;
    transport.state = ConnState::SendingOffer;
    transport.is_initiator = true;
    GetState()->peer_transports[transport_id] = transport;
    return transport_id;
}

s32 EnsureTransportLocked(const OrbisNpOnlineId& local_online_id,
                          const OrbisNpOnlineId& peer_online_id, const OrbisNpId& peer_npid,
                          u32 addr, u16 port) {
    const std::string peer_id = OnlineIdToString(peer_online_id);
    s32 transport_id = FindTransportByOnlineIdLocked(peer_id);
    if (transport_id == 0) {
        transport_id =
            CreateTransportLocked(local_online_id, peer_online_id, peer_npid, addr, port);
    } else {
        auto& transport = GetState()->peer_transports[transport_id];
        if (transport.local_online_id.data[0] == '\0') {
            transport.local_online_id = local_online_id;
        }
        if (transport.online_id.data[0] == '\0') {
            transport.online_id = peer_online_id;
            transport.npid = peer_npid;
        }
        if (addr != 0 && port != 0) {
            transport.addr = addr;
            transport.port = port;
        }
        if (transport.state == ConnState::Inactive) {
            SetTransportStateLocked(transport, ConnState::SendingOffer);
            transport.is_initiator = true;
        }
    }
    return transport_id;
}

std::string PacketOnlineIdToString(const u8* online_id) {
    char buf[ORBIS_NP_ONLINEID_MAX_LENGTH + 1]{};
    std::memcpy(buf, online_id, ORBIS_NP_ONLINEID_MAX_LENGTH);
    return std::string(buf);
}

bool ConnectionMatchesOnlineId(const ConnectionInfo& ci, std::string_view online_id) {
    return !ci.dead_fired && ci.status != ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE &&
           OnlineIdEqualsString(ci.online_id, online_id);
}

s32 FindConnectionByOnlineIdLocked(std::string_view online_id, s32 preferred_ctx_id = 0) {
    if (preferred_ctx_id != 0) {
        for (const auto& [conn_id, ci] : GetState()->connections) {
            if (ci.ctx_id == preferred_ctx_id && ConnectionMatchesOnlineId(ci, online_id)) {
                return conn_id;
            }
        }
    }
    for (const auto& [conn_id, ci] : GetState()->connections) {
        if (ConnectionMatchesOnlineId(ci, online_id)) {
            return conn_id;
        }
    }
    return 0;
}

SignalingHandshake MakeHandshakeLocked(const PeerTransport& transport, HandshakeKind kind) {
    SignalingHandshake pkt{};
    pkt.kind = static_cast<u8>(kind);
    pkt.from_conn_id = static_cast<u32>(transport.transport_id);
    std::memcpy(pkt.online_id_from, transport.local_online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH);
    std::memcpy(pkt.online_id_to, transport.online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH);
    auto* netinfo = Common::Singleton<NetUtil::NetUtilInternal>::Instance();
    pkt.mapped_addr = netinfo->GetExternalIp();
    pkt.mapped_port = 0;
    return pkt;
}

void SendHandshakeLocked(PeerTransport& transport, const SignalingHandshake& pkt) {
    if (transport.addr == 0 || transport.port == 0) {
        LOG_DEBUG(Lib_NpSignaling, "skipped handshake, transport={} kind={} endpoint={:#x}:{}",
                  transport.transport_id, pkt.kind, transport.addr, sceNetNtohs(transport.port));
        return;
    }
    transport.last_handshake_send_us = NowUs();
    const s32 rc = SignalingSendTo(&pkt, sizeof(pkt), transport.addr, transport.port);
    LOG_DEBUG(Lib_NpSignaling, "transport={} kind={} endpoint={:#x}:{} rc={}",
              transport.transport_id, pkt.kind, transport.addr, sceNetNtohs(transport.port), rc);
    SyncSig1ConnectionsForTransportLocked(transport.transport_id);
}

SignalingControl MakeControlLocked(const ConnectionInfo& ci, ControlKind kind) {
    const auto* transport = GetTransportLocked(ci.transport_id);
    if (transport) {
        SignalingControl pkt{};
        pkt.kind = static_cast<u8>(kind);
        pkt.from_conn_id = static_cast<u32>(transport->transport_id);
        std::memcpy(pkt.online_id_from, transport->local_online_id.data,
                    ORBIS_NP_ONLINEID_MAX_LENGTH);
        std::memcpy(pkt.online_id_to, transport->online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH);
        auto* netinfo = Common::Singleton<NetUtil::NetUtilInternal>::Instance();
        pkt.mapped_addr = netinfo->GetExternalIp();
        pkt.mapped_port = 0;
        return pkt;
    }
    SignalingControl pkt{};
    return pkt;
}

SignalingControl MakeControlLocked(const PeerTransport& transport, ControlKind kind) {
    SignalingControl pkt{};
    pkt.kind = static_cast<u8>(kind);
    pkt.from_conn_id = static_cast<u32>(transport.transport_id);
    std::memcpy(pkt.online_id_from, transport.local_online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH);
    std::memcpy(pkt.online_id_to, transport.online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH);
    auto* netinfo = Common::Singleton<NetUtil::NetUtilInternal>::Instance();
    pkt.mapped_addr = netinfo->GetExternalIp();
    pkt.mapped_port = 0;
    return pkt;
}

void SendActivate(const ConnectionInfo& ci) {
    const auto* transport = GetTransportLocked(ci.transport_id);
    if (!transport || transport->addr == 0 || transport->port == 0) {
        LOG_DEBUG(Lib_NpSignaling, "skipped activate, conn={} transport={} endpoint unavailable",
                  ci.conn_id, ci.transport_id);
        return;
    }
    const SignalingControl pkt = MakeControlLocked(ci, ControlKind::ActivationRequest);
    const s32 rc = ControlSendTo(&pkt, sizeof(pkt), transport->addr, transport->port);
    LOG_DEBUG(Lib_NpSignaling, "conn={} transport={} peer='{}' endpoint={:#x}:{} rc={}", ci.conn_id,
              ci.transport_id, OnlineIdToString(ci.online_id), transport->addr,
              sceNetNtohs(transport->port), rc);
}

void SendControlCloseLocked(const ConnectionInfo& ci, ControlReason reason) {
    const auto* transport = GetTransportLocked(ci.transport_id);
    if (!transport || transport->addr == 0 || transport->port == 0) {
        return;
    }
    SignalingControl pkt = MakeControlLocked(ci, ControlKind::Close);
    pkt.reason = static_cast<u16>(reason);
    ControlSendTo(&pkt, sizeof(pkt), transport->addr, transport->port);
}

void QueueActivationLocked(OrbisNpSignalingConnectionId conn_id, std::string_view peer_online_id,
                           bool start_handshake) {
    GetState()->pending_activations.push_back(
        {conn_id, std::string(peer_online_id), start_handshake});
    GetState()->activation_cv.notify_one();
}

void StartTransportHandshakeInitiator(s32 transport_id) {
    SignalingHandshake pkt{};
    HandshakeKind kind = HandshakeKind::Offer;
    {
        SignalingMutexGuard lock;
        auto* transport = GetTransportLocked(transport_id);
        if (!transport) {
            return;
        }
        transport->is_initiator = true;
        switch (transport->state) {
        case ConnState::SendingAccept:
            kind = HandshakeKind::Accept;
            break;
        case ConnState::WaitOffer:
            SetTransportStateLocked(*transport, ConnState::SendingOffer);
            kind = HandshakeKind::Offer;
            break;
        case ConnState::ConnCheck:
            kind = HandshakeKind::Check;
            break;
        case ConnState::SendingOffer:
        case ConnState::WaitAccept:
            kind = HandshakeKind::Offer;
            break;
        case ConnState::Established:
            return;
        case ConnState::Inactive:
        default:
            SetTransportStateLocked(*transport, ConnState::SendingOffer);
            kind = HandshakeKind::Offer;
            break;
        }
        pkt = MakeHandshakeLocked(*transport, kind);
        if (kind == HandshakeKind::Check) {
            pkt.nonce = static_cast<u64>(NowUs());
        }
        SendHandshakeLocked(*transport, pkt);
    }
}

void QueueMatching2DeadForTransport(s32 transport_id, s32 error_code) {
    for (u32 id = 1; id <= NpMatching2::ContextManager::kMaxContexts; ++id) {
        auto ctx = NpMatching2::GetContextManager()->Get(
            static_cast<NpMatching2::OrbisNpMatching2ContextId>(id));
        if (!ctx) {
            continue;
        }
        auto cache = NpMatching2::LockCache(ctx->ctx_id);
        for (auto& [member_id, peer] : cache->peers) {
            if (peer.conn_id != transport_id) {
                continue;
            }
            const NpMatching2::OrbisNpMatching2RoomId room_id = [&]() {
                for (const auto& [rid, room] : cache->rooms) {
                    if (room.members.find(member_id) != room.members.end()) {
                        return rid;
                    }
                }
                return static_cast<NpMatching2::OrbisNpMatching2RoomId>(0);
            }();
            peer.status = kMatching2ConnInactive;
            peer.sent_established = false;
            peer.active_since_us = 0;
            if (room_id != 0) {
                NpMatching2::QueueMatching2SignalingEvent(
                    *ctx, room_id, member_id, NpMatching2::ORBIS_NP_MATCHING2_SIGNALING_EVENT_DEAD,
                    error_code);
            }
        }
    }
}

void CloseConnectionAndDispatchDead(s32 conn_id, s32 sig_error_code, s32 matching2_error_code) {
    bool fire_sig1 = false;
    s32 transport_id = 0;
    {
        SignalingMutexGuard lock;
        const auto it = GetState()->connections.find(conn_id);
        if (it == GetState()->connections.end()) {
            return;
        }
        ConnectionInfo& ci = it->second;
        if (ci.status == ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE || ci.dead_fired) {
            return;
        }
        transport_id = ci.transport_id;
        if (auto* transport = GetTransportLocked(transport_id)) {
            SetTransportStateLocked(*transport, ConnState::Inactive);
        }
        ci.status = ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE;
        ci.dead_fired = true;
        fire_sig1 = ci.ctx_id != 0;
    }

    if (transport_id != 0) {
        QueueMatching2DeadForTransport(transport_id, matching2_error_code);
    }
    if (fire_sig1) {
        DispatchConnectionEvent(conn_id, ORBIS_NP_SIGNALING_EVENT_DEAD, sig_error_code);
    }
    {
        SignalingMutexGuard lock;
        RemoveConnectionLocked(conn_id);
    }
}

void CloseTransportAndDispatchDead(s32 transport_id, s32 sig_error_code, s32 matching2_error_code) {
    std::vector<s32> sig1_conns;
    {
        SignalingMutexGuard lock;
        auto* transport = GetTransportLocked(transport_id);
        if (!transport || transport->state == ConnState::Inactive) {
            return;
        }
        SetTransportStateLocked(*transport, ConnState::Inactive);
        for (auto& [conn_id, ci] : GetState()->connections) {
            if (ci.transport_id != transport_id || ci.dead_fired) {
                continue;
            }
            ci.dead_fired = true;
            ci.status = ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE;
            if (ci.ctx_id != 0) {
                sig1_conns.push_back(conn_id);
            }
        }
    }

    QueueMatching2DeadForTransport(transport_id, matching2_error_code);
    for (const s32 conn_id : sig1_conns) {
        DispatchConnectionEvent(conn_id, ORBIS_NP_SIGNALING_EVENT_DEAD, sig_error_code);
    }
    {
        SignalingMutexGuard lock;
        for (const s32 conn_id : sig1_conns) {
            RemoveConnectionLocked(conn_id);
        }
        GetState()->peer_transports.erase(transport_id);
    }
}

void EstablishConnection(s32 transport_id, bool peer_activated_hint) {
    SignalingControl established_pkt{};
    bool send_established = false;
    u32 peer_addr = 0;
    u16 peer_port = 0;
    std::vector<s32> sig1_peer_activated;
    std::vector<s32> sig1_established;
    std::vector<s32> sig1_mutual;
    {
        SignalingMutexGuard lock;
        auto* transport = GetTransportLocked(transport_id);
        if (!transport) {
            return;
        }
        if (peer_activated_hint) {
            transport->peer_activated = true;
        }

        if (!transport->transport_established_fired) {
            SetTransportStateLocked(*transport, ConnState::Established);
            transport->transport_established_fired = true;
            transport->last_peer_rx_us = NowUs();
            if (transport->addr != 0 && transport->port != 0) {
                established_pkt = MakeControlLocked(*transport, ControlKind::Established);
                send_established = true;
                peer_addr = transport->addr;
                peer_port = transport->port;
            }
        }

        for (auto& [conn_id, ci] : GetState()->connections) {
            if (ci.transport_id != transport_id) {
                continue;
            }
            SyncSig1StatusLocked(ci, *transport);

            if (ci.peer_activated && !ci.locally_activated && !ci.peer_activated_fired) {
                ci.peer_activated_fired = true;
                sig1_peer_activated.push_back(conn_id);
            }
            if (ci.ctx_id != 0 && ci.locally_activated &&
                ci.status == ORBIS_NP_SIGNALING_CONN_STATUS_ACTIVE &&
                !ci.sig1_established_event_fired) {
                ci.sig1_established_event_fired = true;
                sig1_established.push_back(conn_id);
            }
            if (ci.ctx_id != 0 && ci.locally_activated &&
                ci.status == ORBIS_NP_SIGNALING_CONN_STATUS_ACTIVE && ci.peer_activated &&
                !ci.mutual_fired) {
                ci.mutual_fired = true;
                sig1_mutual.push_back(conn_id);
            }
        }
    }

    if (send_established) {
        ControlSendTo(&established_pkt, sizeof(established_pkt), peer_addr, peer_port);
    }
    for (const s32 conn_id : sig1_peer_activated) {
        DispatchPeerActivatedEvent(conn_id);
    }
    for (const s32 conn_id : sig1_established) {
        DispatchConnectionEvent(conn_id, ORBIS_NP_SIGNALING_EVENT_ESTABLISHED, 0);
    }
    for (const s32 conn_id : sig1_mutual) {
        DispatchConnectionEvent(conn_id, ORBIS_NP_SIGNALING_EVENT_MUTUAL_ACTIVATED, 0);
    }
}

void ProcessPendingActivations() {
    std::vector<PendingActivation> work;
    std::vector<PendingActivation> retry;
    {
        SignalingMutexGuard lock;
        if (GetState()->pending_activations.empty()) {
            return;
        }
        work.swap(GetState()->pending_activations);
    }

    for (const PendingActivation& act : work) {
        s32 transport_id = 0;
        {
            SignalingMutexGuard lock;
            const auto it = GetState()->connections.find(act.conn_id);
            if (it == GetState()->connections.end() ||
                it->second.status == ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE) {
                continue;
            }
            transport_id = it->second.transport_id;
        }

        u32 peer_addr = 0;
        u16 peer_port = 0;
        const bool resolved = ResolvePeer(act.peer_online_id, &peer_addr, &peer_port) &&
                              peer_addr != 0 && peer_port != 0;

        if (!resolved) {
            LOG_DEBUG(Lib_NpSignaling, "conn={} peer='{}' resolve failed", act.conn_id,
                      act.peer_online_id);
            retry.push_back(act);
            continue;
        }

        bool already_established = false;
        {
            SignalingMutexGuard lock;
            const auto it = GetState()->connections.find(act.conn_id);
            if (it == GetState()->connections.end() ||
                it->second.status == ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE) {
                continue;
            }
            auto* transport = GetTransportLocked(it->second.transport_id);
            if (!transport) {
                continue;
            }
            transport->addr = peer_addr;
            transport->port = peer_port;
            SyncSig1StatusLocked(it->second, *transport);
            LOG_DEBUG(Lib_NpSignaling,
                      "resolved conn={} transport={} peer='{}' "
                      "endpoint={:#x}:{} start_handshake={}",
                      act.conn_id, it->second.transport_id, act.peer_online_id, peer_addr,
                      sceNetNtohs(peer_port), act.start_handshake);
            SendActivate(it->second);
            already_established = transport->state == ConnState::Established;
        }

        if (act.start_handshake) {
            StartTransportHandshakeInitiator(transport_id);
        } else if (already_established) {
            EstablishConnection(transport_id, false);
        }
    }

    if (!retry.empty()) {
        SignalingMutexGuard lock;
        GetState()->pending_activations.insert(GetState()->pending_activations.end(), retry.begin(),
                                               retry.end());
    }
}

void HandleHandshakePacket(u32 from_addr, u16 from_port, const SignalingHandshake& pkt) {
    const std::string from_id = PacketOnlineIdToString(pkt.online_id_from);
    const HandshakeKind kind = static_cast<HandshakeKind>(pkt.kind);

    s32 establish_transport = 0;
    SignalingHandshake reply{};
    bool send_reply = false;
    s32 reply_transport = 0;

    {
        SignalingMutexGuard lock;

        const s32 transport_id = FindTransportByOnlineIdLocked(from_id);
        if (transport_id != 0) {
            GetState()->peer_transports[transport_id].last_peer_rx_us = NowUs();
        }

        if (kind == HandshakeKind::Offer) {
            if (transport_id == 0) {
                return;
            }
            auto& transport = GetState()->peer_transports[transport_id];
            transport.addr = pkt.mapped_addr != 0 ? pkt.mapped_addr : from_addr;
            transport.port = from_port;
            transport.peer_activated = true;
            if (transport.state != ConnState::Established) {
                SetTransportStateLocked(transport, ConnState::SendingAccept);
            }
            reply = MakeHandshakeLocked(transport, HandshakeKind::Accept);
            send_reply = true;
            reply_transport = transport_id;
        } else if (transport_id == 0) {
            return;
        } else if (kind == HandshakeKind::Accept) {
            auto& transport = GetState()->peer_transports[transport_id];
            transport.peer_activated = true;
            transport.addr = pkt.mapped_addr != 0 ? pkt.mapped_addr : from_addr;
            transport.port = from_port;
            if (transport.state == ConnState::SendingOffer ||
                transport.state == ConnState::WaitAccept ||
                transport.state == ConnState::SendingAccept ||
                transport.state == ConnState::WaitOffer) {
                SetTransportStateLocked(transport, ConnState::ConnCheck);
                reply = MakeHandshakeLocked(transport, HandshakeKind::Check);
                reply.nonce = static_cast<u64>(NowUs());
                send_reply = true;
                reply_transport = transport_id;
            }
        } else if (kind == HandshakeKind::Check) {
            auto& transport = GetState()->peer_transports[transport_id];
            reply = MakeHandshakeLocked(transport, HandshakeKind::CheckAck);
            reply.nonce = pkt.nonce;
            send_reply = true;
            reply_transport = transport_id;
            if (transport.state == ConnState::SendingAccept ||
                transport.state == ConnState::WaitOffer) {
                SetTransportStateLocked(transport, ConnState::ConnCheck);
            }
        } else if (kind == HandshakeKind::CheckAck) {
            const s64 rtt_us = NowUs() - static_cast<s64>(pkt.nonce);
            if (rtt_us >= 0) {
                auto& transport = GetState()->peer_transports[transport_id];
                const u32 idx =
                    transport.probe_sample_write_index % PeerTransport::kProbeSampleCount;
                transport.probe_rtt_samples[idx] = static_cast<s32>(rtt_us);
                transport.probe_sample_write_index = idx + 1;
            }
            if (GetState()->peer_transports[transport_id].state != ConnState::Established) {
                establish_transport = transport_id;
            }
        }
        SyncSig1ConnectionsForTransportLocked(transport_id);
    }

    if (send_reply) {
        SignalingMutexGuard lock;
        auto* transport = GetTransportLocked(reply_transport);
        if (transport) {
            SendHandshakeLocked(*transport, reply);
        }
    }
    if (establish_transport != 0) {
        EstablishConnection(establish_transport, true);
    }
}

void HandleControlPacket(u32 from_addr, u16 from_port, const SignalingControl& pkt) {
    const std::string from_id = PacketOnlineIdToString(pkt.online_id_from);
    const ControlKind kind = static_cast<ControlKind>(pkt.kind);

    s32 establish_transport = 0;
    s32 dead_conn = 0;
    s32 peer_deactivated_conn = 0;
    SignalingControl ack{};
    bool send_ack = false;
    u32 ack_addr = 0;
    u16 ack_port = 0;

    {
        SignalingMutexGuard lock;

        s32 transport_id = FindTransportByOnlineIdLocked(from_id);

        if (kind == ControlKind::ActivationRequest) {
            const std::string to_id = PacketOnlineIdToString(pkt.online_id_to);
            s32 ctx_id = 0;
            for (auto& [cid, ctx] : GetState()->contexts) {
                if (ctx.active && OnlineIdEqualsString(ctx.owner_online_id, to_id)) {
                    ctx_id = cid;
                    break;
                }
            }
            OrbisNpOnlineId peer_online_id{};
            SetNpOnlineId(peer_online_id,
                          std::string_view(reinterpret_cast<const char*>(pkt.online_id_from),
                                           ORBIS_NP_ONLINEID_MAX_LENGTH));
            OrbisNpId peer_npid = NpIdFromOnlineId(peer_online_id);
            OrbisNpOnlineId local_online_id{};
            if (ctx_id != 0) {
                local_online_id = GetState()->contexts[ctx_id].owner_online_id;
            } else {
                SetNpOnlineId(local_online_id,
                              std::string_view(reinterpret_cast<const char*>(pkt.online_id_to),
                                               ORBIS_NP_ONLINEID_MAX_LENGTH));
            }
            if (transport_id == 0) {
                transport_id = EnsureTransportLocked(
                    local_online_id, peer_online_id, peer_npid,
                    pkt.mapped_addr != 0 ? pkt.mapped_addr : from_addr, from_port);
                if (transport_id < 0) {
                    return;
                }
            }
            auto& transport = GetState()->peer_transports[transport_id];
            transport.peer_activated = true;
            transport.addr = pkt.mapped_addr != 0 ? pkt.mapped_addr : from_addr;
            transport.port = from_port;
            if (transport.state == ConnState::Inactive) {
                SetTransportStateLocked(transport, ConnState::WaitOffer);
            }

            s32 conn_id = ctx_id != 0 ? FindConnectionByOnlineIdLocked(from_id, ctx_id) : 0;
            if (ctx_id != 0 && conn_id == 0) {
                conn_id = AllocateConnectionIdLocked();
                if (conn_id < 0) {
                    return;
                }
                ConnectionInfo ci{};
                ci.conn_id = conn_id;
                ci.ctx_id = ctx_id;
                ci.transport_id = transport_id;
                ci.locally_activated = false;
                ci.peer_activated = true;
                ci.online_id = peer_online_id;
                ci.npid = peer_npid;
                SyncSig1StatusLocked(ci, transport);
                GetState()->connections[conn_id] = std::move(ci);
                GetState()
                    ->npid_to_conn[MakeCtxNpIdKey(ctx_id, GetState()->connections[conn_id].npid)] =
                    conn_id;
            }
            if (conn_id != 0) {
                auto& ci = GetState()->connections[conn_id];
                ci.peer_activated = true;
                ci.transport_id = transport_id;
                LOG_DEBUG(
                    Lib_NpSignaling,
                    "activation request, conn={} transport={} peer='{}' state={} local_active={}",
                    conn_id, transport_id, from_id, static_cast<s32>(transport.state),
                    ci.locally_activated);
            }
            SyncSig1ConnectionsForTransportLocked(transport_id);
            if (transport.state == ConnState::Established) {
                establish_transport = transport_id;
            }
            ack = MakeControlLocked(transport, ControlKind::ActivationAck);
            send_ack = true;
            ack_addr = transport.addr;
            ack_port = transport.port;
        } else if (transport_id == 0) {
            return;
        } else if (kind == ControlKind::ActivationAck) {
            auto& transport = GetState()->peer_transports[transport_id];
            transport.peer_activated = true;
            if (pkt.mapped_addr != 0) {
                transport.addr = pkt.mapped_addr;
            }
            const s32 conn_id = FindConnectionByOnlineIdLocked(from_id);
            if (conn_id != 0) {
                const auto& ci = GetState()->connections[conn_id];
                LOG_DEBUG(Lib_NpSignaling,
                          "activation acknowledged, conn={} transport={} peer='{}' state={} "
                          "local_active={}",
                          conn_id, transport_id, from_id, static_cast<s32>(transport.state),
                          ci.locally_activated);
            }
            SyncSig1ConnectionsForTransportLocked(transport_id);
        } else if (kind == ControlKind::Established) {
            auto& transport = GetState()->peer_transports[transport_id];
            transport.peer_established = true;
            if (transport.state == ConnState::Established) {
                establish_transport = transport_id;
            }
        } else if (kind == ControlKind::Close) {
            const s32 conn_id = FindConnectionByOnlineIdLocked(from_id);
            const ControlReason reason = static_cast<ControlReason>(pkt.reason);
            if (reason == ControlReason::Deactivate && conn_id != 0 &&
                GetState()->peer_transports[transport_id].state == ConnState::Established) {
                peer_deactivated_conn = conn_id;
            } else {
                dead_conn = conn_id;
                if (dead_conn == 0) {
                    SetTransportStateLocked(GetState()->peer_transports[transport_id],
                                            ConnState::Inactive);
                    QueueMatching2DeadForTransport(
                        transport_id, ORBIS_NP_MATCHING2_SIGNALING_ERROR_TERMINATED_BY_PEER);
                }
            }
        }
    }

    if (send_ack && ack_addr != 0 && ack_port != 0) {
        ControlSendTo(&ack, sizeof(ack), ack_addr, ack_port);
    }
    if (establish_transport != 0) {
        EstablishConnection(establish_transport, false);
    }
    if (peer_deactivated_conn != 0) {
        DispatchConnectionEvent(peer_deactivated_conn, ORBIS_NP_SIGNALING_EVENT_PEER_DEACTIVATED,
                                0);
    }
    if (dead_conn != 0) {
        CloseConnectionAndDispatchDead(dead_conn, ORBIS_NP_SIGNALING_ERROR_TERMINATED_BY_PEER,
                                       ORBIS_NP_MATCHING2_SIGNALING_ERROR_TERMINATED_BY_PEER);
    }
}

void SyncMatching2Peers() {
    struct Snapshot {
        s32 ctx_id = 0;
        u64 room_id = 0;
        u16 member_id = 0;
        s32 transport_id = 0;
        s32 status = 0;
        u32 addr = 0;
        u16 port = 0;
        u32 rtt = 0;
    };

    std::vector<Snapshot> snapshots;
    {
        SignalingMutexGuard lock;
        for (u32 id = 1; id <= NpMatching2::ContextManager::kMaxContexts; ++id) {
            auto ctx = NpMatching2::GetContextManager()->Get(
                static_cast<NpMatching2::OrbisNpMatching2ContextId>(id));
            if (!ctx || !ctx->signaling_callback) {
                continue;
            }
            auto cache = NpMatching2::LockCache(ctx->ctx_id);
            for (const auto& [member_id, peer] : cache->peers) {
                if (peer.conn_id == 0) {
                    continue;
                }
                const auto transport_it = GetState()->peer_transports.find(peer.conn_id);
                if (transport_it == GetState()->peer_transports.end()) {
                    continue;
                }
                NpMatching2::OrbisNpMatching2RoomId room_id = 0;
                for (const auto& [rid, room] : cache->rooms) {
                    if (room.members.find(member_id) != room.members.end()) {
                        room_id = rid;
                        break;
                    }
                }
                if (room_id == 0) {
                    continue;
                }
                const auto& transport = transport_it->second;
                snapshots.push_back({
                    .ctx_id = ctx->ctx_id,
                    .room_id = room_id,
                    .member_id = member_id,
                    .transport_id = peer.conn_id,
                    .status = transport.status,
                    .addr = transport.addr,
                    .port = transport.port,
                    .rtt = AverageRtt(transport),
                });
            }
        }
    }

    for (const Snapshot& s : snapshots) {
        auto ctx = NpMatching2::GetContextManager()->Get(
            static_cast<NpMatching2::OrbisNpMatching2ContextId>(s.ctx_id));
        if (!ctx || !ctx->signaling_callback) {
            continue;
        }
        auto cache = NpMatching2::LockCache(ctx->ctx_id);
        auto& peer =
            cache->peers[static_cast<NpMatching2::OrbisNpMatching2RoomMemberId>(s.member_id)];
        peer.conn_id = s.transport_id;
        peer.member_id = static_cast<NpMatching2::OrbisNpMatching2RoomMemberId>(s.member_id);
        peer.addr = s.addr;
        peer.port = s.port;
        peer.ping_us = s.rtt;
        peer.status = s.status == ORBIS_NP_SIGNALING_CONN_STATUS_ACTIVE    ? kMatching2ConnActive
                      : s.status == ORBIS_NP_SIGNALING_CONN_STATUS_PENDING ? kMatching2ConnPending
                                                                           : kMatching2ConnInactive;
        if (peer.status != kMatching2ConnActive) {
            peer.active_since_us = 0;
        } else if (!peer.sent_established) {
            const s64 now_us = NowUs();
            if (peer.active_since_us == 0) {
                peer.active_since_us = now_us;
            }
            if (now_us - peer.active_since_us <
                std::chrono::duration_cast<std::chrono::microseconds>(kMatching2EstablishDelay)
                    .count()) {
                peer.status = kMatching2ConnPending;
            }
        }
        if (peer.status == kMatching2ConnActive && !peer.sent_established) {
            peer.sent_established = true;
            NpMatching2::QueueMatching2SignalingEvent(
                *ctx, static_cast<NpMatching2::OrbisNpMatching2RoomId>(s.room_id),
                static_cast<NpMatching2::OrbisNpMatching2RoomMemberId>(s.member_id),
                NpMatching2::ORBIS_NP_MATCHING2_SIGNALING_EVENT_ESTABLISHED, ORBIS_OK);
            LOG_INFO(Lib_NpMatching2,
                     "signaling established, ctx={} room={} member={} addr={:#x}:{}", s.ctx_id,
                     s.room_id, s.member_id, s.addr, sceNetNtohs(s.port));
        }
    }
}

void StepPendingConnections() {
    const s64 now = NowUs();
    std::vector<s32> resend;
    std::vector<s32> timeout;

    {
        SignalingMutexGuard lock;
        for (const auto& [transport_id, transport] : GetState()->peer_transports) {
            if (transport.status == ORBIS_NP_SIGNALING_CONN_STATUS_ACTIVE ||
                transport.state == ConnState::Inactive) {
                continue;
            }

            if (transport.last_handshake_send_us != 0 &&
                now - transport.last_handshake_send_us >
                    std::chrono::duration_cast<std::chrono::microseconds>(kMatching2Timeout)
                        .count()) {
                timeout.push_back(transport_id);
                continue;
            }

            if (transport.last_handshake_send_us == 0 ||
                now - transport.last_handshake_send_us >=
                    std::chrono::duration_cast<std::chrono::microseconds>(kMatching2StepInterval)
                        .count()) {
                resend.push_back(transport_id);
            }
        }
    }

    for (const s32 transport_id : resend) {
        StartTransportHandshakeInitiator(transport_id);
    }
    for (const s32 transport_id : timeout) {
        CloseTransportAndDispatchDead(transport_id, ORBIS_NP_SIGNALING_ERROR_TIMEOUT,
                                      ORBIS_NP_MATCHING2_SIGNALING_ERROR_TIMEOUT);
    }
}

void ThreadMain() {
    LOG_DEBUG(Lib_NpSignaling, "Signaling runtime thread started");
    auto* state = GetState();
    auto last_publish = std::chrono::steady_clock::time_point{};
    while (!state->runtime_stop.load(std::memory_order_relaxed)) {
        if (!EnsureTransport()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        const auto now = std::chrono::steady_clock::now();
        DrainControlPackets();
        DrainSignalingPackets();
        StepPendingConnections();
        SendEchoPings();
        SyncMatching2Peers();

        if (last_publish.time_since_epoch().count() == 0 ||
            now - last_publish >= kStunPingInterval) {
            if (PublishSignalingInfo() != 0) {
                last_publish = now;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    LOG_DEBUG(Lib_NpSignaling, "Signaling runtime thread stopped");
}

void ActivationThreadMain() {
    auto* state = GetState();
    while (!state->runtime_stop.load(std::memory_order_relaxed)) {
        {
            std::unique_lock lock(state->activation_mutex);
            state->activation_cv.wait_for(lock, std::chrono::milliseconds(250), [&state] {
                if (state->runtime_stop.load(std::memory_order_relaxed)) {
                    return true;
                }
                SignalingMutexGuard signaling_lock;
                return !state->pending_activations.empty();
            });
        }

        if (state->runtime_stop.load(std::memory_order_relaxed)) {
            break;
        }

        ProcessPendingActivations();
    }
}

void Start() {
    auto* state = GetState();
    std::lock_guard lock(state->runtime_thread_mutex);
    if (state->runtime_thread.joinable()) {
        LOG_DEBUG(Lib_NpSignaling, "refs={}", state->context_refs);
        return;
    }
    state->runtime_stop.store(false, std::memory_order_relaxed);
    LOG_DEBUG(Lib_NpSignaling, "refs={}", state->context_refs);
    state->runtime_thread = std::thread(ThreadMain);
    state->activation_thread = std::thread(ActivationThreadMain);
}

void Stop() {
    auto* state = GetState();
    {
        std::lock_guard lock(state->runtime_thread_mutex);
        if (state->context_refs > 0) {
            return;
        }
        state->runtime_stop.store(true, std::memory_order_relaxed);
        state->activation_cv.notify_all();
    }
    if (state->runtime_thread.joinable()) {
        state->runtime_thread.join();
    }
    if (state->activation_thread.joinable()) {
        state->activation_thread.join();
    }
}

void AddContextRef() {
    auto* state = GetState();
    u32 refs = 0;
    {
        std::lock_guard lock(state->runtime_thread_mutex);
        refs = ++state->context_refs;
    }
    LOG_DEBUG(Lib_NpSignaling, "Signaling runtime ref added refs={}", refs);
    Start();
}

void ReleaseContextRef() {
    auto* state = GetState();
    bool should_stop = false;
    u32 refs = 0;
    {
        std::lock_guard lock(state->runtime_thread_mutex);
        if (state->context_refs > 0) {
            --state->context_refs;
        }
        refs = state->context_refs;
        should_stop = state->context_refs == 0;
    }
    LOG_DEBUG(Lib_NpSignaling, "Signaling runtime ref released refs={}", refs);
    if (should_stop) {
        Stop();
    }
}

bool FindMatching2MemberNpId(OrbisNpAccountId account_id, OrbisNpId* out_npid) {
    for (u32 id = 1; id <= NpMatching2::ContextManager::kMaxContexts; ++id) {
        auto ctx = NpMatching2::GetContextManager()->Get(
            static_cast<NpMatching2::OrbisNpMatching2ContextId>(id));
        if (!ctx) {
            continue;
        }
        auto cache = NpMatching2::LockCache(ctx->ctx_id);
        for (const auto& [room_id, room] : cache->rooms) {
            for (const auto& [member_id, member] : room.members) {
                if (member.account_id == account_id && member.np_id.handle.data[0] != '\0') {
                    *out_npid = member.np_id;
                    return true;
                }
            }
        }
    }
    return false;
}

s32 ActivateSig1(OrbisNpSignalingContextId ctx_id, const OrbisNpId& peer_npid,
                 const OrbisNpOnlineId& peer_online_id, OrbisNpSignalingConnectionId* out_conn_id) {
    if (!out_conn_id) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    const std::string peer_online_id_str = OnlineIdToString(peer_online_id);
    s32 conn_id = 0;
    bool reused_established = false;
    bool queue_activation = false;
    bool start_handshake = true;

    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }

        const auto ctx_it = GetState()->contexts.find(ctx_id);
        if (ctx_it == GetState()->contexts.end() || !ctx_it->second.active) {
            return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
        }
        if (std::memcmp(&ctx_it->second.owner_npid, &peer_npid, sizeof(peer_npid)) == 0) {
            return ORBIS_NP_SIGNALING_ERROR_OWN_NP_ID;
        }
        if (!ConsumeActivationBudgetGatedLocked(ctx_it->second)) {
            return ORBIS_NP_SIGNALING_ERROR_EXCEED_RATE_LIMIT;
        }

        const CtxNpIdKey lookup_key = MakeCtxNpIdKey(ctx_id, peer_npid);
        const auto existing_for_ctx = GetState()->npid_to_conn.find(lookup_key);
        if (existing_for_ctx != GetState()->npid_to_conn.end()) {
            const auto conn_it = GetState()->connections.find(existing_for_ctx->second);
            if (conn_it != GetState()->connections.end() &&
                conn_it->second.status == ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE) {
                RemoveConnectionLocked(existing_for_ctx->second);
            } else if (conn_it != GetState()->connections.end()) {
                conn_id = existing_for_ctx->second;
            }
        }

        bool created_new = false;
        const s32 transport_id =
            EnsureTransportLocked(ctx_it->second.owner_online_id, peer_online_id, peer_npid, 0, 0);
        if (transport_id < 0) {
            return ORBIS_NP_SIGNALING_ERROR_OUT_OF_MEMORY;
        }
        auto& transport = GetState()->peer_transports[transport_id];
        if (conn_id == 0) {
            conn_id = AllocateConnectionIdLocked();
            if (conn_id < 0) {
                return ORBIS_NP_SIGNALING_ERROR_OUT_OF_MEMORY;
            }
            ConnectionInfo ci{};
            ci.conn_id = conn_id;
            ci.ctx_id = ctx_id;
            ci.transport_id = transport_id;
            ci.locally_activated = true;
            ci.npid = peer_npid;
            ci.online_id = peer_online_id;
            SyncSig1StatusLocked(ci, transport);
            if (ci.status == ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE) {
                ci.status = ORBIS_NP_SIGNALING_CONN_STATUS_PENDING;
            }
            GetState()->connections[conn_id] = std::move(ci);
            created_new = true;
        } else {
            ConnectionInfo& ci = GetState()->connections[conn_id];
            const bool was_locally_activated = ci.locally_activated;
            ci.ctx_id = ctx_id;
            ci.transport_id = transport_id;
            ci.locally_activated = true;
            ci.npid = peer_npid;
            ci.online_id = peer_online_id;
            SyncSig1StatusLocked(ci, transport);
            if (transport.state == ConnState::Established) {
                reused_established = true;
                start_handshake = false;
            } else {
                if (transport.state == ConnState::Inactive ||
                    transport.state == ConnState::WaitOffer) {
                    SetTransportStateLocked(transport, ConnState::SendingOffer);
                    SyncSig1StatusLocked(ci, transport);
                }
                start_handshake = true;
            }
            queue_activation = !was_locally_activated;
        }

        GetState()->npid_to_conn[lookup_key] = conn_id;
        if (created_new) {
            queue_activation = true;
            if (transport.state == ConnState::Established) {
                reused_established = true;
                start_handshake = false;
            } else {
                start_handshake = true;
            }
        }
        if (queue_activation) {
            QueueActivationLocked(conn_id, peer_online_id_str, start_handshake);
        }
        LOG_DEBUG(Lib_NpSignaling,
                  "ctx={} conn={} transport={} peer='{}' "
                  "start_handshake={} state={} status={}",
                  ctx_id, conn_id, transport_id, peer_online_id_str, start_handshake,
                  static_cast<s32>(transport.state), transport.status);
    }

    *out_conn_id = conn_id;
    if (reused_established) {
        s32 transport_id = 0;
        {
            SignalingMutexGuard lock;
            const auto it = GetState()->connections.find(conn_id);
            if (it != GetState()->connections.end()) {
                transport_id = it->second.transport_id;
            }
        }
        if (transport_id != 0) {
            EstablishConnection(transport_id, false);
        }
    }
    return ORBIS_OK;
}

void DeactivateSig1(OrbisNpSignalingConnectionId conn_id) {
    {
        SignalingMutexGuard lock;
        const auto it = GetState()->connections.find(conn_id);
        if (it == GetState()->connections.end()) {
            return;
        }
        ConnectionInfo& ci = it->second;
        auto* transport = GetTransportLocked(ci.transport_id);
        const bool active = transport && transport->state == ConnState::Established;
        if (active && !ci.dead_fired) {
            SendControlCloseLocked(ci, ControlReason::Deactivate);
        }
    }
    CloseConnectionAndDispatchDead(conn_id, ORBIS_NP_SIGNALING_ERROR_TERMINATED_BY_MYSELF,
                                   ORBIS_NP_MATCHING2_SIGNALING_ERROR_TERMINATED_BY_MYSELF);
}

void TerminateSig1(OrbisNpSignalingConnectionId conn_id) {
    {
        SignalingMutexGuard lock;
        const auto it = GetState()->connections.find(conn_id);
        if (it != GetState()->connections.end()) {
            SendControlCloseLocked(it->second, ControlReason::Terminate);
        }
    }
    CloseConnectionAndDispatchDead(conn_id, ORBIS_NP_SIGNALING_ERROR_TERMINATED_BY_MYSELF,
                                   ORBIS_NP_MATCHING2_SIGNALING_ERROR_TERMINATED_BY_MYSELF);
}

void StartMatching2(NpMatching2::ContextObject& ctx) {
    if (!ctx.signaling_callback) {
        return;
    }

    if (!EnsureTransport()) {
        return;
    }

    struct PeerTarget {
        NpMatching2::OrbisNpMatching2RoomMemberId member_id;
        NpMatching2::MemberCache member;
    };

    std::vector<PeerTarget> targets;
    {
        auto cache = NpMatching2::LockCache(ctx.ctx_id);
        for (const auto& [room_id, room] : cache->rooms) {
            for (const auto& [member_id, member] : room.members) {
                if (ShouldConnectToPeer(room, ctx.my_member_id, member_id)) {
                    targets.push_back({member_id, member});
                }
            }
        }
    }

    std::vector<s32> pending_transports;
    for (const auto& target : targets) {
        const auto member_id = target.member_id;
        const auto& member = target.member;

        u32 peer_addr = 0;
        u16 peer_port = 0;
        if (!ResolvePeerEndpoint(member, &peer_addr, &peer_port)) {
            continue;
        }

        s32 transport_id = 0;
        s32 status = ORBIS_NP_SIGNALING_CONN_STATUS_PENDING;
        {
            SignalingMutexGuard lock;
            transport_id = EnsureTransportLocked(ctx.online_id, member.np_id.handle, member.np_id,
                                                 peer_addr, peer_port);
            if (transport_id < 0) {
                continue;
            }
            auto& transport = GetState()->peer_transports[transport_id];
            if (transport.state == ConnState::WaitOffer) {
                SetTransportStateLocked(transport, ConnState::SendingOffer);
            }
            status = transport.status;
            if (transport.state != ConnState::Established) {
                pending_transports.push_back(transport_id);
            }
        }

        auto cache = NpMatching2::LockCache(ctx.ctx_id);
        auto& peer = cache->peers[member_id];
        peer.conn_id = transport_id;
        peer.member_id = member_id;
        peer.addr = peer_addr;
        peer.port = peer_port;
        peer.status = status == ORBIS_NP_SIGNALING_CONN_STATUS_ACTIVE ? kMatching2ConnActive
                                                                      : kMatching2ConnPending;
    }

    SyncMatching2Peers();
    for (const s32 transport_id : pending_transports) {
        StartTransportHandshakeInitiator(transport_id);
    }
}

void StopMatching2(NpMatching2::ContextObject& ctx, NpMatching2::OrbisNpMatching2RoomId room_id,
                   s32 error_code) {
    std::vector<s32> transports;
    {
        auto cache = NpMatching2::LockCache(ctx.ctx_id);
        const auto room_it = cache->rooms.find(room_id);
        if (room_it != cache->rooms.end()) {
            for (const auto& [member_id, member] : room_it->second.members) {
                const auto peer_it = cache->peers.find(member_id);
                if (peer_it != cache->peers.end() && peer_it->second.conn_id != 0) {
                    transports.push_back(peer_it->second.conn_id);
                }
            }
        }
    }

    for (const s32 transport_id : transports) {
        bool close_transport = false;
        {
            SignalingMutexGuard lock;
            close_transport = !HasSig1ConnectionForTransportLocked(transport_id);
        }
        if (close_transport) {
            CloseTransportAndDispatchDead(transport_id, error_code, error_code);
        }
    }
}

void StopMatching2(NpMatching2::ContextObject& ctx, s32 error_code) {
    std::vector<NpMatching2::OrbisNpMatching2RoomId> rooms;
    {
        auto cache = NpMatching2::LockCache(ctx.ctx_id);
        rooms.reserve(cache->rooms.size());
        for (const auto& [room_id, room] : cache->rooms) {
            rooms.push_back(room_id);
        }
    }
    for (const auto room_id : rooms) {
        StopMatching2(ctx, room_id, error_code);
    }
}

} // namespace Libraries::Np::NpSignaling
