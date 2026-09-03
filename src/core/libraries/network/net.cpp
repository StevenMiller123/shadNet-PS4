// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/Net.h>

#include "common/logging/log.h"
#include "core/libraries/network/net.h"

HOOK_INIT(sceNetSocket);

s32 sceNetSocket_hook(const char* name, s32 family, s32 type, s32 protocol) {
    LOG_INFO("called, name = {}, family = {}, type = {}, protocol = {}", name ? name : "(null)", family, type, protocol);
    return HOOK_CONTINUE(sceNetSocket, s32 (*)(const char*, s32, s32, s32), name, family, type, protocol);
}

void RegisterNetHooks() {
    HOOK(sceNetSocket);
}

namespace Libraries::Network::Net {

void RegisterHooks() {
    return RegisterNetHooks();
}
} // namespace Libraries::Network::Net