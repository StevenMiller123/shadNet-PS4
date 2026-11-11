// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <ctime>
#include <orbis/ImeDialog.h>
#include <orbis/MsgDialog.h>

#include "common/logging/log.h"
#include "core/libraries/game_custom_data_dialog/game_custom_data_dialog_ui.h"
#include "core/libraries/pad/pad.h"

namespace Libraries::GameCustomDataDialog {

using CommonDialog::OrbisCommonDialogResult;
using CommonDialog::OrbisCommonDialogStatus;
using Np::NpHandler;
using Choice = ShadNet::MessageDialog::Choice;
using Button = Libraries::Pad::Button;

static constexpr u64 MAX_DISPLAYED_TEXT_LENGTH = 256;

static std::string DescribeExpiry(s32 minutes) {
    if (minutes <= 0) {
        return "never";
    }
    if (minutes % (24 * 60) == 0) {
        const s32 days = minutes / (24 * 60);
        return fmt::format("in {} day{}", days, days == 1 ? "" : "s");
    }
    if (minutes % 60 == 0) {
        const s32 hours = minutes / 60;
        return fmt::format("in {} hour{}", hours, hours == 1 ? "" : "s");
    }
    return fmt::format("in {} minute{}", minutes, minutes == 1 ? "" : "s");
}

static std::string JoinNames(const std::vector<std::string>& items) {
    std::string out;
    for (const auto& item : items) {
        out += (out.empty() ? "" : ", ") + item;
    }
    return out;
}

static std::string Shorten(const std::string& text) {
    return text.size() > MAX_DISPLAYED_TEXT_LENGTH
               ? text.substr(0, MAX_DISPLAYED_TEXT_LENGTH) + "..."
               : text;
}

static std::string NowRfc3339() {
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
    gmtime_r(&now, &utc);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &utc);
    return buf;
}

// "2026-10-31T09:38:00Z" -> "2026-10-31 09:38 UTC".
static std::string ShortDate(const std::string& rfc3339) {
    if (rfc3339.size() < 16) {
        return rfc3339;
    }
    return rfc3339.substr(0, 10) + " " + rfc3339.substr(11, 5) + " UTC";
}

GameCustomDataDialogUi::~GameCustomDataDialogUi() {
    m_dialog.Abort();
    Join();
}

void GameCustomDataDialogUi::Start(std::atomic<OrbisCommonDialogStatus>* status,
                                   DialogState* state) {
    Join();
    m_status = status;
    m_state = state;
    m_finished = false;
    m_thread = std::thread(&GameCustomDataDialogUi::DialogThread, this);
}

void GameCustomDataDialogUi::Finish(OrbisCommonDialogResult result) {
    std::scoped_lock lk{m_finish_mutex};
    if (m_finished || m_state == nullptr || m_status == nullptr) {
        return;
    }
    m_finished = true;
    m_state->result = result;
    LOG_INFO(Lib_GameCustomDataDialog, "finished: {}",
             result == OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_OK ? "OK"
                                                                              : "USER_CANCELED");
    *m_status = OrbisCommonDialogStatus::ORBIS_COMMON_DIALOG_STATUS_FINISHED;
}

void GameCustomDataDialogUi::Close() {
    Finish(OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_OK);
    m_dialog.Abort();
}

void GameCustomDataDialogUi::Join() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void GameCustomDataDialogUi::DialogThread() {
    if (!m_dialog.Begin(m_state->user_id)) {
        Finish(OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED);
        return;
    }

    const OrbisCommonDialogResult result =
        m_state->mode == ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_RECV ? RunRecv() : RunSend();

    m_dialog.End();
    Finish(result);
}

