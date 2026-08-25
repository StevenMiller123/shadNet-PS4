// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Extracted from
// https://github.com/red-prig/GoldHEN_Plugins_Repository/blob/dmem/plugin_src/dmem/source/main.c

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void Detour_WriteJump16(Detour* This, void* Address);
void* Detour_DetourFunction16(Detour* This, uint64_t FunctionPtr, void* HookPtr);

#ifdef __cplusplus
};
#endif

#define HOOK16(name)                                                                               \
    do {                                                                                           \
        klog("%s:%d HOOK16() Create " #name "\n", __FUNCTION__, __LINE__);                         \
        Detour_Construct((&(Detour_##name)), DetourMode_x32);                                      \
        Detour_DetourFunction16((&(Detour_##name)), (uint64_t)name, (void*)(&(name##_hook)));      \
    } while (0)