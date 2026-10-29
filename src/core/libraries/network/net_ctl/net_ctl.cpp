// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/shadnet_hook.h"
#include "core/libraries/network/net_ctl/net_ctl.h"

extern "C" {
void sceNetBweRegisterCallbackIpcInt();
void sceNetCtlApRegisterCallback();
void sceNetCtlApRpRegisterCallback();
void sceNetCtlRegisterCallback();
void sceNetCtlRegisterCallbackForLibIpcInt();
void sceNetCtlRegisterCallbackForNpToolkit();
void sceNetCtlRegisterCallbackIpcInt();
void sceNetCtlRegisterCallbackV6();
void sceNetCtlRegisterCallbackV6IpcInt();
void sceNetBweCheckCallbackIpcInt();
void sceNetCtlApCheckCallback();
void sceNetCtlApRpCheckCallback();
void sceNetCtlCheckCallback();
void sceNetCtlCheckCallbackForLibIpcInt();
void sceNetCtlCheckCallbackForNpToolkit();
void sceNetBweUnregisterCallbackIpcInt();
void sceNetCtlApUnregisterCallback();
void sceNetCtlApRpUnregisterCallback();
void sceNetCtlUnregisterCallback();
void sceNetCtlUnregisterCallbackForLibIpcInt();
void sceNetCtlUnregisterCallbackForNpToolkit();
void sceNetCtlUnregisterCallbackIpcInt();
void sceNetCtlUnregisterCallbackV6();
void sceNetCtlUnregisterCallbackV6IpcInt();
}

SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetBweRegisterCallbackIpcInt);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlApRegisterCallback);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlApRpRegisterCallback);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlRegisterCallback);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlRegisterCallbackForLibIpcInt);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlRegisterCallbackForNpToolkit);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlRegisterCallbackIpcInt);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlRegisterCallbackV6);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlRegisterCallbackV6IpcInt);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetBweCheckCallbackIpcInt);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlApCheckCallback);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlApRpCheckCallback);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlCheckCallback);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlCheckCallbackForLibIpcInt);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlCheckCallbackForNpToolkit);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetBweUnregisterCallbackIpcInt);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlApUnregisterCallback);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlApRpUnregisterCallback);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlUnregisterCallback);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlUnregisterCallbackForLibIpcInt);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlUnregisterCallbackForNpToolkit);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlUnregisterCallbackIpcInt);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlUnregisterCallbackV6);
SHADNET_HOOK_DECLARE(Libraries::Network::NetCtl, sceNetCtlUnregisterCallbackV6IpcInt);

static void RegisterNetCtlHooks() {
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetBweRegisterCallbackIpcInt);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlApRegisterCallback);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlApRpRegisterCallback);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlRegisterCallback);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlRegisterCallbackForLibIpcInt);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlRegisterCallbackForNpToolkit);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlRegisterCallbackIpcInt);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlRegisterCallbackV6);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlRegisterCallbackV6IpcInt);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetBweCheckCallbackIpcInt);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlApCheckCallback);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlApRpCheckCallback);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlCheckCallback);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlCheckCallbackForLibIpcInt);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlCheckCallbackForNpToolkit);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetBweUnregisterCallbackIpcInt);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlApUnregisterCallback);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlApRpUnregisterCallback);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlUnregisterCallback);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlUnregisterCallbackForLibIpcInt);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlUnregisterCallbackForNpToolkit);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlUnregisterCallbackIpcInt);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlUnregisterCallbackV6);
    SHADNET_HOOK(Libraries::Network::NetCtl, sceNetCtlUnregisterCallbackV6IpcInt);
}

namespace Libraries::Network::NetCtl {

void RegisterHooks() {
    return RegisterNetCtlHooks();
}
} // namespace Libraries::Network::NetCtl