std::string GameCustomDataDialogUi::FormatSendSummary(const std::vector<std::string>& recipients) {
    std::string msg = fmt::format("Send Game Data\n\n{}\n{}\n", Shorten(m_state->data_name),
                                  Shorten(m_state->data_detail));
    if (m_state->mode == ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_SEND_URL) {
        msg += fmt::format("Link: {}{}\n", Shorten(m_state->data_url),
                           m_state->with_item_id ? " (+ item ID)" : "");
    } else {
        msg += fmt::format("Attached game data: {} bytes\n", m_state->game_data.size());
    }
    msg += fmt::format("Expires: {}\n", DescribeExpiry(m_state->expire_minutes));
    if (!m_state->platforms.empty()) {
        msg += fmt::format("Platforms: {}\n", JoinNames(m_state->platforms));
    }
    msg += fmt::format("\nMessage: {}\n\nTo: {}\n\nPress Triangle to edit the message",
                       Shorten(m_state->message),
                       recipients.empty() ? "(nobody)" : JoinNames(recipients));
    return msg;
}

void GameCustomDataDialogUi::EditMessage() {
    const auto message =
        m_dialog.ShowKeyboard("Message", m_state->message, ORBIS_TYPE_DEFAULT, false,
                              ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_USER_MSG_SIZE);
    if (message) {
        m_state->message = *message;
    }
}

