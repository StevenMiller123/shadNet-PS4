// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/AppContent.h>
#include <orbis/libkernel.h>

#include "common/elf_info.h"
#include "common/shadnet_hook.h"
#include "core/libraries/app_content/app_content.h"

extern "C" void sceAppContentGetGameTrialsFlag();

SHADNET_HOOK_DECLARE(Libraries::AppContent, sceAppContentAppParamGetInt);
SHADNET_HOOK_DECLARE(Libraries::AppContent, sceAppContentGetGameTrialsFlag);

static void RegisterAppContentHooks() {
    s32 sdk_ver = 0;
    sceKernelGetCompiledSdkVersion(&sdk_ver);
    if (sdk_ver >= Common::ElfInfo::FW_1050) {
        // This function was added in firmware 10.50, only hook it for games that could call it.
        SHADNET_HOOK16(Libraries::AppContent, sceAppContentGetGameTrialsFlag);
    }
    SHADNET_HOOK16(Libraries::AppContent, sceAppContentAppParamGetInt);
}

namespace Libraries::AppContent {

s32 sceAppContentAppParamGetInt(OrbisAppContentAppParamId param_id, s32* out_value) {
    s32 ret = SHADNET_HOOK_CONTINUE(sceAppContentAppParamGetInt, param_id, out_value);
    LOG_INFO(Lib_AppContent, "returned {:#x}", static_cast<u32>(ret));
    if (ret == 0 && param_id == 0) {
        LOG_INFO(Lib_AppContent, "sku_flag = {}", *out_value);
    }
    return ret;
}

s32 sceAppContentGetGameTrialsFlag(OrbisAppContentGameTrialsFlag* game_trials_flag) {
    s32 ret = SHADNET_HOOK_CONTINUE(sceAppContentGetGameTrialsFlag, game_trials_flag);
    LOG_INFO(Lib_AppContent, "returned {:#x}", static_cast<u32>(ret));
    if (ret == 0) {
        LOG_INFO(Lib_AppContent, "game_trials_flag = {}", *game_trials_flag);
    }
    return ret;
}

void RegisterHooks() {
    return RegisterAppContentHooks();
}

} // namespace Libraries::AppContent