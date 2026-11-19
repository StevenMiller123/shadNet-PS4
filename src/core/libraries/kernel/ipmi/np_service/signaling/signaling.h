// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/kernel/ipmi/np_service/np_service.h"

namespace Libraries::Kernel::Ipmi::NpService {

// SceNpSignaling service, handles methods 0x90000-0x9ffff
class SignalingService : public Service {
public:
    static constexpr u32 Id = 8;

    SignalingService();
};

} // namespace Libraries::Kernel::Ipmi::NpService
