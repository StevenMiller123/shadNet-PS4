// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/Net.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/network/fake_psn/fake_matching2.h"
#include "core/libraries/network/fake_psn/fake_psn.h"
#include "core/libraries/network/fake_psn/fake_signaling.h"
#include "core/libraries/network/fake_psn/fake_stun.h"
#include "core/libraries/network/http/http.h"
#include "core/libraries/network/net/net.h"
#include "core/libraries/network/net/net_error.h"

extern "C" void sceNetResolverConnect();

SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetResolverStartNtoa);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetResolverStartNtoa6);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetResolverStartAton);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetResolverStartAton6);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetResolverStartNtoaMultipleRecords);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetResolverStartNtoaMultipleRecordsEx);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetResolverConnect);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetSocket);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetSocketClose);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetSendto);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetRecvfrom);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetConnect);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetSend);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetRecv);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetEpollControl);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetEpollWait);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetEpollDestroy);

static void RegisterNetHooks() {
    SHADNET_HOOK(Libraries::Network::Net, sceNetResolverStartNtoa);
    SHADNET_HOOK(Libraries::Network::Net, sceNetResolverStartNtoa6);
    SHADNET_HOOK(Libraries::Network::Net, sceNetResolverStartAton);
    SHADNET_HOOK(Libraries::Network::Net, sceNetResolverStartAton6);
    SHADNET_HOOK(Libraries::Network::Net, sceNetResolverStartNtoaMultipleRecords);
    SHADNET_HOOK(Libraries::Network::Net, sceNetResolverStartNtoaMultipleRecordsEx);
    SHADNET_HOOK(Libraries::Network::Net, sceNetResolverConnect);
    SHADNET_HOOK(Libraries::Network::Net, sceNetSocket);
    SHADNET_HOOK(Libraries::Network::Net, sceNetSocketClose);
    SHADNET_HOOK(Libraries::Network::Net, sceNetSendto);
    SHADNET_HOOK(Libraries::Network::Net, sceNetRecvfrom);
    SHADNET_HOOK(Libraries::Network::Net, sceNetConnect);
    SHADNET_HOOK(Libraries::Network::Net, sceNetSend);
    SHADNET_HOOK(Libraries::Network::Net, sceNetRecv);
    SHADNET_HOOK(Libraries::Network::Net, sceNetEpollControl);
    SHADNET_HOOK(Libraries::Network::Net, sceNetEpollWait);
    SHADNET_HOOK(Libraries::Network::Net, sceNetEpollDestroy);
}

