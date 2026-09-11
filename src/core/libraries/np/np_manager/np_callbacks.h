// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <orbis/NpManager.h>
#include "common/types.h"

namespace Libraries::Np::NpManager {

using OrbisNpStateCallback = void (*)(s32 userId, OrbisNpState state, OrbisNpId* npId,
                                      void* userdata);
using OrbisNpStateCallbackA = void (*)(s32 userId, OrbisNpState state, void* userdata);
using OrbisNpStateCallbackForToolkit = void (*)(s32 userId, OrbisNpState state, void* userdata);

enum class OrbisNpReachabilityState {
    Unavailable = 0,
    Available = 1,
    Reachable = 2,
};
using OrbisNpReachabilityStateCallback = void (*)(s32 userId, OrbisNpReachabilityState state,
                                                  void* userdata);

s32 sceNpRegisterNpReachabilityStateCallback(OrbisNpReachabilityStateCallback callback,
                                             void* userdata);
s32 sceNpUnregisterNpReachabilityStateCallback();

s32 sceNpRegisterStateCallback(OrbisNpStateCallback callback, void* userdata);
s32 sceNpUnregisterStateCallback();

s32 sceNpRegisterStateCallbackA(OrbisNpStateCallbackA callback, void* userdata);
s32 sceNpUnregisterStateCallbackA(s32 callback_id);

s32 sceNpRegisterStateCallbackForToolkit(OrbisNpStateCallbackForToolkit callback, void* userdata);
s32 sceNpUnregisterStateCallbackForToolkit();

s32 sceNpCheckCallback();
s32 sceNpCheckCallbackForLib();
} // namespace Libraries::Np::NpManager