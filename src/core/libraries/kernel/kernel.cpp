// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <sys/socket.h>

#include "common/logging/log.h"
#include "core/libraries/kernel/kernel.h"

HOOK_INIT(socket);
HOOK_INIT(__sys_socketex);

s32 socket_hook(s32 family, s32 type, s32 protocol) {
    LOG_INFO("called, family = {}, type = {}, protocol = {}", family, type, protocol);
    return HOOK_CONTINUE(socket, s32 (*)(s32, s32, s32), family, type, protocol);
}

s32 __sys_socketex_hook(const char* name, s32 family, s32 type, s32 protocol) {
    LOG_INFO("called, name = {}, family = {}, type = {}, protocol = {}", name, family, type, protocol);
    return HOOK_CONTINUE(__sys_socketex, s32 (*)(const char*, s32, s32, s32), name, family, type, protocol);
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