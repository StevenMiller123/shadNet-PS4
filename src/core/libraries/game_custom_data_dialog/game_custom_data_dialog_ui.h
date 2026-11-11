// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "core/libraries/game_custom_data_dialog/game_custom_data_dialog.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "shadnet/dialogs/message_dialog.h"

namespace Libraries::GameCustomDataDialog {

struct DialogState {
    s32 mode = ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_INVALID;
    UserService::OrbisUserServiceUserId user_id = -1;
    void* callback_arg = nullptr;
    CommonDialog::OrbisCommonDialogResult result =
        CommonDialog::OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED;
    std::string message;
    std::string data_name;
    std::string data_detail;
    std::vector<std::pair<std::string, std::string>> localized_names;
    std::vector<std::pair<std::string, std::string>> localized_details;
    std::string data_url;
    bool with_item_id = false;
    std::vector<u8> game_data;
    std::vector<u8> thumbnail;
    s32 expire_minutes = 0;
    std::vector<std::string> platforms;
    bool user_editable = false;
    u32 user_max_count = 0;
    std::vector<std::string> fixed_online_ids;
    std::vector<Np::OrbisNpAccountId> fixed_account_ids;
    std::vector<std::string> sent_online_ids;
    std::vector<Np::OrbisNpAccountId> sent_account_ids;
};

class GameCustomDataDialogUi {
public:
    GameCustomDataDialogUi() = default;
    ~GameCustomDataDialogUi();
    GameCustomDataDialogUi(const GameCustomDataDialogUi&) = delete;
    GameCustomDataDialogUi& operator=(const GameCustomDataDialogUi&) = delete;

    void Start(std::atomic<CommonDialog::OrbisCommonDialogStatus>* status, DialogState* state);

    void Finish(CommonDialog::OrbisCommonDialogResult result);

    void Close();

    void Join();

private:
    void DialogThread();
    CommonDialog::OrbisCommonDialogResult RunSend();
    CommonDialog::OrbisCommonDialogResult RunRecv();
    std::string FormatSendSummary(const std::vector<std::string>& recipients);
    bool Send(const std::vector<std::string>& recipients);
    void EditMessage();

    std::atomic<CommonDialog::OrbisCommonDialogStatus>* m_status{};
    DialogState* m_state{};
    std::mutex m_finish_mutex;
    bool m_finished{};
    ShadNet::MessageDialog m_dialog;
    std::thread m_thread;
};

} // namespace Libraries::GameCustomDataDialog
