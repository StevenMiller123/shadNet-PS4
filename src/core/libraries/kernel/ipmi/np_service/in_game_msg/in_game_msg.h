// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/kernel/ipmi/np_service/np_service.h"

namespace Libraries::Kernel::Ipmi::NpService {

// SceNpInGameMsg service, handles methods 0x70000-0x7ffff
class InGameMsgService : public Service {
public:
    static constexpr u32 Id = 6;

    InGameMsgService();
};

} // namespace Libraries::Kernel::Ipmi::NpService
