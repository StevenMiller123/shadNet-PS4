// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::System::UserService {
s32 sceUserServiceGetUserName(s32 user_id, char* user_name, u64 name_len);
void RegisterHooks();
} // namespace Libraries::System::UserService