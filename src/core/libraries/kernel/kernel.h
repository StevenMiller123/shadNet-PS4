// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::Kernel {

s32 socket(s32 family, s32 type, s32 protocol);
s32 __sys_socketex(const char* name, s32 family, s32 type, s32 protocol);

void RegisterHooks();
} // namespace Libraries::Kernel