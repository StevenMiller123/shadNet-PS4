// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <sys/socket.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/network/net/net.h"

SHADNET_HOOK_DECLARE(Libraries::Kernel, setsockopt);

static void RegisterKernelHooks() {
    SHADNET_HOOK(Libraries::Kernel, setsockopt);
}

namespace Libraries::Kernel {

s32 setsockopt(s32 s, s32 level, s32 optname, const void* optval, u32 optlen) {
    if (level == Network::Net::ORBIS_NET_SOL_SOCKET &&
        (optname == Network::Net::ORBIS_NET_SO_USECRYPTO ||
         optname == Network::Net::ORBIS_NET_SO_USESIGNATURE)) {
        LOG_INFO(Lib_Net, "ignoring {} on socket {}",
                 optname == Network::Net::ORBIS_NET_SO_USECRYPTO ? "SO_USECRYPTO"
                                                                 : "SO_USESIGNATURE",
                 s);
        return 0;
    }
    return SHADNET_HOOK_CONTINUE(setsockopt, s, level, optname, optval, optlen);
}

void RegisterHooks() {
    return RegisterKernelHooks();
}
} // namespace Libraries::Kernel
