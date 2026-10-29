// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <chrono>
#include <orbis/Net.h>
#include <orbis/NpCommon.h>
#include <orbis/libkernel.h>

#include "common/alignment.h"
#include "common/logging/log.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_signaling/np_signaling_helpers.h"
#include "core/libraries/np/np_signaling/np_signaling_state.h"
#include "core/libraries/np/np_signaling/np_signaling_transport.h"

namespace Libraries::Np::NpSignaling {

bool g_signaling_heap_initialized = false;
void* g_signaling_heap_base = nullptr;
s64 g_signaling_heap_size = 0;
SignalingRuntimeHooks g_runtime_hooks{};
bool g_runtime_hooks_registered = false;
u64 g_echo_probe_word2 = 0;
u64 g_echo_probe_word3 = 0;
u64 g_echo_thread_handle = static_cast<u64>(-1);
s32 g_echo_thread_status = -1;

void SetRuntimeHooks(const SignalingRuntimeHooks& hooks) {
    g_runtime_hooks = hooks;
    g_runtime_hooks_registered = true;
}

s32 InitSignalingHeap(s64 pool_size) {
    LOG_DEBUG(Lib_NpSignaling, "pool_size={}", pool_size);

    if (pool_size == 0 || g_signaling_heap_initialized) {
        return ORBIS_NP_SIGNALING_INTERNAL_ERROR_ALLOCATOR;
    }

    const u64 aligned_size = Common::AlignUp(static_cast<u64>(pool_size), 0x4000);

    void* heap_base = nullptr;
    const s32 rc =
        sceKernelMapNamedFlexibleMemory(&heap_base, aligned_size, 3, 0, "SceNpSignaling");
    if (rc < 0) {
        return rc;
    }

    g_signaling_heap_initialized = true;
    g_signaling_heap_base = heap_base;
    g_signaling_heap_size = static_cast<s64>(aligned_size);
    return ORBIS_OK;
}

void ShutdownSignalingHeap() {
    if (!g_signaling_heap_initialized) {
        return;
    }

    g_signaling_heap_initialized = false;

    if (g_signaling_heap_base != nullptr) {
        sceKernelMunmap(g_signaling_heap_base, static_cast<u64>(g_signaling_heap_size));
        g_signaling_heap_base = nullptr;
        g_signaling_heap_size = 0;
    }
}

s32 StartMainRuntime(s32 thread_priority, s32 cpu_affinity_mask, s64 thread_stack_size) {
    LOG_DEBUG(Lib_NpSignaling,
              "thread_priority={} cpu_affinity_mask={} "
              "thread_stack_size={}",
              thread_priority, cpu_affinity_mask, thread_stack_size);

    if (!g_runtime_hooks_registered) {
        return ORBIS_NP_SIGNALING_INTERNAL_ERROR_NOT_INITIALIZED;
    }

    const s32 priority = thread_priority;
    const u64 affinity = static_cast<u64>(static_cast<u32>(cpu_affinity_mask));
    const u64 stack_size = static_cast<u64>(thread_stack_size);
    if (g_runtime_hooks.start_dispatch != nullptr) {
        g_runtime_hooks.start_dispatch(priority, affinity, stack_size);
    }
    return ORBIS_OK;
}

s32 StartEchoRuntime(s32 thread_priority, s32 cpu_affinity_mask) {
    LOG_DEBUG(Lib_NpSignaling, "thread_priority={} cpu_affinity_mask={}", thread_priority,
              cpu_affinity_mask);

    s32 sock = sceNetSocket("SceNpSignalingIoctl", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM_P2P, 0);

    std::array<u8, 36> ioctl_data{};
    const s32 ioctl_rc = sceNetIoctl(sock, 0xc02450ca, ioctl_data.data());
    if (ioctl_rc < 0) {
        LOG_WARNING(Lib_NpSignaling, "sceNetIoctl failed with error {:#x}", ioctl_rc);
    }
    sceNetSocketClose(sock);

    g_echo_probe_word2 = 0;
    g_echo_probe_word3 = 0;
    if (ioctl_rc >= 0) {
        std::memcpy(&g_echo_probe_word2, ioctl_data.data() + 16, sizeof(g_echo_probe_word2));
        std::memcpy(&g_echo_probe_word3, ioctl_data.data() + 24, sizeof(g_echo_probe_word3));
    }
    g_echo_thread_handle = static_cast<u64>(-1);
    g_echo_thread_status = -1;
    return ioctl_rc;
}

void ShutdownRuntime() {
    if (!g_runtime_hooks_registered) {
        g_echo_probe_word2 = 0;
        g_echo_probe_word3 = 0;
        g_echo_thread_handle = static_cast<u64>(-1);
        g_echo_thread_status = -1;
        return;
    }

    if (g_runtime_hooks.stop_dispatch != nullptr) {
        g_runtime_hooks.stop_dispatch();
    }

    g_echo_probe_word2 = 0;
    g_echo_probe_word3 = 0;
    g_echo_thread_handle = static_cast<u64>(-1);
    g_echo_thread_status = -1;
}

static void DispatchThreadMain() {
    NpSignalingState* state = GetState();
    for (;;) {
        QueuedDispatch dispatch;
        {
            std::unique_lock<std::mutex> lock(state->dispatch_mutex);
            for (;;) {
                if (state->dispatch_stop) {
                    return;
                }
                if (state->dispatch_queue.empty()) {
                    state->dispatch_cv.wait_for(lock, std::chrono::seconds(1), [&state] {
                        return state->dispatch_stop || !state->dispatch_queue.empty();
                    });
                    break;
                }
                const auto it = state->dispatch_queue.begin();
                const auto now = std::chrono::steady_clock::now();
                if (it->first > now) {
                    state->dispatch_cv.wait_until(lock, it->first);
                    break;
                }
                dispatch = std::move(it->second);
                state->dispatch_queue.erase(it);
                break;
            }
        }

        if (dispatch.callback) {
            LOG_INFO(Lib_NpSignaling, "t={} ctxId={} connId={} event={}({}) delay={}ms", NowMs(),
                     dispatch.ctx_id, dispatch.conn_id, dispatch.event_type,
                     SignalingEventName(dispatch.event_type), dispatch.delay_ms);
            dispatch.callback(static_cast<u32>(dispatch.ctx_id), static_cast<u32>(dispatch.conn_id),
                              dispatch.event_type, dispatch.error_code, dispatch.callback_arg);
        }
    }
}

static void* DispatchThreadFunc(void*) {
    DispatchThreadMain();
    return nullptr;
}

static void StartDispatchThread(s32 priority, u64 affinity_mask, u64 stack_size) {
    NpSignalingState* state = GetState();
    std::lock_guard<std::mutex> lock(state->dispatch_mutex);
    state->dispatch_stop = false;
    if (!state->dispatch_thread) {
        sceNpCreateThread(&state->dispatch_thread, DispatchThreadFunc, nullptr, priority,
                          stack_size, affinity_mask, "SceNpSignalingMain");
    }
}

static void StopDispatchThread() {
    NpSignalingState* state = GetState();
    {
        std::lock_guard<std::mutex> lock(state->dispatch_mutex);
        state->dispatch_stop = true;
        state->dispatch_queue.clear();
    }
    state->dispatch_cv.notify_all();
    if (state->dispatch_thread) {
        sceNpJoinThread(state->dispatch_thread, nullptr);
        state->dispatch_thread = {};
    }
}

void RegisterRuntimeHooks() {
    SetRuntimeHooks({
        .start_dispatch = StartDispatchThread,
        .stop_dispatch = StopDispatchThread,
    });
}

} // namespace Libraries::Np::NpSignaling
