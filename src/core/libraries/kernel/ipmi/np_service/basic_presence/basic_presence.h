// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/kernel/ipmi/np_service/np_service.h"

namespace Libraries::Kernel::Ipmi::NpService {

// SceNpBasicPresence service, handles methods 0x60000-0x6ffff
class BasicPresenceService : public Service {
public:
    static constexpr u32 Id = 5;

    BasicPresenceService();

    s32 InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                         std::span<IpmiBufferInfo> out) override;

private:
    s32 GetGamePresenceStatus(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 SetNpTitleId(std::span<const IpmiDataInfo> in);
};

} // namespace Libraries::Kernel::Ipmi::NpService
