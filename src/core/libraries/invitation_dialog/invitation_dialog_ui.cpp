// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <orbis/MsgDialog.h>

#include "common/logging/log.h"
#include "core/libraries/invitation_dialog/invitation_dialog_ui.h"

namespace Libraries::InvitationDialog {

using CommonDialog::OrbisCommonDialogResult;
using CommonDialog::OrbisCommonDialogStatus;
using Choice = ShadNet::MessageDialog::Choice;

static constexpr u64 MAX_DISPLAYED_MESSAGE_LENGTH = 256;

static std::string FormatUserMessage(const std::string& message) {
    if (message.empty()) {
        return {};
    }
    if (message.size() > MAX_DISPLAYED_MESSAGE_LENGTH) {
        return "\n\nMessage: " + message.substr(0, MAX_DISPLAYED_MESSAGE_LENGTH) + "...";
    }
    return "\n\nMessage: " + message;
}

static std::string JoinNames(const std::vector<std::string>& names) {
    std::string out;
    for (const auto& name : names) {
        if (!out.empty()) {
            out += '\n';
        }
        out += name;
    }
    return out;
}

InvitationDialogUi::~InvitationDialogUi() {
    m_dialog.Abort();
    Join();
}

void InvitationDialogUi::Start(std::atomic<OrbisCommonDialogStatus>* status, DialogState* state) {
    Join();
    m_status = status;
    m_state = state;
    m_finished = false;
    m_thread = std::thread(&InvitationDialogUi::DialogThread, this);
}

void InvitationDialogUi::Finish(OrbisCommonDialogResult result,
                                std::vector<std::string> sent_online_ids,
                                std::vector<Np::OrbisNpAccountId> sent_account_ids) {
    std::scoped_lock lk{m_finish_mutex};
    if (m_finished || m_state == nullptr || m_status == nullptr) {
        return;
    }
    m_finished = true;
    m_state->result = result;
    m_state->sent_online_ids = std::move(sent_online_ids);
    m_state->sent_account_ids = std::move(sent_account_ids);
    *m_status = OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_FINISHED;
}

void InvitationDialogUi::Close() {
    Finish(OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_OK);
    m_dialog.Abort();
}

void InvitationDialogUi::Join() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void InvitationDialogUi::DialogThread() {
    if (!m_dialog.Begin(m_state->user_id)) {
        Finish(OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED);
        return;
    }

    if (m_state->mode == ORBIS_INVITATION_DIALOG_MODE_SEND) {
        RunSend();
    } else {
        RunRecv();
    }

    m_dialog.End();
    std::scoped_lock lk{m_finish_mutex};
    if (!m_finished) {
        m_finished = true;
        *m_status = OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_FINISHED;
    }
}

void InvitationDialogUi::RunSend() {
    auto finish = [this](OrbisCommonDialogResult result, std::vector<std::string> online_ids = {},
                         std::vector<Np::OrbisNpAccountId> account_ids = {}) {
        m_dialog.End();
        Finish(result, std::move(online_ids), std::move(account_ids));
    };

    if (!m_state->user_editable) {
        std::vector<std::string> names;
        for (const auto& online_id : m_state->online_ids) {
            names.emplace_back(online_id.data, strnlen(online_id.data, sizeof(online_id.data)));
        }
        const std::string recipients =
            names.empty() ? fmt::format("{} player(s)", m_state->account_ids.size())
                          : JoinNames(names);
        const std::string msg = fmt::format("Send Invitation\n\nSend an invitation to:\n{}{}",
                                            recipients, FormatUserMessage(m_state->message));
        if (m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS, "Send", "Cancel") !=
            Choice::Button1) {
            finish(OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED);
            return;
        }
        SendInvitation(std::move(names));
        return;
    }

