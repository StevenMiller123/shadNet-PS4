// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cctype>
#include <cstring>
#include <orbis/MsgDialog.h>

#include "common/logging/log.h"
#include "shadnet/dialogs/friends_page.h"

namespace ShadNet {

static constexpr u32 MIN_NPID_LENGTH = 3;
static constexpr u32 MAX_NPID_LENGTH = 16;

using Choice = MessageDialog::Choice;
using Button = Libraries::Pad::Button;

static std::string JoinNames(const std::vector<std::string>& names) {
    std::string text;
    for (const auto& name : names) {
        text += text.empty() ? name : ", " + name;
    }
    return text;
}

std::vector<FriendsPage::FriendInfo> FriendsPage::GetFriends() {
    auto friends = Libraries::Np::NpHandler::Instance().GetFriendList(m_user_id).friends;
    std::stable_sort(friends.begin(), friends.end(), [](const auto& a, const auto& b) {
        return a.online != b.online ? a.online : a.npid < b.npid;
    });
    return friends;
}

void FriendsPage::Run(s32 user_id) {
    m_user_id = user_id;
    auto& np_handler = Libraries::Np::NpHandler::Instance();
    if (!np_handler.IsSignedIn(user_id)) {
        m_dialog.Show("Sign in to shadNet to use your friends list.",
                      ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
        return;
    }

    while (true) {
        const auto friends = GetFriends();
        const u64 requests = np_handler.GetFriendList(user_id).requests_received.size();

        std::string msg =
            "Friends\nPress L1 to send friend requests\nPress R1 to view blocked users\n\n";
        if (requests > 0) {
            msg += fmt::format("{} pending friend request(s)\n\n", requests);
        }
        if (friends.empty()) {
            msg += "You haven't added any friends yet.";
        }
        for (const auto& info : friends) {
            msg += fmt::format("{}: {}\n", info.npid, info.online ? "Online" : "Offline");
        }

        Choice choice = m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS, "Manage Friends",
                                      "Friend Requests", Button::L1 | Button::R1);
        if (choice == Choice::Button1) {
            ManageFriends();
        } else if (choice == Choice::Button2) {
            ShowRequests();
        } else if (choice == Choice::L1) {
            ShowSentRequests();
        } else if (choice == Choice::R1) {
            ShowBlocked();
        } else {
            return;
        }
    }
}

void FriendsPage::ManageFriends() {
    const auto friends = GetFriends();
    if (friends.empty()) {
        m_dialog.Show("You haven't added any friends yet.", ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
        return;
    }

    std::vector<Action> actions(friends.size(), Action::None);
    const u64 page_count = friends.size() + 1;
    u64 index = 0;
    while (true) {
        if (index == friends.size()) {
            Choice choice = ReviewChanges(friends, actions);
            if (choice == Choice::L1) {
                index = friends.size() - 1;
            } else if (choice == Choice::R1) {
                index = 0;
            } else {
                return;
            }
            continue;
        }

        const FriendInfo& info = friends[index];
        Action& action = actions[index];
        const std::string msg =
            fmt::format("{}\n{}\n\nSelected: {}\n\n({}/{})\nPress L1 or R1 to navigate", info.npid,
                        info.online ? "Online" : "Offline",
                        action == Action::Remove  ? "Remove"
                        : action == Action::Block ? "Block"
                                                  : "None",
                        index + 1, friends.size());
        Choice choice = m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS,
                                      action == Action::Remove ? "Don't Remove" : "Remove",
                                      action == Action::Block ? "Don't Block" : "Block",
                                      Button::L1 | Button::R1);
        if (choice == Choice::Button1) {
            action = action == Action::Remove ? Action::None : Action::Remove;
        } else if (choice == Choice::Button2) {
            action = action == Action::Block ? Action::None : Action::Block;
        } else if (choice == Choice::L1) {
            index = (index + page_count - 1) % page_count;
        } else if (choice == Choice::R1) {
            index = (index + 1) % page_count;
        } else {
            return;
        }
    }
}

MessageDialog::Choice FriendsPage::ReviewChanges(const std::vector<FriendInfo>& friends,
                                                 const std::vector<Action>& actions) {
    std::vector<std::string> to_remove;
    std::vector<std::string> to_block;
    for (u64 i = 0; i < friends.size(); i++) {
        if (actions[i] == Action::Remove) {
            to_remove.push_back(friends[i].npid);
        } else if (actions[i] == Action::Block) {
            to_block.push_back(friends[i].npid);
        }
    }

    std::string msg = "Review changes\n\n";
    if (to_remove.empty() && to_block.empty()) {
        msg += "No changes selected.\n";
    }
    if (!to_remove.empty()) {
        msg += fmt::format("Remove: {}\n", JoinNames(to_remove));
    }
    if (!to_block.empty()) {
        msg += fmt::format("Block: {}\n", JoinNames(to_block));
    }
    msg += "\nPress L1 or R1 to navigate";

    Choice choice = m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS, "Accept", "Cancel",
                                  Button::L1 | Button::R1);
    if (choice != Choice::Button1) {
        return choice;
    }

