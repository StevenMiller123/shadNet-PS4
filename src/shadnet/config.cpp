// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>

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
    nlohmann::json j;
    j["Server"] = m_server;

    std::ofstream out{config_path};
    if (!out) {
        LOG_ERROR("Failed to open config for writing");
        return;
    }
    out << std::setw(2) << j;
    if (out.fail()) {
        LOG_ERROR("Failed to write to config");
    }
}

void Settings::Initialize() {
    if (!std::filesystem::exists(config_path)) {
        InitialSetup();
    } else {
        std::ifstream in{config_path};
        if (!in.good()) {
            LOG_ERROR("Failed to open read config file");
            return;
        }

        nlohmann::json gj;
        in >> gj;
        if (gj.contains("Server")) {
            nlohmann::json current = m_server;
            current.update(gj.at("Server"));
            m_server = current.get<std::remove_reference_t<ShadNet::ServerSettings>>();
        }
    }
}

} // namespace ShadNet