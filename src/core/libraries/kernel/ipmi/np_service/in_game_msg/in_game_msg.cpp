// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/ipmi/np_service/in_game_msg/in_game_msg.h"

namespace Libraries::Kernel::Ipmi::NpService {

InGameMsgService::InGameMsgService() : Service("SceNpInGameMsg", Id, 0x70000, 0x7ffff) {}

} // namespace Libraries::Kernel::Ipmi::NpService
