// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>

#include "common/types.h"
#include "shadnet/dialogs/message_dialog.h"

namespace ShadNet {

class InvitationPrompt {
public:
    static InvitationPrompt& GetInstance();

    InvitationPrompt(const InvitationPrompt&) = delete;
    InvitationPrompt& operator=(const InvitationPrompt&) = delete;

    void Push(s32 user_id, std::string invitation_id, std::string session_id,
              std::string from_npid);

    void Dismiss(const std::string& invitation_id);

    bool IsOpen() {
        return m_open;
    }

private:
    struct Prompt {
        s32 user_id;
        std::string invitation_id;
        std::string session_id;
        std::string from_npid;
        u32 attempts = 0;
    };

    InvitationPrompt() = default;
    ~InvitationPrompt() = default;

    void PromptThread();
    bool CanShow();
    void ShowPrompt(const Prompt& prompt);

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<Prompt> m_prompts;
    std::string m_shown_invitation_id;
    bool m_thread_started{};
    std::atomic<bool> m_open{false};
    MessageDialog m_dialog;
};

} // namespace ShadNet
