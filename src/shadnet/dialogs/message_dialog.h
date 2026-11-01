// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <string>

#include "common/types.h"

namespace ShadNet {

class MessageDialog {
public:
    enum class Choice {
        Button1,
        Button2,
        Canceled,
        Failed,
    };

    bool Begin(s32 user_id);
    void End();

    Choice Show(const std::string& msg, s32 button_type, const char* button1 = nullptr,
                const char* button2 = nullptr);
    void Abort();

private:
    s32 m_user_id{-1};
    bool m_owns_msg_dialog{};
    std::atomic<bool> m_aborted{};
};

} // namespace ShadNet
