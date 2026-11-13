// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include <unordered_map>
#include <orbis/Net.h>
#include <sys/socket.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/network/net/net.h"
#include "core/net_util/net_p2p.h"
#include "core/net_util/net_upnp.h"
#include "shadnet/config.h"

extern "C" {
void __sys_socketclose();
void _bind();
void _close();
void _connect();
void _recvfrom();
void _recvmsg();
void _sendmsg();
void _sendto();
}

SHADNET_HOOK_DECLARE(Libraries::Kernel, __sys_socketex);
SHADNET_HOOK_DECLARE(Libraries::Kernel, socket);
SHADNET_HOOK_DECLARE(Libraries::Kernel, setsockopt);
SHADNET_HOOK_DECLARE(Libraries::Kernel, _bind);
SHADNET_HOOK_DECLARE(Libraries::Kernel, __sys_socketclose);
SHADNET_HOOK_DECLARE(Libraries::Kernel, _close);
SHADNET_HOOK_DECLARE(Libraries::Kernel, _recvfrom);
SHADNET_HOOK_DECLARE(Libraries::Kernel, _recvmsg);
SHADNET_HOOK_DECLARE(Libraries::Kernel, _sendmsg);
SHADNET_HOOK_DECLARE(Libraries::Kernel, _sendto);

static void RegisterKernelSocketHooks() {
    SHADNET_HOOK(Libraries::Kernel, __sys_socketex);
    SHADNET_HOOK(Libraries::Kernel, socket);
    SHADNET_HOOK(Libraries::Kernel, setsockopt);
    SHADNET_HOOK(Libraries::Kernel, _bind);
    SHADNET_HOOK(Libraries::Kernel, __sys_socketclose);
    SHADNET_HOOK(Libraries::Kernel, _close);
    SHADNET_HOOK(Libraries::Kernel, _recvfrom);
    SHADNET_HOOK(Libraries::Kernel, _recvmsg);
    SHADNET_HOOK(Libraries::Kernel, _sendmsg);
    SHADNET_HOOK(Libraries::Kernel, _sendto);
}

