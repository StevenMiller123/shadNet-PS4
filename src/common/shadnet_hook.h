#pragma once

#ifdef __cplusplus

#include <Common.h>

#define SHADNET_HOOK_DECLARE(name_space, func)                                                     \
    HOOK_INIT(func);                                                                               \
    template <class ReturnType, class... Args>                                                     \
    static ReturnType func##_hook(Args... args) {                                                  \
        return name_space::func(args...);                                                          \
    }

#define SHADNET_HOOK_DECLARE1(name_space, func)                                                    \
    HOOK_INIT(func);                                                                               \
    template <class ReturnType, class... Args>                                                     \
    static ReturnType func##_hook(Args... args) {                                                  \
        return name_space::func(args...);                                                          \
    }

#define SHADNET_HOOK(name_space, name)                                                             \
    do {                                                                                           \
        LOG_INFO(Hooks, "Creating hook for {}", #name);                                            \
        Detour_Construct((&(Detour_##name)), DetourMode_x64);                                      \
        Detour_DetourFunction((&(Detour_##name)), (uint64_t)::name, (void*)(&name_space::name));    \
    } while (0)

#define SHADNET_HOOK1(name)                                                             \
    do {                                                                                           \
        LOG_INFO(Hooks, "Creating hook for {}", #name);                                            \
        Detour_Construct((&(Detour_##name)), DetourMode_x64);                                      \
        Detour_DetourFunction((&(Detour_##name)), (uint64_t)::name, (void*)(&name));    \
    } while (0)

template <auto f>
struct HookContinueWrapperImpl;

template <class ReturnType, class... Args, ReturnType (*func)(Args...)>
struct HookContinueWrapperImpl<func> {
    static ReturnType wrap(Detour detour, Args... args) {
        return Detour_Stub((&detour), ReturnType (*)(Args...), args...);
    }
};

#define SHADNET_HOOK_CONTINUE(func, ...)                                                           \
    HookContinueWrapperImpl<func>::wrap(Detour_##func, __VA_ARGS__)

#endif