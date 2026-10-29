// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::Network::NetCtl {

using OrbisNetCtlCallback = void (*)(s32 event_type, void* user_data);

s32 sceNetBweRegisterCallbackIpcInt(OrbisNetCtlCallback func, void* user_data, s32* cb_id);
s32 sceNetCtlApRegisterCallback(OrbisNetCtlCallback func, void* user_data, s32* cb_id);
s32 sceNetCtlApRpRegisterCallback(OrbisNetCtlCallback func, void* user_data, s32* cb_id);
s32 sceNetCtlRegisterCallback(OrbisNetCtlCallback func, void* user_data, s32* cb_id);
s32 sceNetCtlRegisterCallbackForLibIpcInt(OrbisNetCtlCallback func, void* user_data, s32* cb_id);
s32 sceNetCtlRegisterCallbackForNpToolkit(OrbisNetCtlCallback func, void* user_data, s32* cb_id);
s32 sceNetCtlRegisterCallbackIpcInt(OrbisNetCtlCallback func, void* user_data, s32* cb_id);
s32 sceNetCtlRegisterCallbackV6(OrbisNetCtlCallback func, void* user_data, s32* cb_id);
s32 sceNetCtlRegisterCallbackV6IpcInt(OrbisNetCtlCallback func, void* user_data, s32* cb_id);

s32 sceNetBweCheckCallbackIpcInt();
s32 sceNetCtlApCheckCallback();
s32 sceNetCtlApRpCheckCallback();
s32 sceNetCtlCheckCallback();
s32 sceNetCtlCheckCallbackForLibIpcInt();
s32 sceNetCtlCheckCallbackForNpToolkit();

s32 sceNetBweUnregisterCallbackIpcInt(s32 cb_id);
s32 sceNetCtlApUnregisterCallback(s32 cb_id);
s32 sceNetCtlApRpUnregisterCallback(s32 cb_id);
s32 sceNetCtlUnregisterCallback(s32 cb_id);
s32 sceNetCtlUnregisterCallbackForLibIpcInt(s32 cb_id);
s32 sceNetCtlUnregisterCallbackForNpToolkit(s32 cb_id);
s32 sceNetCtlUnregisterCallbackIpcInt(s32 cb_id);
s32 sceNetCtlUnregisterCallbackV6(s32 cb_id);
s32 sceNetCtlUnregisterCallbackV6IpcInt(s32 cb_id);

void RegisterHooks();
} // namespace Libraries::Network::NetCtl