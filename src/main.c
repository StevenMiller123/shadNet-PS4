// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <Common.h>
#include "common/plugin_common.h"

attr_public const char* g_pluginName = "shadNet-PS4";
attr_public const char* g_pluginDesc = "A plugin for replacing PSN calls with shadNet";
attr_public const char* g_pluginAuth = "Stephen";
attr_public u32 g_pluginVersion = 0x00000018; // 1.00
extern s32 client_start();

s32 attr_public attr_aligned plugin_load(s32 argc, const char* argv[]) {
    final_printf("[GoldHEN] <%s\\Ver.0x%08x> %s\n", g_pluginName, g_pluginVersion, __func__);
    final_printf("[GoldHEN] Plugin Author(s): %s\n", g_pluginAuth);
    boot_ver();
    return 0;
}

s32 attr_public attr_aligned plugin_unload(s32 argc, const char* argv[]) {
    final_printf("[GoldHEN] <%s\\Ver.0x%08x> %s\n", g_pluginName, g_pluginVersion, __func__);
    return 0;
}

s32 attr_module_hidden attr_aligned module_start(s64 argc, const void* args) {
    client_start();
    return 0;
}

s32 attr_module_hidden attr_aligned module_stop(s64 argc, const void* args) {
    return 0;
}
