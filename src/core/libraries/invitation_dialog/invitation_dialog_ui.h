// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "core/libraries/invitation_dialog/invitation_dialog.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "shadnet/dialogs/message_dialog.h"

namespace Libraries::InvitationDialog {

struct DialogState {
    s32 mode = ORBIS_INVITATION_DIALOG_MODE_INVALID;
    CommonDialog::OrbisCommonDialogResult result =
        CommonDialog::OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_OK;
    bool user_editable = false;
    u32 max_recipients = ORBIS_INVITATION_DIALOG_ADDRESS_USER_LIST_MAX_SIZE;
    std::string message;
    UserService::OrbisUserServiceUserId user_id = -1;
    std::string session_id;
    void* callback_arg = nullptr;
    s32 error_code = 0;
    std::vector<Np::OrbisNpOnlineId> online_ids;
    std::vector<Np::OrbisNpAccountId> account_ids;
    std::vector<std::string> sent_online_ids;
    std::vector<Np::OrbisNpAccountId> sent_account_ids;
};

class InvitationDialogUi {
public:
    InvitationDialogUi() = default;
    ~InvitationDialogUi();
    InvitationDialogUi(const InvitationDialogUi&) = delete;
    InvitationDialogUi& operator=(const InvitationDialogUi&) = delete;

    void Start(std::atomic<CommonDialog::OrbisCommonDialogStatus>* status, DialogState* state);

    void Finish(CommonDialog::OrbisCommonDialogResult result,
                std::vector<std::string> sent_online_ids = {},
                std::vector<Np::OrbisNpAccountId> sent_account_ids = {});

    void Close();

    void Join();

private:
    void DialogThread();
    void RunSend();
    void RunRecv();
    void SendInvitation(std::vector<std::string> online_ids);

    std::atomic<CommonDialog::OrbisCommonDialogStatus>* m_status{};
    DialogState* m_state{};
    std::mutex m_finish_mutex;
    bool m_finished{};
    ShadNet::MessageDialog m_dialog;
    std::thread m_thread;
};

} // namespace Libraries::InvitationDialog
