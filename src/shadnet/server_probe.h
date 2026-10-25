// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include "common/types.h"

namespace ShadNet {

static constexpr u32 SHAD_PROBE_TIMEOUT_MS = 3000;

enum class ProbeResult {
    Ok,
    Unreachable,
    VersionMismatch,
    ProtocolError,
};

struct ProbeInfo {
    ProbeResult result = ProbeResult::Unreachable;
    u32 server_version = 0;
};

ProbeInfo ProbeServer(const std::string& host, u16 port, u32 timeout_ms = SHAD_PROBE_TIMEOUT_MS);

} // namespace ShadNet