    auto& np_handler = Libraries::Np::NpHandler::Instance();
    bool sent = true;
    for (const auto& npid : to_remove) {
        sent &= np_handler.RemoveFriend(m_user_id, npid) == ORBIS_OK;
    }
    for (const auto& npid : to_block) {
        sent &= np_handler.BlockUser(m_user_id, npid) == ORBIS_OK;
    }
    if (!sent) {
        m_dialog.Show("Some changes couldn't be sent to shadNet.", ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
    }
    return choice;
}

void FriendsPage::ShowRequests() {
    auto& np_handler = Libraries::Np::NpHandler::Instance();
    u64 index = 0;
    while (true) {
        const auto requests = np_handler.GetFriendList(m_user_id).requests_received;
        if (requests.empty()) {
            m_dialog.Show("You have no pending friend requests.", ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
            return;
        }
        index = std::min<u64>(index, requests.size() - 1);

        const std::string& npid = requests[index];
        const std::string msg =
            fmt::format("Friend request from {}\n\n({}/{})\nPress L1 or R1 to navigate", npid,
                        index + 1, requests.size());
        Choice choice = m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS, "Accept",
                                      "Ignore", Button::L1 | Button::R1);
        if (choice == Choice::Button1 || choice == Choice::Button2) {
            const s32 result = choice == Choice::Button1
                                   ? np_handler.SendFriendRequest(m_user_id, npid)
                                   : np_handler.RemoveFriend(m_user_id, npid);
            if (result != ORBIS_OK) {
                m_dialog.Show("Couldn't send your response to shadNet.",
                              ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
                return;
            }
        } else if (choice == Choice::L1) {
            index = (index + requests.size() - 1) % requests.size();
        } else if (choice == Choice::R1) {
            index = (index + 1) % requests.size();
        } else {
            return;
        }
    }
}

static bool IsValidNpid(const std::string& npid) {
    return npid.size() >= MIN_NPID_LENGTH && npid.size() <= MAX_NPID_LENGTH &&
           std::all_of(npid.begin(), npid.end(), [](char c) {
               return std::isalnum(static_cast<u8>(c)) || c == '-' || c == '_';
           });
}

void FriendsPage::ShowSentRequests() {
    auto& np_handler = Libraries::Np::NpHandler::Instance();
    while (true) {
        auto sent = np_handler.GetFriendList(m_user_id).requests_sent;
        std::sort(sent.begin(), sent.end());

        std::string msg = "Sent friend requests\nPress R1 to return to your friends list\n\n";
        if (sent.empty()) {
            msg += "You have no pending sent requests.";
        }
        for (const auto& npid : sent) {
            msg += npid + "\n";
        }

        Choice choice = m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS, "Send Request",
                                      "Back", Button::R1);
        if (choice != Choice::Button1) {
            return;
        }

        const auto npid = m_dialog.ShowKeyboard("Send a friend request to", "",
                                                ORBIS_TYPE_BASIC_LATIN, false, MAX_NPID_LENGTH);
        if (!npid || npid->empty()) {
            continue;
        }

        const auto state = np_handler.GetFriendList(m_user_id);
        const auto own_id = np_handler.GetNpId(m_user_id);
        const std::string own_npid(own_id.handle.data,
                                   strnlen(own_id.handle.data, sizeof(own_id.handle.data)));
        const auto contains = [&](const std::vector<std::string>& list) {
            return std::find(list.begin(), list.end(), *npid) != list.end();
        };
        std::string error;
        if (!IsValidNpid(*npid)) {
            error = fmt::format("An NPID is {} to {} letters, digits, hyphens and underscores.",
                                MIN_NPID_LENGTH, MAX_NPID_LENGTH);
        } else if (*npid == own_npid) {
            error = "You can't send a friend request to yourself.";
        } else if (std::any_of(state.friends.begin(), state.friends.end(),
                               [&](const FriendInfo& f) { return f.npid == *npid; })) {
            error = fmt::format("{} is already your friend.", *npid);
        } else if (contains(state.requests_sent)) {
            error = fmt::format("You already sent {} a friend request.", *npid);
        } else if (contains(state.blocked)) {
            error = fmt::format("Unblock {} before sending a friend request.", *npid);
        }
        if (!error.empty()) {
            m_dialog.Show(error, ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
            continue;
        }

        if (m_dialog.Show(fmt::format("Send a friend request to {}?", *npid),
                          ORBIS_MSG_DIALOG_BUTTON_TYPE_YESNO) != Choice::Button1) {
            continue;
        }

        if (np_handler.SendFriendRequest(m_user_id, *npid) != ORBIS_OK) {
            m_dialog.Show("Couldn't send your friend request to shadNet.",
                          ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
        } else if (contains(state.requests_received)) {
            // If the requested person sent you a request already, then accept their request.
            m_dialog.Show(fmt::format("Accepted {}'s friend request.", *npid),
                          ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
        }
    }
}

std::vector<std::string> FriendsPage::GetBlocked() {
    auto blocked = Libraries::Np::NpHandler::Instance().GetFriendList(m_user_id).blocked;
    std::sort(blocked.begin(), blocked.end());
    return blocked;
}

void FriendsPage::ShowBlocked() {
    while (true) {
        const auto blocked = GetBlocked();

        std::string msg = "Blocked users\nPress L1 to return to your friends list\n\n";
        if (blocked.empty()) {
            msg += "You haven't blocked anyone.";
        }
        for (const auto& npid : blocked) {
            msg += npid + "\n";
        }

        Choice choice = m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS, "Manage Blocks",
                                      "Back", Button::L1);
        if (choice != Choice::Button1) {
            return;
        }
        ManageBlocks();
    }
}

void FriendsPage::ManageBlocks() {
    const auto blocked = GetBlocked();
    if (blocked.empty()) {
        m_dialog.Show("You haven't blocked anyone.", ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
        return;
    }

    // The page after the last blocked user reviews the selected changes.
    std::vector<bool> unblock(blocked.size(), false);
    const u64 page_count = blocked.size() + 1;
    u64 index = 0;
    while (true) {
        if (index == blocked.size()) {
            Choice choice = ReviewUnblocks(blocked, unblock);
            if (choice == Choice::L1) {
                index = blocked.size() - 1;
            } else if (choice == Choice::R1) {
                index = 0;
            } else {
                return;
            }
            continue;
        }

        const std::string msg =
            fmt::format("{}\n\nSelected: {}\n\n({}/{})\nPress L1 or R1 to navigate", blocked[index],
                        unblock[index] ? "Unblock" : "None", index + 1, blocked.size());
        Choice choice = m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS,
                                      unblock[index] ? "Don't Unblock" : "Unblock", "Review",
                                      Button::L1 | Button::R1);
        if (choice == Choice::Button1) {
            unblock[index] = !unblock[index];
        } else if (choice == Choice::Button2) {
            index = blocked.size();
        } else if (choice == Choice::L1) {
            index = (index + page_count - 1) % page_count;
        } else if (choice == Choice::R1) {
            index = (index + 1) % page_count;
        } else {
            return;
        }
    }
}

MessageDialog::Choice FriendsPage::ReviewUnblocks(const std::vector<std::string>& blocked,
                                                  const std::vector<bool>& unblock) {
    std::vector<std::string> to_unblock;
    for (u64 i = 0; i < blocked.size(); i++) {
        if (unblock[i]) {
            to_unblock.push_back(blocked[i]);
        }
    }

    std::string msg = "Review changes\n\n";
    msg += to_unblock.empty() ? "No changes selected.\n"
                              : fmt::format("Unblock: {}\n", JoinNames(to_unblock));
    msg += "\nPress L1 or R1 to navigate";

    Choice choice = m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS, "Accept", "Cancel",
                                  Button::L1 | Button::R1);
    if (choice != Choice::Button1) {
        return choice;
    }

    auto& np_handler = Libraries::Np::NpHandler::Instance();
    bool sent = true;
    for (const auto& npid : to_unblock) {
        sent &= np_handler.UnblockUser(m_user_id, npid) == ORBIS_OK;
    }
    if (!sent) {
        m_dialog.Show("Some changes couldn't be sent to shadNet.", ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
    }
    return choice;
}

} // namespace ShadNet
