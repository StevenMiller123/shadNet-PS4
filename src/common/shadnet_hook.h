#pragma once

#ifdef __cplusplus

#include <Common.h>

#define SHADNET_HOOK_DECLARE(name_space, func)                                                     \
    HOOK_INIT(func);                                                                               \
    static auto func##_hook = [](auto... args) {                                                   \
        return name_space::func(args...);                                                          \
    }

#define SHADNET_HOOK16(name_space, name) do { \
    klog("%s:%d HOOK16() Create " #name "\n", __FUNCTION__, __LINE__);  \
    Detour_Construct( (&(Detour_##name)), DetourMode_x32);                                 \
    Detour_DetourFunction16( (&(Detour_##name)), (uint64_t)name, (void *)((decltype(&name_space::name))(name##_hook)) ); \
} while (0)

#define SHADNET_HOOK32(name_space, name) do { \
    klog("%s:%d HOOK32() Create " #name "\n", __FUNCTION__, __LINE__);  \
    Detour_Construct( (&(Detour_##name)), DetourMode_x32);                                 \
    Detour_DetourFunction( (&(Detour_##name)), (uint64_t)name, (void *)((decltype(&name_space::name))(name##_hook)) ); \
} while (0)

#define SHADNET_HOOK64(name_space, name) do { \
    klog("%s:%d HOOK64() Create " #name "\n", __FUNCTION__, __LINE__);  \
    Detour_Construct( (&(Detour_##name)), DetourMode_x64);                                 \
    Detour_DetourFunction( (&(Detour_##name)), (uint64_t)name, (void *)((decltype(&name_space::name))(name##_hook)) ); \
} while (0)

#define SHADNET_HOOK(name_space, name) SHADNET_HOOK64(name_space, name)

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