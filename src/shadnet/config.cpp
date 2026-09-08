// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <orbis/UserService.h>
#include "common/logging/log.h"
#include "shadnet/config.h"

namespace ShadNet {

Settings& Settings::GetInstance() {
    static Settings s_instance;
    return s_instance;
}

static const char* config_path = "/data/shadnet/config.json";

void Settings::InitialSetup() {
    // TODO: user-friendly setup via PS4 dialog libraries.

    // Create and save config.
    if (!std::filesystem::exists("/data/shadnet")) {
        // Need to make shadnet config folder.
        std::filesystem::create_directory("/data/shadnet");
    }

    OrbisUserServiceLoginUserIdList user_list{};
    s32 result = sceUserServiceGetLoginUserIdList(&user_list);
    if (result != 0) {
        // Failed to get logged in users.
        LOG_ERROR(Config, "Failed to retrieve logged in users: {:#x}", (u32)result);
        return;
    }

    nlohmann::json j;
    for (s32 i = 0; i < ORBIS_USER_SERVICE_MAX_LOGIN_USERS; i++) {
        s32 user_id = user_list.userId[i];
        if (user_id == ORBIS_USER_SERVICE_USER_ID_INVALID) {
            break;
        }
        std::string id_str = std::to_string(user_id);
        j[id_str] = m_user[user_id];
    }
    j["Server"] = m_server;

    std::ofstream out{config_path};
    if (!out) {
        LOG_ERROR(Config, "Failed to open config for writing");
        return;
    }
    out << std::setw(2) << j;
    if (out.fail()) {
        LOG_ERROR(Config, "Failed to write to config");
    }
}

void Settings::Initialize() {
    if (!std::filesystem::exists(config_path)) {
        InitialSetup();
    } else {
        std::ifstream in{config_path};
        if (!in.good()) {
            LOG_ERROR(Config, "Failed to open read config file");
            return;
        }

        nlohmann::json gj;
        in >> gj;
        OrbisUserServiceLoginUserIdList user_list{};
        sceUserServiceInitialize(nullptr);
        s32 result = sceUserServiceGetLoginUserIdList(&user_list);
        if (result != 0) {
            // Failed to get logged in users.
            LOG_ERROR(Config, "Failed to retrieve logged in users: {:#x}", (u32)result);
            return;
        }

        for (s32 i = 0; i < ORBIS_USER_SERVICE_MAX_LOGIN_USERS; i++) {
            s32 user_id = user_list.userId[i];
            if (user_id == ORBIS_USER_SERVICE_USER_ID_INVALID) {
                break;
            }
            std::string id_str = std::to_string(user_id);
            if (gj.contains(id_str)) {
                nlohmann::json current = m_user[user_id];
                current.update(gj.at(id_str));
                m_user[user_id] = current.get<std::remove_reference_t<ShadNet::UserSettings>>();
            }
        }
        if (gj.contains("Server")) {
            nlohmann::json current = m_server;
            current.update(gj.at("Server"));
            m_server = current.get<std::remove_reference_t<ShadNet::ServerSettings>>();
        }
    }
}

} // namespace ShadNet