// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/kernel.h"

static void RegisterKernelHooks() {
    // If we need to hook anything more generic, it'll go here.
}

namespace Libraries::Kernel {

void RegisterHooks() {
    RegisterKernelHooks();
    RegisterSocketHooks();
}
} // namespace Libraries::Kernel
