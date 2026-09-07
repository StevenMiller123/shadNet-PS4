#pragma once

#ifdef __cplusplus

#include <Common.h>

#define SHADNET_HOOK_DECLARE(name_space, func)                                                     \
    HOOK_INIT(func);                                                                               \
    static auto func##_hook = [](auto... args) {                                                   \
        return name_space::func(args...);                                                          \
    }

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