    const auto friends = Np::NpHandler::Instance().GetFriendList(m_state->user_id).friends;
    if (friends.empty()) {
        m_dialog.Show("Send Invitation\n\nNo friends available.\nAdd friends to invite them.",
                      ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
        finish(OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED);
        return;
    }

    std::vector<bool> selected(friends.size());
    u64 index = 0;
    while (true) {
        std::vector<std::string> names;
        for (u64 i = 0; i < friends.size(); i++) {
            if (selected[i]) {
                names.push_back(friends[i].npid);
            }
        }

        if (index < friends.size()) {
            const auto& friend_info = friends[index];
            const std::string msg =
                fmt::format("Send Invitation\n\n{} ({})\n\n({}/{})\nSelected: {}/{}",
                            friend_info.npid, friend_info.online ? "online" : "offline", index + 1,
                            friends.size(), names.size(), m_state->max_recipients);
            Choice choice = m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS, "Next",
                                          selected[index] ? "Remove" : "Add");
            if (choice == Choice::Button1) {
                index++;
            } else if (choice == Choice::Button2) {
                if (!selected[index] && names.size() >= m_state->max_recipients) {
                    m_dialog.Show(
                        fmt::format("You can invite up to {} player(s).", m_state->max_recipients),
                        ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
                } else {
                    selected[index] = !selected[index];
                }
            } else {
                finish(OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED);
                return;
            }
            continue;
        }

        if (names.empty()) {
            if (m_dialog.Show("Send Invitation\n\nNo players selected.",
                              ORBIS_MSG_DIALOG_BUTTON_TYPE_OK) != Choice::Button1) {
                finish(OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED);
                return;
            }
            index = 0;
            continue;
        }
        const std::string msg = fmt::format("Send Invitation\n\nSend an invitation to:\n{}{}",
                                            JoinNames(names), FormatUserMessage(m_state->message));
        Choice choice = m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS, "Send", "Back");
        if (choice == Choice::Button1) {
            SendInvitation(std::move(names));
            return;
        } else if (choice == Choice::Button2) {
            index = 0;
        } else {
            finish(OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED);
            return;
        }
    }
}

void InvitationDialogUi::SendInvitation(std::vector<std::string> online_ids) {
    std::vector<std::string> to = online_ids;
    for (const auto account_id : m_state->account_ids) {
        to.push_back(std::to_string(account_id));
    }

    const bool success = Np::NpHandler::Instance().SendSessionInvitation(
        m_state->user_id, m_state->session_id, to, m_state->message);
    if (!success) {
        m_dialog.Show("Failed to send the invitation.", ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
        m_dialog.End();
        Finish(OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_OK);
        return;
    }
    m_dialog.End();
    Finish(OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_OK, std::move(online_ids),
           m_state->account_ids);
}

void InvitationDialogUi::RunRecv() {
    auto& np_handler = Np::NpHandler::Instance();
    auto invitations = np_handler.GetPendingInvitations(m_state->user_id);
    if (invitations.empty()) {
        m_dialog.Show("Invitations\n\nYou have no pending invitations.",
                      ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
    }

    while (!invitations.empty()) {
        const auto invitation = invitations.front();
        const std::string msg = fmt::format(
            "Invitations\n\n{} invited you to a game session.\n\n(1/{})",
            invitation.from_npid.empty() ? "(unknown)" : invitation.from_npid, invitations.size());
        Choice choice =
            m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS, "Accept", "Decline");
        if (choice == Choice::Button1) {
            if (np_handler.AcceptSessionInvitation(m_state->user_id, invitation.invitation_id)) {
                m_dialog.End();
                Finish(OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_OK);
                return;
            }
            m_dialog.Show("This invitation is no longer available.",
                          ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
        } else if (choice == Choice::Button2) {
            np_handler.DeclineSessionInvitation(m_state->user_id, invitation.invitation_id);
        } else {
            break;
        }
        invitations.erase(invitations.begin());
    }

    m_dialog.End();
    Finish(OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED);
}

} // namespace Libraries::InvitationDialog
