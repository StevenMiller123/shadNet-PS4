// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <string>
#include <vector>
#include <orbis/Net.h>
#include <orbis/NetCtl.h>
#include "common/logging/log.h"
#include "shadnet/client.h" // socket platform typedefs + protocol constants
#include "shadnet/server_probe.h"

static void ProbePlatformInit() {}
namespace ShadNet {

static u32 ProbeGetLE32(const u8* p) {
    return static_cast<u32>(p[0]) | (static_cast<u32>(p[1]) << 8) | (static_cast<u32>(p[2]) << 16) |
           (static_cast<u32>(p[3]) << 24);
}

// recv() exactly n bytes (blocking, bounded by the socket's SO_RCVTIMEO).
static bool ProbeRecvN(ShadSocketHandle sock, u8* buf, u32 n) {
    u32 got = 0;
    while (got < n) {
        const int r =
            ::recv(sock, reinterpret_cast<char*>(buf) + got, static_cast<int>(n - got), 0);
        if (r <= 0)
            return false;
        got += static_cast<u32>(r);
    }
    return true;
}

ProbeInfo ProbeServer(const std::string& host, u16 port, u32 timeout_ms) {
    ProbeInfo info{};

    if (host.empty()) {
        LOG_WARNING("probe skipped: empty shadNet host");
        return info; // Unreachable
    }

    ProbePlatformInit();

    // Check for internet
    OrbisNetCtlInfo net_info{};
    s32 result = sceNetCtlGetInfo(ORBIS_NET_CTL_INFO_LINK, &net_info);
    if (result == ORBIS_NET_CTL_ERROR_NOT_CONNECTED || (result == 0 && net_info.link == 0)) {
        LOG_WARNING("Probe skipped, internet is disabled");
        return info;
    }

    s32 netpool_id = sceNetPoolCreate("shadNet Pool", 0x1000, 0);
    if (netpool_id < 0) {
        LOG_WARNING("Failed to create net pool, error = {:#x}", static_cast<u32>(netpool_id));
        return info;
    }

    s32 resolver = sceNetResolverCreate("shadNet Resolver", netpool_id, 0);
    if (resolver < 0) {
        LOG_WARNING("Failed to create resolver, error = {:#x}", static_cast<u32>(resolver));
        return info;
    }

    OrbisNetInAddr shadnet_addr{};
    result = sceNetResolverStartNtoa(resolver, host.c_str(), &shadnet_addr, 0, 0, 0);
    if (result != 0) {
        LOG_WARNING("DNS resolution failed for '{}', error = {:#x}", host.c_str(),
                    static_cast<u32>(result));
        return info;
    }

    result = sceNetResolverDestroy(resolver);
    if (result != 0) {
        LOG_WARNING("Failed to destroy resolver, error = {:#x}", static_cast<u32>(result));
    }

    result = sceNetPoolDestroy(netpool_id);
    if (result != 0) {
        LOG_WARNING("Failed to destroy net pool, error = {:#x}", static_cast<u32>(result));
    }

    ShadSocketHandle sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == SHAD_INVALID_SOCK) {
        LOG_WARNING("Failed to create socket");
        return info; // Unreachable
    }

    s32 optval = 1;
    result = ::setsockopt(sock, SOL_SOCKET, SO_NBIO, reinterpret_cast<void*>(&optval),
                          static_cast<socklen_t>(sizeof(optval)));
    if (result != 0) {
        LOG_WARNING("Failed to mark socket as non-blocking");
        return info;
    }

    OrbisNetSockaddrIn addr{};
    addr.sin_len = sizeof(OrbisNetSockaddrIn);
    addr.sin_family = AF_INET;
    addr.sin_addr = shadnet_addr.s_addr;
    addr.sin_port = htons(port);
    const int cr =
        ::connect(sock, reinterpret_cast<sockaddr*>(&addr), static_cast<socklen_t>(addr.sin_len));

