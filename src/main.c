// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <Common.h>
#include "common/plugin_common.h"

attr_public const char* g_pluginName = "shadNet-PS4";
attr_public const char* g_pluginDesc = "A plugin for replacing PSN calls with shadNet";
attr_public const char* g_pluginAuth = "Stephen";
attr_public uint g_pluginVersion = 0x00000018; // 1.00
extern int client_start();

int attr_public attr_aligned plugin_load(int argc, const char* argv[]) {
    final_printf("[GoldHEN] <%s\\Ver.0x%08x> %s\n", g_pluginName, g_pluginVersion, __func__);
    final_printf("[GoldHEN] Plugin Author(s): %s\n", g_pluginAuth);
    boot_ver();
    return 0;
}

int attr_public attr_aligned plugin_unload(int argc, const char* argv[]) {
    final_printf("[GoldHEN] <%s\\Ver.0x%08x> %s\n", g_pluginName, g_pluginVersion, __func__);
    return 0;
}

int attr_module_hidden attr_aligned module_start(long argc, const void* args) {
    client_start();
    return 0;
}

int attr_module_hidden attr_aligned module_stop(long argc, const void* args) {
    return 0;
}
