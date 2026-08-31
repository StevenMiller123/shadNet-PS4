// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <magic_enum/magic_enum.hpp>
#include "common/logging/log.h"
#include "np_handler.h"
#include "shadnet/server_probe.h"

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

    // TODO: Make server host and port configurable
    static std::string hostname = "srv.shadps4.net";
    static u16 port = 31313;

    const ShadNet::ProbeInfo probe = ShadNet::ProbeServer(hostname, port);
    if (probe.result != ShadNet::ProbeResult::Ok) {
        LOG_NOTIFICATION("Failed to connect to shadNet server, error {}",
                         magic_enum::enum_name(probe.result));
        LOG_ERROR("Failed to connect to shadNet server, error {}",
                  magic_enum::enum_name(probe.result));
    } else {
        LOG_NOTIFICATION("shadNet server is accessible");
    }
}

} // namespace Libraries::Np
