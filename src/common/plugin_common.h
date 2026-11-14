// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// From https://github.com/GoldHEN/GoldHEN_Plugins_Repository/blob/main/common/plugin_common.h

#pragma once

#include "common/libjbc/jailbreak.h"

#include <Common.h>
#include "git_ver.h"
#include "shadnet_hook.h"

#define GOLDHEN_PATH "/data/GoldHEN"

#define TEX_ICON_SYSTEM "cxml://psnotification/tex_icon_system"
#define MAX_PATH_ 260

#define attr_module_hidden __attribute__((weak)) __attribute__((visibility("hidden")))
#define attr_public __attribute__((visibility("default")))
#define attr_aligned __attribute__((__force_align_arg_pointer__))

#if (__FINAL__) == 1
#define BUILD_TYPE "(Release)"
#define debug_printf(a, args...)
#else
#define BUILD_TYPE "(Debug)"
#define debug_printf(a, args...) klog("[%s] (%s:%d) " a, __func__, __FILE__, __LINE__, ##args)
#endif

#define final_printf(a, args...) klog("(%s:%d) " a, __FILE__, __LINE__, ##args)
#define boot_ver()                                                                                 \
    {                                                                                              \
        final_printf("Commit: %s Branch: %s Build: %i %s\n", GIT_COMMIT, GIT_VER, GIT_NUM,         \
                     BUILD_TYPE);                                                                  \
        final_printf("Built: %s\n", BUILD_DATE);                                                   \
        final_printf("GoldHEN SDK Ver: 0x%08x\n", GOLDHEN_SDK_VERSION);                            \
    }

#define print_proc_info()                                                                          \
    {                                                                                              \
        final_printf("process info\n");                                                            \
        final_printf("pid: %d\n", procInfo.pid);                                                   \
        final_printf("name: %s\n", procInfo.name);                                                 \
        final_printf("path: %s\n", procInfo.path);                                                 \
        final_printf("titleid: %s\n", procInfo.titleid);                                           \
        final_printf("contentid: %s\n", procInfo.contentid);                                       \
        final_printf("version: %s\n", procInfo.version);                                           \
        final_printf("base_address: 0x%lx\n", procInfo.base_address);                              \
    }

void NotifyStatic(const char* IconUri, const char* text);
void Notify(const char* IconUri, const char* FMT, ...);

#ifdef __cplusplus
namespace Common {
bool IsPs5();
bool DebugFlagSet(const char* name);
bool JailbreakViaDaemon();
void RejailViaDaemon();

template <bool change_root, typename F, typename... Args>
auto RunPrivilegedImpl(F func, Args... args) -> decltype(func(args...)) {
    using ret_type = decltype(func(args...));
    const bool use_libjbc = !Common::IsPs5();
    const bool skip_daemon = !use_libjbc && Common::DebugFlagSet("no_jailbreak");
    const bool use_daemon = !use_libjbc && !skip_daemon;

    // todo there's no logic for leaving the root as is on the PS5!
    bool const daemon_success = use_daemon && Common::JailbreakViaDaemon();
    if (skip_daemon) {
        LOG_DEBUG(shadNet, "Daemon jailbreak skipped by the no_jailbreak flag");
    } else if (use_daemon) {
        LOG_DEBUG(shadNet, "PS5 jailbreak through the daemon {}",
                  daemon_success ? "succeeded" : "failed");
    }
    jbc_cred cred;
    if (use_libjbc) {
        if constexpr (change_root) {
            jbc_get_cred(&cred);
            jbc_cred root_cred = cred;
            jbc_jailbreak_cred(&root_cred);
            root_cred.cdir = cred.rdir;
            jbc_set_cred(&root_cred);
        } else {
            jbc_get_cred(&cred);
            // Hardcoded cred values come from libjbc's jbc_jailbreak_cred call.
            // jbc_jailbreak_cred also changes the process's root directory, we do not want that.
            jbc_cred root_cred = cred;
            root_cred.sceProcCap = 0xffffffffffffffff;
            root_cred.sceProcType = 0x3801000000000013;
            root_cred.sonyCred = 0xffffffffffffffff;
            jbc_set_cred(&root_cred);
        }
    }

    std::conditional_t<std::is_void_v<ret_type>, u8, ret_type> ret;
    if constexpr (std::is_void_v<ret_type>) {
        func(args...);
    } else {
        ret = func(args...);
    }

    // Un-jailbreak the process by restoring the original cred.
    if (use_libjbc) {
        jbc_set_cred(&cred);
    } else if (daemon_success) {
        Common::RejailViaDaemon();
    }

    if constexpr (std::is_void_v<decltype(func(args...))>) {
        return;
    } else {
        return ret;
    }
}

template <typename F, typename... Args>
auto RunPrivileged(F func, Args... args) -> decltype(func(args...)) {
    return RunPrivilegedImpl<false>(func, args...);
}

template <typename F, typename... Args>
auto RunPrivilegedRoot(F func, Args... args) -> decltype(func(args...)) {
    return RunPrivilegedImpl<true>(func, args...);
}

template <bool change_root, typename F, typename... Args>
auto RetryPrivilegedImpl(F func, Args... args) {
    if (func(args...)) {
        return;
    }
    RunPrivilegedImpl<change_root>(func, args...);
}
template <typename F, typename... Args>
auto RetryPrivileged(F func, Args... args) {
    RetryPrivilegedImpl<false>(func, args...);
}
template <typename F, typename... Args>
auto RetryPrivilegedRoot(F func, Args... args) {
    RetryPrivilegedImpl<true>(func, args...);
}

} // namespace Common
#endif

// Takes hardcoded input string 2 to strlen against during compile time.
// startsWith(input_1, "input 2");
#define startsWith(str1, str2) (strncmp(str1, str2, __builtin_strlen(str2)) == 0)
