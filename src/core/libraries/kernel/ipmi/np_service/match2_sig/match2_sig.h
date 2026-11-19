// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/kernel/ipmi/np_service/np_service.h"

namespace Libraries::Kernel::Ipmi::NpService {

// SceNpMatch2Sig service, handles methods 0xb0000-0xbffff
class Match2SigService : public Service {
public:
    static constexpr u32 Id = 10;

    Match2SigService();

    s32 InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                         std::span<IpmiBufferInfo> out) override;
};

} // namespace Libraries::Kernel::Ipmi::NpService
