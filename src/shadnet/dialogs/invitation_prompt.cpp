// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>
#include <orbis/CommonDialog.h>
#include <orbis/ImeDialog.h>
#include <orbis/MsgDialog.h>
#include <orbis/libkernel.h>

#include "common/logging/log.h"
#include "core/libraries/invitation_dialog/invitation_dialog.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "shadnet/dialogs/invitation_prompt.h"
#include "shadnet/dialogs/plugin_menu.h"

namespace ShadNet {

static constexpr u64 MAX_PROMPTS = 4;
static constexpr u32 MAX_ATTEMPTS = 5;
static constexpr u32 WAIT_INTERVAL = 500000;

using Choice = MessageDialog::Choice;

InvitationPrompt& InvitationPrompt::GetInstance() {
    static InvitationPrompt s_instance;
    return s_instance;
}

void InvitationPrompt::Push(s32 user_id, std::string invitation_id, std::string session_id,
                            std::string from_npid) {
    LOG_DEBUG(shadNet, "{} invited you to a game session", from_npid);

    std::scoped_lock lk{m_mutex};
    std::erase_if(m_prompts, [&](const Prompt& p) { return p.invitation_id == invitation_id; });
    m_prompts.push_back(
        {user_id, std::move(invitation_id), std::move(session_id), std::move(from_npid)});
    while (m_prompts.size() > MAX_PROMPTS) {
        m_prompts.pop_front();
    }

    if (!m_thread_started) {
        m_thread_started = true;
        std::thread(&InvitationPrompt::PromptThread, this).detach();
    }
    m_cv.notify_one();
}

void InvitationPrompt::Dismiss(const std::string& invitation_id) {
    std::scoped_lock lk{m_mutex};
    std::erase_if(m_prompts, [&](const Prompt& p) { return p.invitation_id == invitation_id; });
    if (m_shown_invitation_id == invitation_id) {
        m_dialog.Abort();
    }
}

void InvitationPrompt::PromptThread() {
    while (true) {
        {
            std::unique_lock lk{m_mutex};
            m_cv.wait(lk, [this] { return !m_prompts.empty(); });
        }

        while (!CanShow()) {
            sceKernelUsleep(WAIT_INTERVAL);
        }

        Prompt prompt;
        {
            std::scoped_lock lk{m_mutex};
            if (m_prompts.empty()) {
                continue;
            }
            prompt = std::move(m_prompts.front());
            m_prompts.pop_front();
            m_shown_invitation_id = prompt.invitation_id;
        }
        ShowPrompt(prompt);
        {
            std::scoped_lock lk{m_mutex};
            m_shown_invitation_id.clear();
        }
    }
}

bool InvitationPrompt::CanShow() {
    return !PluginMenu::GetInstance().IsOpen() && !Libraries::InvitationDialog::IsRunning() &&
           sceMsgDialogGetStatus() != ORBIS_COMMON_DIALOG_STATUS_RUNNING &&
           sceImeDialogGetStatus() != ORBIS_DIALOG_STATUS_RUNNING;
}

void InvitationPrompt::ShowPrompt(const Prompt& prompt) {
    Choice choice = Choice::Failed;
    m_open = true;
    if (m_dialog.Begin(prompt.user_id)) {
        const std::string msg =
            fmt::format("{} invited you to a game session.\nWould you like to join?",
                        prompt.from_npid.empty() ? "(unknown)" : prompt.from_npid);
        choice = m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS, "Accept", "Decline");
        m_dialog.End();
    }
    m_open = false;

    auto& np_handler = Libraries::Np::NpHandler::Instance();
    if (choice == Choice::Button1) {
        if (np_handler.AcceptSessionInvitation(prompt.user_id, prompt.invitation_id)) {
            LOG_DEBUG(shadNet, "Joining {}'s session", prompt.from_npid);
        } else {
            LOG_NOTIFICATION(shadNet, "{}'s invitation is no longer valid", prompt.from_npid);
        }
    } else if (choice == Choice::Button2) {
        np_handler.DeclineSessionInvitation(prompt.user_id, prompt.invitation_id);
    } else if (choice == Choice::Failed && prompt.attempts + 1 < MAX_ATTEMPTS) {
        Prompt retry = prompt;
        retry.attempts++;
        std::scoped_lock lk{m_mutex};
        m_prompts.push_front(std::move(retry));
        sceKernelUsleep(WAIT_INTERVAL);
    }
}

} // namespace ShadNet
