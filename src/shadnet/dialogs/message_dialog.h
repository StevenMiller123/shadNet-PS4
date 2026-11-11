// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <optional>
#include <string>
#include <orbis/ImeDialog.h>

#include "common/enum.h"
#include "common/types.h"
#include "core/libraries/pad/pad.h"

namespace ShadNet {

class MessageDialog {
public:
    enum class Choice {
        // Dialog returns
        Button1,
        Button2,
        Canceled,
        Failed,

        // Possible pad inputs
        Left,
        Right,
        Up,
        Down,
        Circle,
        Cross,
        Triangle,
        Square,
        L1,
        L2,
        L3,
        R1,
        R2,
        R3,
        Options,
        TouchPad
    };

    bool Begin(s32 user_id);
    void End();

    Choice Show(const std::string& msg, s32 button_type, const char* button1 = nullptr,
                const char* button2 = nullptr,
                Libraries::Pad::Button buttons = Libraries::Pad::Button::None);
    std::optional<std::string> ShowKeyboard(const char* title, const std::string& value,
                                            OrbisImeType type, bool password = false,
                                            u32 max_length = 128);
    void Abort();

private:
    s32 m_user_id{-1};
    bool m_owns_msg_dialog{};
    std::atomic<bool> m_aborted{};
};

} // namespace ShadNet
