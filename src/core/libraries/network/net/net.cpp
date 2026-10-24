// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/Net.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/network/net/net.h"

extern "C" void sceNetResolverConnect();

SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetSocket);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetResolverStartNtoa);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetResolverStartNtoa6);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetResolverStartAton);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetResolverStartAton6);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetResolverStartNtoaMultipleRecords);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetResolverStartNtoaMultipleRecordsEx);
SHADNET_HOOK_DECLARE(Libraries::Network::Net, sceNetResolverConnect);

static void RegisterNetHooks() {
    SHADNET_HOOK(Libraries::Network::Net, sceNetSocket);
    SHADNET_HOOK(Libraries::Network::Net, sceNetResolverStartNtoa);
    SHADNET_HOOK(Libraries::Network::Net, sceNetResolverStartNtoa6);
    SHADNET_HOOK(Libraries::Network::Net, sceNetResolverStartAton);
    SHADNET_HOOK(Libraries::Network::Net, sceNetResolverStartAton6);
    SHADNET_HOOK(Libraries::Network::Net, sceNetResolverStartNtoaMultipleRecords);
    SHADNET_HOOK(Libraries::Network::Net, sceNetResolverStartNtoaMultipleRecordsEx);
    SHADNET_HOOK(Libraries::Network::Net, sceNetResolverConnect);
}

namespace Libraries::Network::Net {

s32 sceNetSocket(const char* name, s32 family, s32 type, s32 protocol) {
    LOG_INFO(Lib_Net, "called, name = {}, family = {}, type = {}, protocol = {}",
             name ? name : "(null)", family, type, protocol);
    return SHADNET_HOOK_CONTINUE(sceNetSocket, name, family, type, protocol);
}

s32 sceNetResolverStartNtoa(s32 rid, const char* hostname, OrbisNetInAddr* addr, s32 timeout,
                            s32 retry, s32 flags) {
    LOG_INFO(Lib_Net, "called, hostname = {}, timeout = {}, retry = {}, flags = {:#x}",
             hostname ? hostname : "(null)", timeout, retry, flags);
    return SHADNET_HOOK_CONTINUE(sceNetResolverStartNtoa, rid, hostname, addr, timeout, retry,
                                 flags);
}

s32 sceNetResolverStartNtoa6(s32 rid, const char* hostname, OrbisNetIn6Addr* addr, s32 timeout,
                             s32 retry, s32 flags) {
    LOG_INFO(Lib_Net, "called, hostname = {}, timeout = {}, retry = {}, flags = {:#x}",
             hostname ? hostname : "(null)", timeout, retry, flags);
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
    return SHADNET_HOOK_CONTINUE(sceNetResolverStartNtoaMultipleRecords, rid, hostname, info,
                                 timeout, retry, flags);
}

s32 sceNetResolverStartNtoaMultipleRecordsEx(s32 rid, const char* hostname,
                                             OrbisNetResolverInfo* info, s32 timeout, s32 retry,
                                             s32 flags) {
    LOG_INFO(Lib_Net, "called, hostname = {}, timeout = {}, retry = {}, flags = {:#x}",
             hostname ? hostname : "(null)", timeout, retry, flags);
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
    return SHADNET_HOOK_CONTINUE(sceNetResolverConnect, rcinfo, hostname, port, rcparam);
}

void RegisterHooks() {
    return RegisterNetHooks();
}
} // namespace Libraries::Network::Net