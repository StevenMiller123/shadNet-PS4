// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <orbis/Pad.h>
#include "common/enum.h"
#include "common/types.h"

namespace Libraries::Pad {

enum class Button : u32 {
    None = 0,
    L3 = ORBIS_PAD_BUTTON_L3,
    R3 = ORBIS_PAD_BUTTON_R3,
    Options = ORBIS_PAD_BUTTON_OPTIONS,
    Up = ORBIS_PAD_BUTTON_UP,
    Right = ORBIS_PAD_BUTTON_RIGHT,
    Down = ORBIS_PAD_BUTTON_DOWN,
    Left = ORBIS_PAD_BUTTON_LEFT,
    L2 = ORBIS_PAD_BUTTON_L2,
    R2 = ORBIS_PAD_BUTTON_R2,
    L1 = ORBIS_PAD_BUTTON_L1,
    R1 = ORBIS_PAD_BUTTON_R1,
    Triangle = ORBIS_PAD_BUTTON_TRIANGLE,
    Circle = ORBIS_PAD_BUTTON_CIRCLE,
    Cross = ORBIS_PAD_BUTTON_CROSS,
    Square = ORBIS_PAD_BUTTON_SQUARE,
    TouchPad = ORBIS_PAD_BUTTON_TOUCH_PAD
};
DECLARE_ENUM_FLAG_OPERATORS(Button)

s32 scePadRead(s32 handle, OrbisPadData* data, s32 num);
s32 scePadReadState(s32 handle, OrbisPadData* data);

void RegisterHooks();
} // namespace Libraries::Pad
