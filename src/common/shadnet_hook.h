// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef __cplusplus

#include <Common.h>
#include "common/logging/log.h"

#define SHADNET_HOOK_DECLARE(name_space, func)                                                     \
    HOOK_INIT(func);                                                                               \
    static auto func##_hook = [](auto... args) { return name_space::func(args...); }

#define SHADNET_HOOK SHADNET_HOOK32

#define SHADNET_HOOK64(name_space, name)                                                           \
    do {                                                                                           \
        LOG_DEBUG(shadNet, "HOOK64() Create {}", #name);                                           \
        Detour_Construct((&(Detour_##name)), DetourMode_x64);                                      \
        Detour_DetourFunction((&(Detour_##name)), (uint64_t)name,                                  \
                              (void*)((decltype(&name_space::name))(name##_hook)));                \
    } while (0)

#define SHADNET_HOOK32(name_space, name)                                                           \
    do {                                                                                           \
        LOG_DEBUG(shadNet, "HOOK32() Create {}", #name);                                           \
        Detour_Construct((&(Detour_##name)), DetourMode_x32);                                      \
        Detour_DetourFunction((&(Detour_##name)), (uint64_t)name,                                  \
                              (void*)((decltype(&name_space::name))(name##_hook)));                \
    } while (0)

#define SHADNET_HOOK16(name_space, name)                                                           \
    do {                                                                                           \
        LOG_DEBUG(shadNet, "HOOK16() Create {}", #name);                                           \
        Detour_Construct((&(Detour_##name)), DetourMode_x16);                                      \
        Detour_DetourFunction((&(Detour_##name)), (uint64_t)name,                                  \
                              (void*)((decltype(&name_space::name))(name##_hook)));                \
    } while (0)

template <auto f>
struct HookContinueWrapperImpl;

template <class ReturnType, class... Args, ReturnType (*func)(Args...)>
struct HookContinueWrapperImpl<func> {
    static ReturnType wrap(Detour* detour, Args... args) {
        return Detour_Stub(detour, ReturnType (*)(Args...), args...);
    }
};

#define SHADNET_HOOK_CONTINUE(func, ...)                                                           \
    HookContinueWrapperImpl<func>::wrap(&Detour_##func, __VA_ARGS__)

#endif