namespace Libraries::Network::Net {

namespace {

bool BlockPsnHost(const char* hostname) {
    if (hostname == nullptr || !IsPsnHost(hostname)) {
        return false;
    }
    LOG_ERROR(Lib_Net, "blocked dns lookup for {}", hostname);
    return true;
}

// Replace Sony's STUN server address with shadNet's
std::optional<u32> FakeStunAddr(const char* hostname) {
    if (hostname == nullptr || !FakeStun::IsStunHost(hostname)) {
        return std::nullopt;
    }
    const u32 addr = FakeStun::Instance().GetServerAddr();
    if (addr == 0) {
        return std::nullopt;
    }
    LOG_INFO(Lib_Net, "resolving {} to the fake stun server", hostname);
    return addr;
}

void FillResolverInfo(OrbisNetResolverInfo* info, u32 addr) {
    *info = {};
    info->addrs[0].un.addr.s_addr = addr;
    info->addrs[0].af = ORBIS_NET_AF_INET;
    info->records = 1;
    info->dns4records = 1;
}

} // namespace

s32 sceNetResolverStartNtoa(s32 rid, const char* hostname, OrbisNetInAddr* addr, s32 timeout,
                            s32 retry, s32 flags) {
    LOG_INFO(Lib_Net, "called, hostname = {}, timeout = {}, retry = {}, flags = {:#x}",
             hostname ? hostname : "(null)", timeout, retry, flags);
    if (hostname != nullptr) {
        Http::NoteResolvedHost(hostname);
    }
    if (const auto stun_addr = FakeStunAddr(hostname); stun_addr && addr != nullptr) {
        addr->s_addr = *stun_addr;
        return 0;
    }
    if (BlockPsnHost(hostname)) {
        return ORBIS_NET_ERROR_RESOLVER_ENOHOST;
    }
    return SHADNET_HOOK_CONTINUE(sceNetResolverStartNtoa, rid, hostname, addr, timeout, retry,
                                 flags);
}

s32 sceNetResolverStartNtoa6(s32 rid, const char* hostname, OrbisNetIn6Addr* addr, s32 timeout,
                             s32 retry, s32 flags) {
    LOG_INFO(Lib_Net, "called, hostname = {}, timeout = {}, retry = {}, flags = {:#x}",
             hostname ? hostname : "(null)", timeout, retry, flags);
    if (hostname != nullptr) {
        Http::NoteResolvedHost(hostname);
    }
    if (BlockPsnHost(hostname)) {
        return ORBIS_NET_ERROR_RESOLVER_ENOHOST;
    }
    return SHADNET_HOOK_CONTINUE(sceNetResolverStartNtoa6, rid, hostname, addr, timeout, retry,
                                 flags);
}

s32 sceNetResolverStartAton(s32 rid, const OrbisNetInAddr* addr, char* hostname, s32 len,
                            s32 timeout, s32 retry, s32 flags) {
    char addrbuf[16];
    LOG_INFO(Lib_Net, "called, addr = {}, len = {}, timeout = {}, retry = {}, flags = {:#x}",
             sceNetInetNtop(ORBIS_NET_AF_INET, addr, addrbuf, sizeof(addrbuf)), len, timeout, retry,
             flags);
    return SHADNET_HOOK_CONTINUE(sceNetResolverStartAton, rid, addr, hostname, len, timeout, retry,
                                 flags);
}

s32 sceNetResolverStartAton6(s32 rid, const OrbisNetIn6Addr* addr, char* hostname, s32 len,
                             s32 timeout, s32 retry, s32 flags) {
    char addrbuf[46];
    LOG_INFO(Lib_Net, "called, addr = {}, len = {}, timeout = {}, retry = {}, flags = {:#x}",
             sceNetInetNtop(ORBIS_NET_AF_INET6, addr, addrbuf, sizeof(addrbuf)), len, timeout,
             retry, flags);
    return SHADNET_HOOK_CONTINUE(sceNetResolverStartAton6, rid, addr, hostname, len, timeout, retry,
                                 flags);
}

s32 sceNetResolverStartNtoaMultipleRecords(s32 rid, const char* hostname,
                                           OrbisNetResolverInfo* info, s32 timeout, s32 retry,
                                           s32 flags) {
    LOG_INFO(Lib_Net, "called, hostname = {}, timeout = {}, retry = {}, flags = {:#x}",
             hostname ? hostname : "(null)", timeout, retry, flags);
    if (hostname != nullptr) {
        Http::NoteResolvedHost(hostname);
    }
    if (const auto stun_addr = FakeStunAddr(hostname); stun_addr && info != nullptr) {
        FillResolverInfo(info, *stun_addr);
        return 0;
    }
    if (BlockPsnHost(hostname)) {
        return ORBIS_NET_ERROR_RESOLVER_ENOHOST;
    }
    return SHADNET_HOOK_CONTINUE(sceNetResolverStartNtoaMultipleRecords, rid, hostname, info,
                                 timeout, retry, flags);
}

s32 sceNetResolverStartNtoaMultipleRecordsEx(s32 rid, const char* hostname,
                                             OrbisNetResolverInfo* info, s32 timeout, s32 retry,
                                             s32 flags) {
    LOG_INFO(Lib_Net, "called, hostname = {}, timeout = {}, retry = {}, flags = {:#x}",
             hostname ? hostname : "(null)", timeout, retry, flags);
    if (hostname != nullptr) {
        Http::NoteResolvedHost(hostname);
    }
    if (const auto stun_addr = FakeStunAddr(hostname); stun_addr && info != nullptr) {
        FillResolverInfo(info, *stun_addr);
        return 0;
    }
    if (BlockPsnHost(hostname)) {
        return ORBIS_NET_ERROR_RESOLVER_ENOHOST;
    }
    return SHADNET_HOOK_CONTINUE(sceNetResolverStartNtoaMultipleRecordsEx, rid, hostname, info,
                                 timeout, retry, flags);
}

s32 sceNetResolverConnect(OrbisNetResolverConnectInfo* rcinfo, const char* hostname,
                          OrbisNetInPort_t port, OrbisNetResolverConnectParam* rcparam) {
    LOG_INFO(Lib_Net,
             "called, hostname = {}, port = {}, connect_timeout = {}, timeout = {}, retry = {}, "
             "flags = {:#x}",
             hostname ? hostname : "(null)", sceNetHtons(port), rcparam->connecttimeo,
             rcparam->timeout, rcparam->retry, rcparam->flags);
    if (hostname != nullptr) {
        Http::NoteResolvedHost(hostname);
    }
    if (BlockPsnHost(hostname)) {
        return ORBIS_NET_ERROR_RESOLVER_ENOHOST;
    }
    return SHADNET_HOOK_CONTINUE(sceNetResolverConnect, rcinfo, hostname, port, rcparam);
}

s32 sceNetSocket(const char* name, s32 family, s32 type, s32 protocol) {
    LOG_DEBUG(Lib_Net, "called, name = {}, family = {}, type = {}, protocol = {}",
              name ? name : "(null)", family, type, protocol);
    const s32 socket = SHADNET_HOOK_CONTINUE(sceNetSocket, name, family, type, protocol);
    if (socket >= 0) {
        FakeMatching2::Instance().OnSocketCreate(socket, name, type);
        FakeSignaling::Instance().OnSocketCreate(socket, name);
    }
    return socket;
}

s32 sceNetSocketClose(s32 s) {
    FakeStun::Instance().OnSocketClose(s);
    FakeMatching2::Instance().OnSocketClose(s);
    FakeSignaling::Instance().OnSocketClose(s);
    return SHADNET_HOOK_CONTINUE(sceNetSocketClose, s);
}

s32 sceNetConnect(s32 s, const OrbisNetSockaddr* addr, u32 addrlen) {
    if (const auto ret = FakeMatching2::Instance().OnConnect(s, addr, addrlen)) {
        return *ret;
    }
    return SHADNET_HOOK_CONTINUE(sceNetConnect, s, addr, addrlen);
}

s32 sceNetSend(s32 s, const void* msg, u64 len, s32 flags) {
    if (const auto ret = FakeMatching2::Instance().OnSend(s, msg, len, flags)) {
        return *ret;
    }
    return SHADNET_HOOK_CONTINUE(sceNetSend, s, msg, len, flags);
}

s32 sceNetRecv(s32 s, void* buf, u64 len, s32 flags) {
    if (const auto ret = FakeMatching2::Instance().OnRecv(s, buf, len, flags)) {
        return *ret;
    }
    return SHADNET_HOOK_CONTINUE(sceNetRecv, s, buf, len, flags);
}

s32 sceNetEpollControl(s32 eid, s32 op, s32 id, OrbisNetEpollEvent* event) {
    if (const auto ret = FakeMatching2::Instance().OnEpollControl(eid, op, id, event)) {
        return *ret;
    }
    FakeSignaling::Instance().OnEpollControl(eid, op, id, event);
    return SHADNET_HOOK_CONTINUE(sceNetEpollControl, eid, op, id, event);
}

s32 sceNetEpollWait(s32 eid, OrbisNetEpollEvent* events, s32 maxevents, s32 timeout) {
    if (const auto ret = FakeMatching2::Instance().OnEpollWait(eid, events, maxevents, timeout)) {
        return *ret;
    }
    if (const auto ret =
            FakeSignaling::Instance().OnEpollWait(eid, events, maxevents, timeout, [&](s32 slice) {
                return SHADNET_HOOK_CONTINUE(sceNetEpollWait, eid, events, maxevents, slice);
            })) {
        return *ret;
    }
    return SHADNET_HOOK_CONTINUE(sceNetEpollWait, eid, events, maxevents, timeout);
}

s32 sceNetEpollDestroy(s32 eid) {
    FakeMatching2::Instance().OnEpollDestroy(eid);
    FakeSignaling::Instance().OnEpollDestroy(eid);
    return SHADNET_HOOK_CONTINUE(sceNetEpollDestroy, eid);
}

s32 sceNetSendto(s32 s, const void* msg, u64 len, s32 flags, const OrbisNetSockaddr* to,
                 u32 tolen) {
    if (const auto ret = FakeStun::Instance().OnSendTo(s, msg, len, to, tolen)) {
        return *ret;
    }
    if (const auto ret =
            FakeSignaling::Instance().OnSendTo(s, {static_cast<const u8*>(msg), len}, to, tolen)) {
        return *ret;
    }
    return SHADNET_HOOK_CONTINUE(sceNetSendto, s, msg, len, flags, to, tolen);
}

s32 sceNetRecvfrom(s32 s, void* buf, u64 len, s32 flags, OrbisNetSockaddr* from, u32* fromlen) {
    if (const auto ret = FakeSignaling::Instance().OnRecvFrom(s, buf, len, from, fromlen)) {
        return *ret;
    }
    const s32 ret = SHADNET_HOOK_CONTINUE(sceNetRecvfrom, s, buf, len, flags, from, fromlen);
    if (ret <= 0) {
        return ret;
    }
    FakeSignaling::Instance().OnReceived(s, {static_cast<const u8*>(buf), static_cast<u64>(ret)},
                                         from, fromlen != nullptr ? *fromlen : 0);
    return FakeStun::Instance().OnRecvFrom(s, buf, len, ret, from, fromlen);
}

void RegisterHooks() {
    return RegisterNetHooks();
}
} // namespace Libraries::Network::Net