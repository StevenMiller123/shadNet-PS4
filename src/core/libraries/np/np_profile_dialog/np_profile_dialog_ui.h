// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include "core/libraries/np/np_profile_dialog/np_profile_dialog.h"
#include "shadnet/dialogs/message_dialog.h"

namespace Libraries::Np::NpProfileDialog {

struct DialogState {
    s32 mode = ORBIS_NP_PROFILE_DIALOG_MODE_INVALID;
    UserService::OrbisUserServiceUserId user_id = -1;
    std::string online_id;
    OrbisNpAccountId account_id = 0;
    void* user_data = nullptr;
    CommonDialog::OrbisCommonDialogResult result =
        CommonDialog::OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_OK;
};

class NpProfileDialogUi {
public:
    NpProfileDialogUi() = default;
    ~NpProfileDialogUi();
    NpProfileDialogUi(const NpProfileDialogUi&) = delete;
    NpProfileDialogUi& operator=(const NpProfileDialogUi&) = delete;

    void Start(std::atomic<CommonDialog::OrbisCommonDialogStatus>* status, DialogState* state);

    void Finish(CommonDialog::OrbisCommonDialogResult result);

    void Close();

    void Join();

private:
    void DialogThread();
    std::string ResolveOnlineId();
    std::string FormatRelation(const std::string& online_id);
    CommonDialog::OrbisCommonDialogResult ShowProfile(const std::string& online_id);
    CommonDialog::OrbisCommonDialogResult ConfirmAction(const std::string& online_id);

    std::atomic<CommonDialog::OrbisCommonDialogStatus>* m_status{};
    DialogState* m_state{};
    std::mutex m_finish_mutex;
    bool m_finished{};
    ShadNet::MessageDialog m_dialog;
    std::thread m_thread;
};

} // namespace Libraries::Np::NpProfileDialog
