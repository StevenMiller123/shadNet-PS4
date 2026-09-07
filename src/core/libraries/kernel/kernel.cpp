// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <sys/socket.h>
#include <sys/mman.h>

#include "common/logging/log.h"
#include "core/libraries/kernel/kernel.h"

HOOK_INIT(sceKernelMmap);
s32 sceKernelMmap_hook(void* addr, u64 len, s32 prot, s32 flags, s32 fd, s64 offset, void** result) {
    if ((flags & 0x1000) == 0x1000 && (flags & 0x2000) == 0) {
        // Append MAP_SYSTEM, that way mmaps used to create hooks aren't consuming flex budget.
        flags |= 0x2000;
    }
    return SHADNET_HOOK_CONTINUE(sceKernelMmap, addr, len, prot, flags, fd, offset, result);
}

HOOK_INIT(mmap);
void* mmap_hook(void* addr, u64 len, s32 prot, s32 flags, s32 fd, s64 offset) {
    if ((flags & 0x1000) == 0x1000 && (flags & 0x2000) == 0) {
        // Append MAP_SYSTEM, that way mmaps used to create hooks aren't consuming flex budget.
        flags |= 0x2000;
    }
    return SHADNET_HOOK_CONTINUE(mmap, addr, len, prot, flags, fd, offset);
}

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
    HOOK(sceKernelMmap);
    HOOK(mmap);
    HOOK(socket);
    HOOK(__sys_socketex);
}

namespace Libraries::Kernel::Kernel {

void RegisterHooks() {
    return RegisterKernelHooks();
}
} // namespace Libraries::Kernel::Kernel