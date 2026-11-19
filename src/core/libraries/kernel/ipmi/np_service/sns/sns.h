// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/kernel/ipmi/np_service/np_service.h"

namespace Libraries::Kernel::Ipmi::NpService {

// SceNpSns service, handles methods 0x80000-0x8ffff
class SnsService : public Service {
public:
    static constexpr u32 Id = 7;

    SnsService();
};

} // namespace Libraries::Kernel::Ipmi::NpService
