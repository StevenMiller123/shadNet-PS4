// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/ipmi/np_service/sns/sns.h"

namespace Libraries::Kernel::Ipmi::NpService {

SnsService::SnsService() : Service("SceNpSns", Id, 0x80000, 0x8ffff) {}

} // namespace Libraries::Kernel::Ipmi::NpService
