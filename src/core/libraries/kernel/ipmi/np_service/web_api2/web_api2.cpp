// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/ipmi/np_service/web_api2/web_api2.h"

namespace Libraries::Kernel::Ipmi::NpService {

WebApi2Service::WebApi2Service() : Service("SceNpWebApi2", Id, 0x100000, 0x10ffff) {}

} // namespace Libraries::Kernel::Ipmi::NpService
