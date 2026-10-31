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

static bool WriteConfig(const nlohmann::json& j) {
    if (!std::filesystem::exists("/data/shadnet")) {
        // Need to make shadnet config folder.
        std::filesystem::create_directory("/data/shadnet");
    }

    std::ofstream out{config_path};
    if (!out) {
        LOG_ERROR(Config, "Failed to open config for writing");
        return false;
    }
    out << std::setw(2) << j;
    if (out.fail()) {
        LOG_ERROR(Config, "Failed to write to config");
        return false;
    }
    return true;
}

void Settings::InitialSetup() {
    OrbisUserServiceLoginUserIdList user_list{};
    s32 result = sceUserServiceGetLoginUserIdList(&user_list);
    if (result != 0) {
        // Failed to get logged in users.
        LOG_ERROR(Config, "Failed to retrieve logged in users: {:#x}", (u32)result);
        return;
    }

    // Write initial settings
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
    WriteConfig(j);
}

bool Settings::Save(s32 user_id, const UserSettings& user, const ServerSettings& server) {
    // Start from the existing file, so other users' settings are kept.
    nlohmann::json j = nlohmann::json::object();
    if (std::filesystem::exists(config_path)) {
        std::ifstream in{config_path};
        nlohmann::json existing = nlohmann::json::parse(in, nullptr, false);
        if (existing.is_object()) {
            j = existing;
        } else {
            LOG_WARNING(Config, "existing config is invalid, overwriting it");
        }
    }

    j[std::to_string(user_id)] = user;
    j["Server"] = server;
    return WriteConfig(j);
}

void Settings::Initialize() {
    if (!std::filesystem::exists(config_path)) {
        // Config doesn't exist, create it.
        InitialSetup();
        m_first_run = true;
    } else {
        std::ifstream in{config_path};
        if (!in.good()) {
            LOG_ERROR(Config, "Failed to open read config file");
            return;
        }

        // Update the log appropriately
        nlohmann::json gj;
        in >> gj;
        OrbisUserServiceLoginUserIdList user_list{};
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

        // Tell the user how to access the settings menu
        LOG_NOTIFICATION(shadNet, "Hold L3 + R3 to open plugin settings");
    }
}

} // namespace ShadNet