// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::Network::Net {

struct OrbisNetInAddr {
    u32 s_addr;
};

struct OrbisNetIn6Addr {
    union {
        u8 __u6_addr8[16];
        u16 __u6_addr16[8];
        u32 __u6_addr32[4];
    } __u6_addr;
};

union OrbisNetResolverAddrUn {
    OrbisNetInAddr addr;
    OrbisNetIn6Addr addr6;
};

struct OrbisNetResolverAddr {
    OrbisNetResolverAddrUn un;
    s32 af;
    s32 reserved[3];
};

struct OrbisNetResolverInfo {
    OrbisNetResolverAddr addrs[10];
    s32 records;
    s32 dns4records;
    s32 dns6records;
    s32 rid6;
    s32 reserved[12];
};

struct OrbisNetResolverConnectInfo {
    OrbisNetResolverInfo info;
    s32 reserved1[10];
    s32 reserved2[6];
    s32 s4;
    s32 s6;
    s32 memid;
    s32 rid;
    s32 eid;
    s32 index;
    s32 flags;
    s32 rid6;
    s32 reserved[4];
};

struct OrbisNetResolverConnectParam {
    s32 connecttimeo;
    s32 timeout;
    s32 retry;
    s32 flags;
    s32 reserved[4];
};

using OrbisNetInAddr_t = u32;
using OrbisNetInPort_t = u16;
using OrbisNetSaFamily_t = u8;
using OrbisNetSocklen_t = u32;

constexpr u16 ORBIS_NET_ADHOC_PORT = 3658;

enum OrbisNetFlags : u32 {
    ORBIS_NET_MSG_PEEK = 0x00000002,
    ORBIS_NET_MSG_WAITALL = 0x00000040,
    ORBIS_NET_MSG_DONTWAIT = 0x00000080,
    ORBIS_NET_MSG_USECRYPTO = 0x00100000,
    ORBIS_NET_MSG_USESIGNATURE = 0x00200000,
    ORBIS_NET_MSG_PEEKLEN = (0x00400000 | ORBIS_NET_MSG_PEEK),
};

// Socket functions
s32 sceNetSocket(const char* name, s32 family, s32 type, s32 protocol);

// Resolver functions
s32 sceNetResolverStartNtoa(s32 rid, const char* hostname, OrbisNetInAddr* addr, s32 timeout,
                            s32 retry, s32 flags);
s32 sceNetResolverStartNtoa6(s32 rid, const char* hostname, OrbisNetIn6Addr* addr, s32 timeout,
                             s32 retry, s32 flags);
s32 sceNetResolverStartAton(s32 rid, const OrbisNetInAddr* addr, char* hostname, s32 len,
                            s32 timeout, s32 retry, s32 flags);
s32 sceNetResolverStartAton6(s32 rid, const OrbisNetIn6Addr* addr, char* hostname, s32 len,
                             s32 timeout, s32 retry, s32 flags);
s32 sceNetResolverStartNtoaMultipleRecords(s32 rid, const char* hostname,
                                           OrbisNetResolverInfo* info, s32 timeout, s32 retry,
                                           s32 flags);
s32 sceNetResolverStartNtoaMultipleRecordsEx(s32 rid, const char* hostname,
                                             OrbisNetResolverInfo* info, s32 timeout, s32 retry,
                                             s32 flags);

s32 sceNetResolverConnect(OrbisNetResolverConnectInfo* rcinfo, const char* hostname,
                          OrbisNetInPort_t port, OrbisNetResolverConnectParam* rcparam);
void RegisterHooks();
} // namespace Libraries::Network::Net