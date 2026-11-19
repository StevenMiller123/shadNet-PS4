// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <set>

#include "core/libraries/kernel/ipmi/np_service/np_service.h"

namespace Libraries::Kernel::Ipmi::NpService {

// SceNpPush service, handles methods 0x40000-0x4ffff
class PushService : public Service {
public:
    static constexpr u32 Id = 3;

    PushService();

    s32 InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                         std::span<IpmiBufferInfo> out) override;

private:
    s32 RegisterNotificationUser(std::span<const IpmiDataInfo> in);
    s32 UnregisterNotificationUser(std::span<const IpmiDataInfo> in);

    std::mutex m_mutex;
    std::set<s32> m_notification_users;
};

} // namespace Libraries::Kernel::Ipmi::NpService
