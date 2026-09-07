// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/Net.h>

#include "common/logging/log.h"
#include "core/libraries/network/net.h"

HOOK_INIT(sceNetSocket);
s32 sceNetSocket_hook(const char* name, s32 family, s32 type, s32 protocol) {
    LOG_INFO(Lib_Net, "called, name = {}, family = {}, type = {}, protocol = {}",
             name ? name : "(null)", family, type, protocol);
    return SHADNET_HOOK_CONTINUE(sceNetSocket, name, family, type, protocol);
}

HOOK_INIT(sceNetResolverStartNtoa);
s32 sceNetResolverStartNtoa_hook(s32 rid, const char* hostname, OrbisNetInAddr* addr, s32 timeout,
                                 s32 retry, s32 flags) {
    LOG_INFO(Lib_Net, "called, hostname = {}, timeout = {}, retry = {}",
             hostname ? hostname : "(null)", timeout, retry);
    return SHADNET_HOOK_CONTINUE(sceNetResolverStartNtoa, rid, hostname, addr, timeout, retry,
                                 flags);
}

void RegisterNetHooks() {
    HOOK(sceNetSocket);
    HOOK(sceNetResolverStartNtoa);
}

namespace Libraries::Network::Net {

void RegisterHooks() {
    return RegisterNetHooks();
}
} // namespace Libraries::Network::Net