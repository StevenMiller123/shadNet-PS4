// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/Net.h>
#include <orbis/NetCtl.h>

#include "common/logging/log.h"
#include "core/libraries/network/net/net.h"
#include "core/libraries/network/net/net_error.h"
#include "core/net_util/net_p2p.h"
#include "shadnet/config.h"

namespace NetUtil {

constexpr u16 SignalingVport = 0x7fff;
constexpr u16 P2PPortFallbacks = 16;
constexpr u64 ChannelQueueBytes = 40 * 1024;
constexpr u64 RecvBufferSize = 2048;

OrbisNetSockaddrIn MakeSockaddr(u32 addr, u16 port) {
    OrbisNetSockaddrIn sa{};
    sa.sin_len = sizeof(sa);
    sa.sin_family = ORBIS_NET_AF_INET;
    sa.sin_port = port;
    sa.sin_addr = addr;
    sa.sin_vport = sceNetHtons(SignalingVport);
    return sa;
}

P2PTransport& P2PTransport::Instance() {
    static P2PTransport instance;
    return instance;
}

P2PTransport::~P2PTransport() {
    std::scoped_lock lock{m_mutex};
    StopLocked();
}

P2PTransport::Channel P2PTransport::Classify(std::span<const u8> packet) {
    if (packet.size() < 5 || std::memcmp(packet.data(), "SHAD", 4) != 0) {
        return Signaling;
    }
    switch (packet[4]) {
    case 0x10:
        return Control;
    case 0x21:
        return Matching2;
    default:
        return Signaling;
    }
}

u16 P2PTransport::PortToBind() const {
    const s32 port = ShadNet::Settings::GetInstance().GetP2PPort();
    if (port > 0 && port <= 0xffff) {
        return static_cast<u16>(port);
    }
    return m_configured_port != 0 ? m_configured_port
                                  : Libraries::Network::Net::ORBIS_NET_ADHOC_PORT;
}

s32 P2PTransport::BindSocket(u16 udp_port) {
    const s32 sock =
        sceNetSocket("shadNetP2PSignaling", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM_P2P, 0);
    if (sock < 0) {
        LOG_ERROR(Lib_Net, "sceNetSocket failed with error {:#x}", static_cast<u32>(sock));
        return sock;
    }

    OrbisNetSockaddrIn sa = MakeSockaddr(0, sceNetHtons(udp_port));
    const s32 rc = sceNetBind(sock, reinterpret_cast<const OrbisNetSockaddr*>(&sa), sizeof(sa));
    if (rc < 0) {
        LOG_ERROR(Lib_Net, "sceNetBind on UDP port {} vport {:#x} failed with error {:#x}",
                  udp_port, SignalingVport, static_cast<u32>(rc));
        sceNetSocketClose(sock);
        return rc;
    }
    return sock;
}

u16 P2PTransport::StartLocked(u16 udp_port, bool allow_fallback) {
    s32 sock = BindSocket(udp_port);
    u16 port = udp_port;
    for (u16 i = 1; sock == ORBIS_NET_ERROR_EADDRINUSE && allow_fallback && udp_port != 0 &&
                    i <= P2PPortFallbacks && udp_port + i <= 0xffff;
         ++i) {
        port = static_cast<u16>(udp_port + i);
        sock = BindSocket(port);
    }
    if (sock < 0) {
        return 0;
    }

    OrbisNetSockaddrIn sa{};
    OrbisNetSocklen_t sa_len = sizeof(sa);
    if (sceNetGetsockname(sock, reinterpret_cast<OrbisNetSockaddr*>(&sa), &sa_len) >= 0 &&
        sa.sin_port != 0) {
        port = sceNetNtohs(sa.sin_port);
    }

    m_socket = sock;
    m_bound_port = port;
    if (udp_port != 0 && port != udp_port) {
        LOG_WARNING(Lib_Net, "UDP port {} is in use, P2P transport on UDP port {} instead",
                    udp_port, port);
    } else {
        LOG_INFO(Lib_Net, "P2P transport on UDP port {}", port);
    }
    return port;
}

void P2PTransport::StopLocked() {
    if (m_socket >= 0) {
        sceNetSocketClose(m_socket);
        LOG_INFO(Lib_Net, "P2P transport on UDP port {} stopped", m_bound_port);
    }
    m_socket = -1;
    m_bound_port = 0;
    m_retry_after = {};
    for (size_t i = 0; i < ChannelCount; ++i) {
        m_queues[i].clear();
        m_queue_bytes[i] = 0;
    }
}

bool P2PTransport::RunningLocked() {
    const auto now = std::chrono::steady_clock::now();
    if (m_socket < 0 && now >= m_retry_after) {
        if (StartLocked(PortToBind(), true) == 0) {
            m_retry_after = now + std::chrono::seconds{2};
        }
    }
    return m_socket >= 0;
}

void P2PTransport::DrainLocked() {
    u8 buf[RecvBufferSize];
    for (;;) {
        OrbisNetSockaddrIn from{};
        OrbisNetSocklen_t from_len = sizeof(from);
        const s32 rc = sceNetRecvfrom(m_socket, buf, sizeof(buf),
                                      Libraries::Network::Net::ORBIS_NET_MSG_DONTWAIT,
                                      reinterpret_cast<OrbisNetSockaddr*>(&from), &from_len);
        if (rc < 0) {
            if (rc != ORBIS_NET_ERROR_EAGAIN) {
                LOG_DEBUG(Lib_Net, "sceNetRecvfrom failed with error {:#x}", static_cast<u32>(rc));
            }
            return;
        }

        const std::span<const u8> packet{buf, static_cast<size_t>(rc)};
        const Channel channel = Classify(packet);
        if (m_queue_bytes[channel] + packet.size() > ChannelQueueBytes) {
            continue;
        }
        m_queue_bytes[channel] += packet.size();
        m_queues[channel].push_back(Packet{
            .data = {packet.begin(), packet.end()},
            .from_addr = from.sin_addr,
            .from_port = from.sin_port,
        });
    }
}

void P2PTransport::Configure(u16 udp_port, u32 advertised_addr) {
    std::scoped_lock lock{m_mutex};
    m_configured_port = udp_port;
    m_advertised_addr = advertised_addr;
    const u32 ip = sceNetNtohl(advertised_addr);
    LOG_INFO(Lib_Net, "UDP port {}, advertised address {}.{}.{}.{}", udp_port, ip >> 24,
             (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
}

u16 P2PTransport::Start(u16 udp_port) {
    std::scoped_lock lock{m_mutex};
    StopLocked();
    return StartLocked(udp_port, false);
}

void P2PTransport::Stop() {
    std::scoped_lock lock{m_mutex};
    StopLocked();
}

bool P2PTransport::Ensure() {
    std::scoped_lock lock{m_mutex};
    return RunningLocked();
}

bool P2PTransport::IsReady() {
    std::scoped_lock lock{m_mutex};
    return m_socket >= 0;
}

u16 P2PTransport::GetConfiguredPort() {
    std::scoped_lock lock{m_mutex};
    return PortToBind();
}

u16 P2PTransport::GetBoundPort() {
    std::scoped_lock lock{m_mutex};
    return m_bound_port;
}

u32 P2PTransport::GetAdvertisedAddr() {
    {
        std::scoped_lock lock{m_mutex};
        if (m_advertised_addr != 0) {
            return m_advertised_addr;
        }
    }

    OrbisNetCtlInfo info{};
    if (sceNetCtlGetInfo(ORBIS_NET_CTL_INFO_IP_ADDRESS, &info) < 0) {
        return 0;
    }
    u32 addr = 0;
    if (sceNetInetPton(ORBIS_NET_AF_INET, info.ip_address, &addr) <= 0) {
        return 0;
    }
    return addr;
}

u16 P2PTransport::GetAdvertisedPort() {
    const u16 bound = GetBoundPort();
    return bound != 0 ? bound : GetConfiguredPort();
}

s32 P2PTransport::SendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port) {
    std::scoped_lock lock{m_mutex};
    if (!RunningLocked() || (data == nullptr && len != 0)) {
        return -1;
    }
    const OrbisNetSockaddrIn to = MakeSockaddr(dest_addr, dest_port);
    const s32 rc = sceNetSendto(m_socket, data, len, 0,
                                reinterpret_cast<const OrbisNetSockaddr*>(&to), sizeof(to));
    return rc >= 0 ? static_cast<s32>(len) : -1;
}

s32 P2PTransport::RecvFrom(Channel channel, void* buf, u32 len, u32* from_addr, u16* from_port) {
    std::scoped_lock lock{m_mutex};
    if (!RunningLocked()) {
        return -1;
    }
    DrainLocked();
    auto& queue = m_queues[channel];
    if (queue.empty()) {
        return -1;
    }

    const Packet packet = std::move(queue.front());
    queue.pop_front();
    m_queue_bytes[channel] -= packet.data.size();

    const u32 n = std::min<u32>(len, static_cast<u32>(packet.data.size()));
    if (buf != nullptr) {
        std::memcpy(buf, packet.data.data(), n);
    }
    if (from_addr != nullptr) {
        *from_addr = packet.from_addr;
    }
    if (from_port != nullptr) {
        *from_port = packet.from_port;
    }
    return static_cast<s32>(n);
}

void ConfigureP2P(u16 udp_port, u32 advertised_addr) {
    P2PTransport::Instance().Configure(udp_port, advertised_addr);
}

u16 StartP2P(u16 udp_port) {
    return P2PTransport::Instance().Start(udp_port);
}

void StopP2P() {
    P2PTransport::Instance().Stop();
}

bool EnsureP2PTransport() {
    return P2PTransport::Instance().Ensure();
}

bool P2PTransportIsReady() {
    return P2PTransport::Instance().IsReady();
}

u16 GetP2PConfiguredPort() {
    return P2PTransport::Instance().GetConfiguredPort();
}

u16 GetP2PBoundPort() {
    return P2PTransport::Instance().GetBoundPort();
}

u32 GetP2PAdvertisedAddr() {
    return P2PTransport::Instance().GetAdvertisedAddr();
}

u16 GetP2PAdvertisedPort() {
    return P2PTransport::Instance().GetAdvertisedPort();
}

s32 P2PSignalingSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port) {
    return P2PTransport::Instance().SendTo(data, len, dest_addr, dest_port);
}

s32 P2PSignalingRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port) {
    return P2PTransport::Instance().RecvFrom(P2PTransport::Signaling, buf, len, from_addr,
                                             from_port);
}

s32 P2PControlSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port) {
    return P2PTransport::Instance().SendTo(data, len, dest_addr, dest_port);
}

s32 P2PControlRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port) {
    return P2PTransport::Instance().RecvFrom(P2PTransport::Control, buf, len, from_addr, from_port);
}

s32 P2PMatching2SendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port) {
    return P2PTransport::Instance().SendTo(data, len, dest_addr, dest_port);
}

s32 P2PMatching2RecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port) {
    return P2PTransport::Instance().RecvFrom(P2PTransport::Matching2, buf, len, from_addr,
                                             from_port);
}

} // namespace NetUtil
