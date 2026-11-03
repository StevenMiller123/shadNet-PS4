// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::Kernel {

s32 setsockopt(s32 s, s32 level, s32 optname, const void* optval, u32 optlen);

void RegisterHooks();
} // namespace Libraries::Kernel