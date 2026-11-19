// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <mutex>

#include "core/libraries/kernel/ipmi/np_service/np_service.h"

namespace Libraries::Kernel::Ipmi::NpService {

// SceNpPush2 service, handles methods 0xe0000-0xeffff
class Push2Service : public Service {
public:
    static constexpr u32 Id = 13;

    Push2Service();

    s32 InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                         std::span<IpmiBufferInfo> out) override;

private:
    s32 RegisterNotificationUser(std::span<const IpmiDataInfo> in);
    s32 UnregisterNotificationUser(std::span<const IpmiDataInfo> in);
    s32 ConnectContext(std::span<const IpmiDataInfo> in);
    s32 CreateServerContext(std::span<const IpmiDataInfo> in);
    s32 DeleteServerContext(std::span<const IpmiDataInfo> in);

    std::mutex m_mutex;

    struct UserContext {
        s32 user_id;
    };
    struct ProcessContext {
        std::map<s32, UserContext> user_contexts;
    };
    std::map<s32, ProcessContext> m_proc_contexts;
};

} // namespace Libraries::Kernel::Ipmi::NpService