    bool connected = false;
    const bool in_progress = (cr < 0 && errno == EINPROGRESS);
    if (cr == 0 || in_progress) {
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(sock, &wfds);
        struct timeval tv{};
        tv.tv_sec = static_cast<decltype(tv.tv_sec)>(timeout_ms / 1000);
        tv.tv_usec = static_cast<decltype(tv.tv_usec)>((timeout_ms % 1000) * 1000);
        if (::select(static_cast<int>(sock) + 1, nullptr, &wfds, nullptr, &tv) > 0) {
            int err = 0;
            socklen_t len = sizeof(err);
            ::getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&err), &len);
            connected = (err == 0);
        }
    } else {
        LOG_ERROR("Failed to connect to server, error = {}", errno);
    }

    if (!connected) {
        SHAD_CLOSE(sock);
        LOG_WARNING("server {}:{} is unreachable (timeout {} ms)", host, port, timeout_ms);
        return info; // Unreachable
    }

    optval = 0;
    result = ::setsockopt(sock, SOL_SOCKET, SO_NBIO, reinterpret_cast<void*>(&optval),
                          static_cast<socklen_t>(sizeof(optval)));
    if (result != 0) {
        LOG_WARNING("Failed to mark socket as blocking");
        return info;
    }

    struct timeval so_rcv{};
    so_rcv.tv_sec = static_cast<decltype(so_rcv.tv_sec)>(timeout_ms / 1000);
    so_rcv.tv_usec = static_cast<decltype(so_rcv.tv_usec)>((timeout_ms % 1000) * 1000);

    result = ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&so_rcv),
                          sizeof(so_rcv));
    if (result != 0) {
        LOG_WARNING("Failed to set recv timeout");
        return info;
    }

    info.result = ProbeResult::ProtocolError;

    u8 hdr[SHAD_HEADER_SIZE];
    if (!ProbeRecvN(sock, hdr, SHAD_HEADER_SIZE)) {
        SHAD_CLOSE(sock);
        LOG_WARNING("{}:{} reachable but no ServerInfo header received", host, port);
        return info;
    }
    if (static_cast<PacketType>(hdr[0]) != PacketType::ServerInfo) {
        SHAD_CLOSE(sock);
        LOG_WARNING("{}:{} sent packet type {:02x} instead of ServerInfo", host, port, hdr[0]);
        return info;
    }

    const u32 total_sz = ProbeGetLE32(hdr + 3);
    if (total_sz < SHAD_HEADER_SIZE || total_sz > SHAD_MAX_PACKET_SIZE) {
        SHAD_CLOSE(sock);
        LOG_WARNING("{}:{} sent corrupt ServerInfo (total_sz={})", host, port, total_sz);
        return info;
    }
    const u32 payload_sz = total_sz - SHAD_HEADER_SIZE;
    std::vector<u8> payload(payload_sz);
    if (payload_sz > 0 && !ProbeRecvN(sock, payload.data(), payload_sz)) {
        SHAD_CLOSE(sock);
        LOG_WARNING("{}:{} ServerInfo payload read failed", host, port);
        return info;
    }
    SHAD_CLOSE(sock);

    if (payload_sz < 4) {
        info.result = ProbeResult::Ok;
        LOG_INFO("server {}:{} is reachable (no version field in ServerInfo)", host, port);
        return info;
    }

    info.server_version = ProbeGetLE32(payload.data());
    if (info.server_version != SHAD_PROTOCOL_VERSION) {
        info.result = ProbeResult::VersionMismatch;
        LOG_WARNING("server {}:{} protocol version mismatch (server v{}, ours v{})", host, port,
                    info.server_version, SHAD_PROTOCOL_VERSION);
        return info;
    }

    info.result = ProbeResult::Ok;
    LOG_INFO("server {}:{} is reachable (protocol v{})", host, port, info.server_version);
    return info;
}

} // namespace ShadNet
