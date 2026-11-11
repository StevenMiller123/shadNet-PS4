// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>

#include "common/types.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "shadnet/dialogs/message_dialog.h"

namespace ShadNet {

class FriendsPage {
public:
    explicit FriendsPage(MessageDialog& dialog) : m_dialog{dialog} {}

    void Run(s32 user_id);

private:
    using FriendInfo = Libraries::Np::NpHandler::FriendInfo;

    enum class Action {
        None,
        Remove,
        Block,
    };

    std::vector<FriendInfo> GetFriends();
    std::vector<std::string> GetBlocked();
    void ManageFriends();
    MessageDialog::Choice ReviewChanges(const std::vector<FriendInfo>& friends,
                                        const std::vector<Action>& actions);
    void ShowRequests();
    void ShowSentRequests();
    void ShowBlocked();
    void ManageBlocks();
    MessageDialog::Choice ReviewUnblocks(const std::vector<std::string>& blocked,
                                         const std::vector<bool>& unblock);

    MessageDialog& m_dialog;
    s32 m_user_id{-1};
};

} // namespace ShadNet
