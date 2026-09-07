// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>
#include <common/types.h>
#include <orbis/NpWebApi.h>

namespace Libraries::Np::NpWebApi {
struct PushEventInput {
    s32 target_user_id = 0;
    std::string np_service_name;
    u32 np_service_label = 0;
    std::string data_type;
    std::string data;
    OrbisNpOnlineId from_online_id{};
    bool has_from = false;
    OrbisNpOnlineId to_online_id{};
    bool has_to = false;
    std::vector<std::pair<std::string, std::string>> extd_data;
};
void EnqueuePushEvent(const PushEventInput& ev);
void RegisterHooks();
}; // namespace Libraries::Np::NpWebApi
