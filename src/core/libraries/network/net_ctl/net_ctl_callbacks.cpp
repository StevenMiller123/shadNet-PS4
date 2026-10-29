// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/network/net_ctl/net_ctl.h"

namespace Libraries::Network::NetCtl {

s32 sceNetBweRegisterCallbackIpcInt(OrbisNetCtlCallback func, void* user_data, s32* cb_id) {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    *cb_id = 0;
    return 0;
}

s32 sceNetCtlApRegisterCallback(OrbisNetCtlCallback func, void* user_data, s32* cb_id) {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    *cb_id = 0;
    return 0;
}

s32 sceNetCtlApRpRegisterCallback(OrbisNetCtlCallback func, void* user_data, s32* cb_id) {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    *cb_id = 0;
    return 0;
}

s32 sceNetCtlRegisterCallback(OrbisNetCtlCallback func, void* user_data, s32* cb_id) {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    *cb_id = 0;
    return 0;
}

s32 sceNetCtlRegisterCallbackForLibIpcInt(OrbisNetCtlCallback func, void* user_data, s32* cb_id) {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    *cb_id = 0;
    return 0;
}

s32 sceNetCtlRegisterCallbackForNpToolkit(OrbisNetCtlCallback func, void* user_data, s32* cb_id) {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    *cb_id = 0;
    return 0;
}

s32 sceNetCtlRegisterCallbackIpcInt(OrbisNetCtlCallback func, void* user_data, s32* cb_id) {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    *cb_id = 0;
    return 0;
}

s32 sceNetCtlRegisterCallbackV6(OrbisNetCtlCallback func, void* user_data, s32* cb_id) {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    *cb_id = 0;
    return 0;
}

s32 sceNetCtlRegisterCallbackV6IpcInt(OrbisNetCtlCallback func, void* user_data, s32* cb_id) {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    *cb_id = 0;
    return 0;
}

s32 sceNetBweCheckCallbackIpcInt() {
    // LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return 0;
}

s32 sceNetCtlApCheckCallback() {
    // LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return 0;
}

s32 sceNetCtlApRpCheckCallback() {
    // LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return 0;
}

s32 sceNetCtlCheckCallback() {
    // LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return 0;
}

s32 sceNetCtlCheckCallbackForLibIpcInt() {
    // LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return 0;
}

s32 sceNetCtlCheckCallbackForNpToolkit() {
    // LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return 0;
}

s32 sceNetBweUnregisterCallbackIpcInt(s32 cb_id) {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return 0;
}

s32 sceNetCtlApUnregisterCallback(s32 cb_id) {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return 0;
}

s32 sceNetCtlApRpUnregisterCallback(s32 cb_id) {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return 0;
}

s32 sceNetCtlUnregisterCallback(s32 cb_id) {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return 0;
}

s32 sceNetCtlUnregisterCallbackForLibIpcInt(s32 cb_id) {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return 0;
}

s32 sceNetCtlUnregisterCallbackForNpToolkit(s32 cb_id) {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return 0;
}

s32 sceNetCtlUnregisterCallbackIpcInt(s32 cb_id) {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return 0;
}

s32 sceNetCtlUnregisterCallbackV6(s32 cb_id) {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return 0;
}

s32 sceNetCtlUnregisterCallbackV6IpcInt(s32 cb_id) {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return 0;
}

} // namespace Libraries::Network::NetCtl
