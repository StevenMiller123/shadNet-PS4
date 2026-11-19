// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>
#include <orbis/libkernel.h>
#include <unistd.h>

#include "common/logging/log.h"
#include "core/libraries/kernel/ipmi/np_service/score/score.h"

namespace Libraries::Kernel::Ipmi::NpService {

namespace {

enum class Method : u32 {
    Initialize = 0x30000,
};

} // namespace

ScoreService::ScoreService() : Service("SceNpScore", Id, 0x30000, 0x3ffff) {
    // SceShellCore creates this event flag once per process when the NpService server starts.
    [[maybe_unused]] static const bool created = [] {
        char name[32];
        snprintf(name, sizeof(name), "SceNpScoreIpc_%08x", getpid());
        OrbisKernelEventFlag evf{};
        const s32 ret = sceKernelCreateEventFlag(&evf, name, 0x121, 0, nullptr);
        if (ret < 0) {
            LOG_WARNING(Lib_NpScore, "Failed to create event flag {}, ret = {:#x}", name,
                        static_cast<u32>(ret));
        }
        return ret >= 0;
    }();
}

s32 ScoreService::InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                                   std::span<IpmiBufferInfo> out) {
    switch (static_cast<Method>(method)) {
    case Method::Initialize:
        return 0;
    default:
        return Service::InvokeSyncMethod(method, in, out);
    }
}

} // namespace Libraries::Kernel::Ipmi::NpService
