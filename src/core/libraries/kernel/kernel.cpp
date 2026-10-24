// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <sys/socket.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/kernel/kernel.h"

SHADNET_HOOK_DECLARE(Libraries::Kernel, socket);
SHADNET_HOOK_DECLARE(Libraries::Kernel, __sys_socketex);

static void RegisterKernelHooks() {
    SHADNET_HOOK(Libraries::Kernel, socket);
    SHADNET_HOOK(Libraries::Kernel, __sys_socketex);
}

namespace Libraries::Kernel {

s32 socket(s32 family, s32 type, s32 protocol) {
    LOG_INFO(Lib_Kernel, "called, family = {}, type = {}, protocol = {}", family, type, protocol);
    return SHADNET_HOOK_CONTINUE(socket, family, type, protocol);
}

s32 __sys_socketex(const char* name, s32 family, s32 type, s32 protocol) {
    LOG_INFO(Lib_Kernel, "called, name = {}, family = {}, type = {}, protocol = {}",
             name ? name : "(null)", family, type, protocol);
    return SHADNET_HOOK_CONTINUE(__sys_socketex, name, family, type, protocol);
}

void RegisterHooks() {
    return RegisterKernelHooks();
}
} // namespace Libraries::Kernel
