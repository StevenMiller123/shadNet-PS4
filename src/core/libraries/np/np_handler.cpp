// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <magic_enum/magic_enum.hpp>
#include "common/logging/log.h"
#include "np_handler.h"
#include "shadnet/server_probe.h"
#include "shadnet/config.h"

namespace Libraries::Np {

NpHandler& NpHandler::GetInstance() {
    static NpHandler s_instance;
    return s_instance;
}

void NpHandler::Initialize() {
    if (m_initialized.exchange(true)) {
        LOG_WARNING("Initialize called more than once");
        return;
    }

    auto& config = ShadNet::Settings::GetInstance();

    static std::string server_url = config.GetServerUrl();
    static const u64 colon = server_url.rfind(':');
    if (colon == std::string::npos) {
        LOG_WARNING("Invalid server url {}", server_url);
        m_initialized.exchange(false);
        return;
    }
    static std::string hostname = server_url.substr(0, colon);
    u16 port{};
    try {
        port = static_cast<u16>(std::stoi(server_url.substr(colon + 1)));
    } catch (const std::exception&) {
        LOG_WARNING("Invalid server url {}", server_url);
        m_initialized.exchange(false);
        return;
    }

    const ShadNet::ProbeInfo probe = ShadNet::ProbeServer(hostname, port);
    if (probe.result != ShadNet::ProbeResult::Ok) {
        LOG_NOTIFICATION("Failed to connect to shadNet server, error {}",
                         magic_enum::enum_name(probe.result));
    } else {
        LOG_NOTIFICATION("shadNet server is accessible");
    }
}

} // namespace Libraries::Np
