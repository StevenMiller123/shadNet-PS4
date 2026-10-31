// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <sys/socket.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/kernel/kernel.h"

static void RegisterKernelHooks() {}

namespace Libraries::Kernel {

void RegisterHooks() {
    return RegisterKernelHooks();
}
} // namespace Libraries::Kernel
