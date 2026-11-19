// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/kernel/ipmi/np_service/np_service.h"

namespace Libraries::Kernel::Ipmi::NpService {

// SceNpWebApi2 service, handles methods 0x100000-0x10ffff
class WebApi2Service : public Service {
public:
    static constexpr u32 Id = 14;

    WebApi2Service();
};

} // namespace Libraries::Kernel::Ipmi::NpService
