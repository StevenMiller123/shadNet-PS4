// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <orbis/Pad.h>
#include "common/types.h"

namespace Libraries::Pad {

s32 scePadRead(s32 handle, OrbisPadData* data, s32 num);
s32 scePadReadState(s32 handle, OrbisPadData* data);

void RegisterHooks();
} // namespace Libraries::Pad
