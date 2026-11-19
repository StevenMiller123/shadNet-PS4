// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/kernel/ipmi/np_service/np_service.h"

namespace Libraries::Kernel::Ipmi::NpService {

// SceNpTus service, handles methods 0x20000-0x2ffff
class TusService : public Service {
public:
    static constexpr u32 Id = 1;

    TusService();

    s32 InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                         std::span<IpmiBufferInfo> out) override;
};

} // namespace Libraries::Kernel::Ipmi::NpService