namespace Libraries::Kernel {

std::mutex p2p_mtx{};
std::unordered_map<s32, u16> p2p_socks{};

static void MapP2PPort(s32 s, const void* addr = nullptr, u32 addrlen = 0) {
    {
        std::scoped_lock lk{p2p_mtx};
        const auto it = p2p_socks.find(s);
        if (it == p2p_socks.end() || it->second != 0) {
            return;
        }
    }
    if (!ShadNet::Settings::GetInstance().IsUpnpEnabled()) {
        return;
    }

    u16 port = 0;
    if (addr != nullptr) {
        if (addrlen >= sizeof(OrbisNetSockaddrIn)) {
            port = sceNetNtohs(static_cast<const OrbisNetSockaddrIn*>(addr)->sin_port);
        }
    } else {
        OrbisNetSockaddrIn sa{};
        OrbisNetSocklen_t sa_len = sizeof(sa);
        if (sceNetGetsockname(s, reinterpret_cast<OrbisNetSockaddr*>(&sa), &sa_len) >= 0) {
            port = sceNetNtohs(sa.sin_port);
        }
    }
    if (port == 0) {
        return;
    }

    {
        std::scoped_lock lk{p2p_mtx};
        const auto it = p2p_socks.find(s);
        if (it == p2p_socks.end() || it->second != 0) {
            return;
        }
        it->second = port;
    }
    LOG_INFO(Lib_Kernel, "mapping udp port {} for p2p socket {}", port, s);
    auto& upnp = NetUtil::UPnPClient::Instance();
    upnp.Start();
    upnp.MapPortAsync(port);
}

s32 __sys_socketex(const char* name, s32 family, s32 type, s32 protocol) {
    s32 s = SHADNET_HOOK_CONTINUE(__sys_socketex, name, family, type, protocol);
    if (s > 0 && (type == Network::Net::ORBIS_NET_SOCK_DGRAM_P2P ||
                  type == Network::Net::ORBIS_NET_SOCK_STREAM_P2P)) {
        std::scoped_lock lk{p2p_mtx};
        p2p_socks.insert_or_assign(s, 0);
    }
    return s;
}

s32 socket(s32 family, s32 type, s32 protocol) {
    s32 s = SHADNET_HOOK_CONTINUE(socket, family, type, protocol);
    if (s > 0 && (type == Network::Net::ORBIS_NET_SOCK_DGRAM_P2P ||
                  type == Network::Net::ORBIS_NET_SOCK_STREAM_P2P)) {
        std::scoped_lock lk{p2p_mtx};
        p2p_socks.insert_or_assign(s, 0);
    }
    return s;
}

s32 setsockopt(s32 s, s32 level, s32 optname, const void* optval, u32 optlen) {
    {
        std::scoped_lock lk{p2p_mtx};
        if (level == Network::Net::ORBIS_NET_SOL_SOCKET && p2p_socks.contains(s) &&
            (optname == Network::Net::ORBIS_NET_SO_USECRYPTO ||
             optname == Network::Net::ORBIS_NET_SO_USESIGNATURE)) {
            LOG_INFO(Lib_Kernel, "Removing crypto flags from P2P socket {}", s);
            return 0;
        }
    }
    return SHADNET_HOOK_CONTINUE(setsockopt, s, level, optname, optval, optlen);
}

static constexpr s32 RemoveCryptoFlags(s32 s, s32 flags) {
    {
        std::scoped_lock lk{p2p_mtx};
        if (!p2p_socks.contains(s)) {
            return flags;
        }
    }
    s32 new_flags =
        flags & ~(Network::Net::ORBIS_NET_MSG_USECRYPTO | Network::Net::ORBIS_NET_MSG_USESIGNATURE);
    if (new_flags != flags) {
        LOG_INFO(Lib_Kernel, "Removing crypto flags from P2P socket {}", s);
    }
    return new_flags;
}

s32 _bind(s32 s, const void* addr, u32 addrlen) {
    MapP2PPort(s, addr, addrlen);
    return SHADNET_HOOK_CONTINUE(_bind, s, addr, addrlen);
}

static constexpr void UnmapP2PPort(s32 s) {
    u16 port = 0;
    {
        std::scoped_lock lk{p2p_mtx};
        if (const auto it = p2p_socks.find(s); it != p2p_socks.end()) {
            port = it->second;
            p2p_socks.erase(it);
        }
    }
    if (port != 0) {
        LOG_INFO(Lib_Kernel, "unmapping udp port {} for p2p socket {}", port, s);
        NetUtil::UPnPClient::Instance().UnmapPort(port);
    }
}

s32 __sys_socketclose(s32 s) {
    UnmapP2PPort(s);
    return SHADNET_HOOK_CONTINUE(__sys_socketclose, s);
}

s32 _close(s32 fd) {
    UnmapP2PPort(fd);
    return SHADNET_HOOK_CONTINUE(_close, fd);
}

s32 _recvfrom(s32 s, void* buf, u64 len, s32 flags, void* from, u32* fromlen) {
    return SHADNET_HOOK_CONTINUE(_recvfrom, s, buf, len, RemoveCryptoFlags(s, flags), from,
                                 fromlen);
}

s32 _recvmsg(s32 s, void* msg, s32 flags) {
    return SHADNET_HOOK_CONTINUE(_recvmsg, s, msg, RemoveCryptoFlags(s, flags));
}

s32 _sendmsg(s32 s, const void* msg, s32 flags) {
    return SHADNET_HOOK_CONTINUE(_sendmsg, s, msg, RemoveCryptoFlags(s, flags));
}

s32 _sendto(s32 s, const void* msg, u64 len, s32 flags, const void* to, u32 tolen) {
    return SHADNET_HOOK_CONTINUE(_sendto, s, msg, len, RemoveCryptoFlags(s, flags), to, tolen);
}

void RegisterSocketHooks() {
    return RegisterKernelSocketHooks();
}

} // namespace Libraries::Kernel