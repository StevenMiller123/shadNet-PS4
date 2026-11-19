// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/ipmi/np_service/match2_sig/match2_sig.h"
#include "core/libraries/np/np_error.h"

namespace Libraries::Kernel::Ipmi::NpService {

namespace {

enum class Method : u32 {
    ReportConnectionInfo = 0xb0000,
    ReportConnectionStats = 0xb0001,
};

constexpr u64 ConnectionInfoSize = 0x38;
constexpr u64 ConnectionStatsSize = 0x144;

} // namespace

Match2SigService::Match2SigService() : Service("SceNpMatch2Sig", Id, 0xb0000, 0xbffff) {}

s32 Match2SigService::InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                                       std::span<IpmiBufferInfo> out) {
    u64 size;
    switch (static_cast<Method>(method)) {
    case Method::ReportConnectionInfo:
        // Only used for telemetry, which we can ignore.
        size = ConnectionInfoSize;
        break;
    case Method::ReportConnectionStats:
        // Only used for telemetry, which we can ignore.
        size = ConnectionStatsSize;
        break;
    default:
        return Service::InvokeSyncMethod(method, in, out);
    }
    if (in.empty() || in[0].data == nullptr || in[0].size < size) {
        return ORBIS_NP_SIGNALING_INTERNAL_ERROR_INVALID_ARGUMENT;
    }
    return 0;
}

} // namespace Libraries::Kernel::Ipmi::NpService
