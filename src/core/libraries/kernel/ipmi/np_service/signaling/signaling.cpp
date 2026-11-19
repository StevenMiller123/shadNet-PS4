// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/ipmi/np_service/signaling/signaling.h"

namespace Libraries::Kernel::Ipmi::NpService {

SignalingService::SignalingService() : Service("SceNpSignaling", Id, 0x90000, 0x9ffff) {}

} // namespace Libraries::Kernel::Ipmi::NpService
