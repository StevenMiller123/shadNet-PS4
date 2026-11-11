// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/MsgDialog.h>

#include "common/logging/log.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_profile_dialog/np_profile_dialog_ui.h"
#include "core/libraries/np/np_signaling/np_signaling_handler.h"

namespace Libraries::Np::NpProfileDialog {

using CommonDialog::OrbisCommonDialogResult;
using CommonDialog::OrbisCommonDialogStatus;
using Choice = ShadNet::MessageDialog::Choice;

static std::string ToString(const OrbisNpOnlineId& online_id) {
    return std::string(online_id.data, strnlen(online_id.data, sizeof(online_id.data)));
}

static bool Contains(const std::vector<std::string>& list, const std::string& value) {
    return std::find(list.begin(), list.end(), value) != list.end();
}

NpProfileDialogUi::~NpProfileDialogUi() {
    m_dialog.Abort();
    Join();
}

void NpProfileDialogUi::Start(std::atomic<OrbisCommonDialogStatus>* status, DialogState* state) {
    Join();
    m_status = status;
    m_state = state;
    m_finished = false;
    m_thread = std::thread(&NpProfileDialogUi::DialogThread, this);
}

void NpProfileDialogUi::Finish(OrbisCommonDialogResult result) {
    std::scoped_lock lk{m_finish_mutex};
    if (m_finished || m_state == nullptr || m_status == nullptr) {
        return;
    }
    m_finished = true;
    m_state->result = result;
    *m_status = OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_FINISHED;
}

void NpProfileDialogUi::Close() {
    Finish(OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_OK);
    m_dialog.Abort();
}

void NpProfileDialogUi::Join() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void NpProfileDialogUi::DialogThread() {
    if (!m_dialog.Begin(m_state->user_id)) {
        Finish(OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED);
        return;
    }

    const std::string online_id = ResolveOnlineId();
    const OrbisCommonDialogResult result = m_state->mode == ORBIS_NP_PROFILE_DIALOG_MODE_NORMAL
                                               ? ShowProfile(online_id)
                                               : ConfirmAction(online_id);
    m_dialog.End();
    Finish(result);
}

std::string NpProfileDialogUi::ResolveOnlineId() {
    if (!m_state->online_id.empty()) {
        return m_state->online_id;
    }

    // shadNet has no account ID lookup, so look for the player among known users.
    auto& np_handler = NpHandler::Instance();
    if (m_state->account_id == np_handler.GetAccountId(m_state->user_id)) {
        return ToString(np_handler.GetNpId(m_state->user_id).handle);
    }
    OrbisNpId np_id{};
    if (NpSignaling::FindMatching2MemberNpId(m_state->account_id, &np_id)) {
        return ToString(np_id.handle);
    }
    LOG_WARNING(Lib_NpProfileDialog, "Could not retrieve online id for account id {:#x}", m_state->account_id);
    return {};
}

std::string NpProfileDialogUi::FormatRelation(const std::string& online_id) {
    auto& np_handler = NpHandler::Instance();
    if (online_id == ToString(np_handler.GetNpId(m_state->user_id).handle)) {
        return "This is you.";
    }

    const auto state = np_handler.GetFriendList(m_state->user_id);
    const auto it = std::find_if(state.friends.begin(), state.friends.end(),
                                 [&](const auto& info) { return info.npid == online_id; });
    if (it != state.friends.end()) {
        return fmt::format("Friend ({})", it->online ? "Online" : "Offline");
    }
    if (Contains(state.blocked, online_id)) {
        return "Blocked";
    }
    if (Contains(state.requests_received, online_id)) {
        return "Sent you a friend request";
    }
    if (Contains(state.requests_sent, online_id)) {
        return "Friend request sent";
    }
    return "Player";
}

OrbisCommonDialogResult NpProfileDialogUi::ShowProfile(const std::string& online_id) {
    const std::string msg =
        online_id.empty() ? "Profile\n\nThis player's profile isn't available."
                          : fmt::format("Profile\n\n{}\n{}", online_id, FormatRelation(online_id));
    const Choice choice = m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
    return choice == Choice::Button1
               ? OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_OK
               : OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED;
}

OrbisCommonDialogResult NpProfileDialogUi::ConfirmAction(const std::string& online_id) {
    const auto canceled = OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED;
    auto& np_handler = NpHandler::Instance();
    if (online_id.empty()) {
        m_dialog.Show("Profile\n\nThis player's profile isn't available.",
                      ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
        return canceled;
    }
    if (!np_handler.IsSignedIn(m_state->user_id)) {
        m_dialog.Show("Sign in to shadNet to manage your friends.",
                      ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
        return canceled;
    }

    const auto state = np_handler.GetFriendList(m_state->user_id);
    const bool is_self = online_id == ToString(np_handler.GetNpId(m_state->user_id).handle);
    const bool is_friend = std::any_of(state.friends.begin(), state.friends.end(),
                                       [&](const auto& info) { return info.npid == online_id; });
    const bool is_blocked = Contains(state.blocked, online_id);

    std::string error;
    std::string question;
    switch (m_state->mode) {
    case ORBIS_NP_PROFILE_DIALOG_MODE_FRIEND_REQUEST:
        if (is_self) {
            error = "You can't send a friend request to yourself.";
        } else if (is_friend) {
            error = fmt::format("{} is already your friend.", online_id);
        } else if (is_blocked) {
            error = fmt::format("Unblock {} before sending a friend request.", online_id);
        } else if (Contains(state.requests_sent, online_id)) {
            error = fmt::format("You already sent {} a friend request.", online_id);
        } else {
            question = fmt::format("Send a friend request to {}?", online_id);
        }
        break;
    case ORBIS_NP_PROFILE_DIALOG_MODE_ADD_TO_BLOCK_LIST:
        if (is_self) {
            error = "You can't block yourself.";
        } else if (is_blocked) {
            error = fmt::format("{} is already blocked.", online_id);
        } else {
            question = fmt::format("Block {}?", online_id);
        }
        break;
    case ORBIS_NP_PROFILE_DIALOG_MODE_GRIEF_REPORT:
        error = "Reporting players isn't supported on shadNet.";
        break;
    default:
        LOG_ERROR(Lib_NpProfileDialog, "unknown mode {}", m_state->mode);
        return canceled;
    }

    if (!error.empty()) {
        m_dialog.Show(fmt::format("Profile\n\n{}", error), ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
        return canceled;
    }
    if (m_dialog.Show(question, ORBIS_MSG_DIALOG_BUTTON_TYPE_YESNO) != Choice::Button1) {
        return canceled;
    }

    const s32 result = m_state->mode == ORBIS_NP_PROFILE_DIALOG_MODE_FRIEND_REQUEST
                           ? np_handler.SendFriendRequest(m_state->user_id, online_id)
                           : np_handler.BlockUser(m_state->user_id, online_id);
    if (result != ORBIS_OK) {
        LOG_ERROR(Lib_NpProfileDialog, "friend list update failed with error {:#x}",
                  static_cast<u32>(result));
        m_dialog.Show("Couldn't reach shadNet.", ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
        return canceled;
    }
    return OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_OK;
}

} // namespace Libraries::Np::NpProfileDialog
