// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <sys/socket.h>
#include <sys/mman.h>

#include "common/logging/log.h"
#include "core/libraries/kernel/kernel.h"

HOOK_INIT(socket);
s32 socket_hook(s32 family, s32 type, s32 protocol) {
    LOG_INFO(Lib_Kernel, "called, family = {}, type = {}, protocol = {}", family, type, protocol);
    return SHADNET_HOOK_CONTINUE(socket, family, type, protocol);
}

HOOK_INIT(__sys_socketex);
s32 __sys_socketex_hook(const char* name, s32 family, s32 type, s32 protocol) {
    LOG_INFO(Lib_Kernel, "called, name = {}, family = {}, type = {}, protocol = {}", name, family, type, protocol);
    return SHADNET_HOOK_CONTINUE(__sys_socketex, name, family, type, protocol);
}

void RegisterKernelHooks() {
    HOOK(socket);
    HOOK(__sys_socketex);
}

namespace Libraries::Kernel::Kernel {

void RegisterHooks() {
    return RegisterKernelHooks();
}
} // namespace Libraries::Kernel::Kernel