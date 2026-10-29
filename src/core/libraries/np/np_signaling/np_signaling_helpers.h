// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::Np::NpSignaling {

struct SignalingRuntimeHooks {
    void (*start_dispatch)(s32 priority, u64 affinity_mask, u64 stack_size);
    void (*stop_dispatch)();
};

void SetRuntimeHooks(const SignalingRuntimeHooks& hooks);
s32 InitSignalingHeap(s64 pool_size);
void ShutdownSignalingHeap();
s32 StartMainRuntime(s32 thread_priority, s32 cpu_affinity_mask, s64 thread_stack_size);
s32 StartEchoRuntime(s32 thread_priority, s32 cpu_affinity_mask);
void ShutdownRuntime();

void RegisterRuntimeHooks();
} // namespace Libraries::Np::NpSignaling
