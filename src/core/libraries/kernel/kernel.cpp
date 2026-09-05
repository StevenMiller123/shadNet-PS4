// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <sys/socket.h>
#include <sys/mman.h>

#include "common/logging/log.h"
#include "core/libraries/kernel/kernel.h"

s32 __sys_socketex(const char* name, s32 family, s32 type, s32 protocol);

HOOK_INIT(mmap);
HOOK_INIT(sceKernelMmap);
HOOK_INIT(socket);
HOOK_INIT(__sys_socketex);

s32 socket_hook(s32 family, s32 type, s32 protocol) {
    LOG_INFO(Lib_Kernel, "called, family = {}, type = {}, protocol = {}", family, type, protocol);
    return HOOK_CONTINUE(socket, s32 (*)(s32, s32, s32), family, type, protocol);
}

s32 __sys_socketex_hook(const char* name, s32 family, s32 type, s32 protocol) {
    LOG_INFO(Lib_Kernel, "called, name = {}, family = {}, type = {}, protocol = {}", name, family, type, protocol);
    return HOOK_CONTINUE(__sys_socketex, s32 (*)(const char*, s32, s32, s32), name, family, type, protocol);
}

void* mmap_hook(void* addr, u64 len, s32 prot, s32 flags, s32 fd, s64 offset) {
    if ((flags & 0x1000) == 0x1000 && (flags & 0x2000) == 0) {
        LOG_INFO(Lib_Kernel, "called with MAP_ANON, appending MAP_SYSTEM");
        flags |= 0x2000;
    }
    return HOOK_CONTINUE(mmap, void* (*)(void*, u64, s32, s32, s32, s64), addr, len, prot, flags, fd, offset);
}

s32 sceKernelMmap_hook(void* addr, u64 len, s32 prot, s32 flags, s32 fd, s64 offset, void** result) {
    if ((flags & 0x1000) == 0x1000 && (flags & 0x2000) == 0) {
        LOG_INFO(Lib_Kernel, "called with MAP_ANON, appending MAP_SYSTEM");
        flags |= 0x2000;
    }
    return HOOK_CONTINUE(sceKernelMmap, s32 (*)(void*, u64, s32, s32, s32, s64, void**), addr, len, prot, flags, fd, offset, result);
}

void RegisterKernelHooks() {
    HOOK(mmap);
    HOOK(sceKernelMmap);
    HOOK(socket);
    HOOK(__sys_socketex);
}

namespace Libraries::Kernel::Kernel {

void RegisterHooks() {
    return RegisterKernelHooks();
}
} // namespace Libraries::Kernel::Kernel