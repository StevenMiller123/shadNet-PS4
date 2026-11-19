// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <mutex>

#include "core/libraries/kernel/ipmi/np_service/np_service.h"

namespace Libraries::Kernel::Ipmi::NpService {

// SceNpPresence service, handles methods 0x50000-0x5ffff
class PresenceService : public Service {
public:
    static constexpr u32 Id = 4;

    PresenceService();

    s32 InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                         std::span<IpmiBufferInfo> out) override;

protected:
    s32 OnDestroyRequest(s32 request_id) override;

private:
    s32 StartPresenceRequest(std::span<const IpmiDataInfo> in);
    s32 GetPresenceRequestResult(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);

    // Results of finished requests, by request id.
    std::mutex m_mutex;
    std::map<s32, s32> m_results;
};

} // namespace Libraries::Kernel::Ipmi::NpService
