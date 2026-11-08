// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::Kernel {

s32 __sys_socketex(const char* name, s32 family, s32 type, s32 protocol);
s32 socket(s32 family, s32 type, s32 protocol);
s32 setsockopt(s32 s, s32 level, s32 optname, const void* optval, u32 optlen);
s32 _recvfrom(s32 s, void* buf, u64 len, s32 flags, void* from, u32* fromlen);
s32 _recvmsg(s32 s, void* msg, s32 flags);
s32 _sendmsg(s32 s, const void* msg, s32 flags);
s32 _sendto(s32 s, const void* msg, u64 len, s32 flags, const void* to, u32 tolen);

void RegisterHooks();
} // namespace Libraries::Kernel