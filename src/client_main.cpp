// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "client_main.h"
#include "common/logging/log.h"
#include "common/types.h"

extern "C" s32 client_start() {
    LOG_INFO("Starting shadNet Client");
    return 0;
}