bool GameCustomDataDialogUi::Send(const std::vector<std::string>& recipients) {
    NpHandler::GameCustomDataToSend item;
    item.to = recipients;
    item.message = m_state->message;
    item.data_name = m_state->data_name;
    item.data_description = m_state->data_detail;
    item.localized_names = m_state->localized_names;
    item.localized_descriptions = m_state->localized_details;
    item.expire_minutes = std::max(m_state->expire_minutes, 0);
    item.platforms = m_state->platforms;
    item.is_url = m_state->mode == ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_SEND_URL;
    item.url = m_state->data_url;
    item.with_item_id = m_state->with_item_id;
    item.game_data = m_state->game_data;
    item.thumbnail = m_state->thumbnail;

    LOG_INFO(Lib_GameCustomDataDialog, "sending '{}' to {}", item.data_name, JoinNames(item.to));
    const auto result = NpHandler::Instance().SendGameCustomData(m_state->user_id, item);
    if (!result.ok) {
        // Stay open so the player can retry or cancel.
        m_dialog.Show(fmt::format("Couldn't send: {}", result.message),
                      ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
        return false;
    }
    return true;
}

OrbisCommonDialogResult GameCustomDataDialogUi::RunSend() {
    const auto canceled = OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED;

    if (!m_state->user_editable) {
        std::vector<std::string> names = m_state->fixed_online_ids;
        for (const auto account_id : m_state->fixed_account_ids) {
            names.push_back(fmt::format("account {}", account_id));
        }
        std::vector<std::string> recipients = m_state->fixed_online_ids;
        for (const auto account_id : m_state->fixed_account_ids) {
            recipients.push_back(std::to_string(account_id));
        }
        while (true) {
            const Choice choice =
                m_dialog.Show(FormatSendSummary(names), ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS,
                              "Send", "Cancel", Button::Triangle);
            if (choice == Choice::Triangle) {
                EditMessage();
            } else if (choice != Choice::Button1) {
                return canceled;
            } else if (Send(recipients)) {
                m_state->sent_online_ids = m_state->fixed_online_ids;
                m_state->sent_account_ids = m_state->fixed_account_ids;
                return OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_OK;
            }
        }
    }

    const auto friends = NpHandler::Instance().GetFriendList(m_state->user_id).friends;
    if (friends.empty()) {
        m_dialog.Show(
            "Send Game Data\n\nNo friends available.\nAdd friends to send them game data.",
            ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
        return canceled;
    }

    std::vector<bool> selected(friends.size());
    const u64 page_count = friends.size() + 1;
    u64 index = page_count - 1;
    while (true) {
        std::vector<std::string> names;
        for (u64 i = 0; i < friends.size(); i++) {
            if (selected[i]) {
                names.push_back(friends[i].npid);
            }
        }

        Choice choice;
        if (index < friends.size()) {
            const auto& info = friends[index];
            const std::string msg =
                fmt::format("Send Game Data\n\n{} ({})\n\n({}/{})\nSelected: {}/{}\n"
                            "Press L1 or R1 to navigate",
                            info.npid, info.online ? "online" : "offline", index + 1,
                            friends.size(), names.size(), m_state->user_max_count);
            choice = m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS, "Next",
                                   selected[index] ? "Remove" : "Add", Button::L1 | Button::R1);
            if (choice == Choice::Button1) {
                index++;
                continue;
            }
            if (choice == Choice::Button2) {
                if (!selected[index] && names.size() >= m_state->user_max_count) {
                    m_dialog.Show(
                        fmt::format("You can choose up to {} friend(s).", m_state->user_max_count),
                        ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
                } else {
                    selected[index] = !selected[index];
                }
                continue;
            }
        } else {
            const std::string msg = FormatSendSummary(names) + "\nPress L1 or R1 to choose friends";
            choice = m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS, "Send", "Cancel",
                                   Button::L1 | Button::R1 | Button::Triangle);
            if (choice == Choice::Triangle) {
                EditMessage();
                continue;
            }
            if (choice == Choice::Button1) {
                if (names.empty()) {
                    m_dialog.Show("Choose at least one friend to send the game data to.",
                                  ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
                    index = 0;
                } else if (Send(names)) {
                    m_state->sent_online_ids = std::move(names);
                    return OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_OK;
                }
                continue;
            }
        }

        if (choice == Choice::L1) {
            index = (index + page_count - 1) % page_count;
        } else if (choice == Choice::R1) {
            index = (index + 1) % page_count;
        } else {
            return canceled;
        }
    }
}

OrbisCommonDialogResult GameCustomDataDialogUi::RunRecv() {
    const auto canceled = OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED;
    auto& np_handler = NpHandler::Instance();

    std::vector<NpHandler::ReceivedGameCustomData> items;
    const auto result = np_handler.ListReceivedGameCustomData(m_state->user_id, {}, items);
    if (!result.ok) {
        m_dialog.Show(
            fmt::format("Received Game Data\n\nCouldn't get game data: {}", result.message),
            ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
        return canceled;
    }
    if (items.empty()) {
        m_dialog.Show("Received Game Data\n\nYou haven't received any game data.",
                      ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
        return canceled;
    }

    const std::string now = NowRfc3339();
    u64 index = 0;
    while (true) {
        const auto& item = items[index];
        const bool expired = !item.expired_date.empty() && item.expired_date <= now;

        std::string msg =
            fmt::format("Received Game Data\n\n{}\nfrom {}, {}\n", Shorten(item.data_name),
                        item.from_online_id, ShortDate(item.received_date));
        if (!item.message.empty()) {
            msg += fmt::format("\n\"{}\"\n", Shorten(item.message));
        }
        if (!item.data_description.empty()) {
            msg += Shorten(item.data_description) + "\n";
        }
        if (item.is_url) {
            msg += fmt::format("\nLink: {}\n",
                               item.url.empty() ? "(no longer available)" : Shorten(item.url));
        } else if (item.used) {
            msg += "\nAlready used\n";
        } else if (expired) {
            msg += "\nExpired\n";
        }
        msg += fmt::format("\n({}/{})\nPress L1 or R1 to navigate", index + 1, items.size());

        const Choice choice = m_dialog.Show(msg, ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS, "Use",
                                            "Close", Button::L1 | Button::R1);
        if (choice == Choice::L1) {
            index = (index + items.size() - 1) % items.size();
        } else if (choice == Choice::R1) {
            index = (index + 1) % items.size();
        } else if (choice != Choice::Button1) {
            return canceled;
        } else if (item.is_url) {
            m_dialog.Show("Links can't be opened from here.", ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
        } else if (item.used || expired) {
            m_dialog.Show(item.used ? "This game data was already used."
                                    : "This game data has expired.",
                          ORBIS_MSG_DIALOG_BUTTON_TYPE_OK);
        } else {
            LOG_INFO(Lib_GameCustomDataDialog, "player chose item {} '{}' from {}", item.item_id,
                     item.data_name, item.from_online_id);
            np_handler.PostGameCustomDataEvent(m_state->user_id, item.item_id);
            return OrbisCommonDialogResult::ORBIS_COMMON_DIALOG_RESULT_OK;
        }
    }
}

} // namespace Libraries::GameCustomDataDialog
