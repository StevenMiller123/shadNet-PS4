// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/Sysmodule.h>
#include <orbis/libkernel.h>
#include "client_main.h"
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/types.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/network/net.h"
#include "core/libraries/np/np_handler.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_score.h"
#include "core/libraries/system/user_service.h"
#include "shadnet/config.h"

extern "C" s32 client_start() {
    LOG_INFO(shadNet, "Starting shadNet Client");

    // Preload modules used by the plugin
    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NET);
    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NETCTL);
    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SYS_UTIL);
    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_USER_SERVICE);
    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NP_MANAGER);

    sceSysmoduleLoadModule(ORBIS_SYSMODULE_NP_SCORE_RANKING);

    // Initialize config backend
    ShadNet::Settings::GetInstance().Initialize();

    // Init kernel hooks
    // This must be first because it hooks mmap to reduce the plugin's flexible memory usage.
    Libraries::Kernel::Kernel::RegisterHooks();

    // Init other library hooks
    Libraries::Network::Net::RegisterHooks();
    Libraries::System::UserService::RegisterHooks();
    Libraries::Np::NpManager::RegisterHooks();
    Libraries::Np::NpScore::RegisterHooks();

    // Initialize elfinfo
    auto& game_info = Common::ElfInfo::Instance();

    // Retrieve as much game metadata as we can
    OrbisAppInfo app_info{};
    s32 result = sceKernelGetAppInfo(getpid(), &app_info);
    if (result != 0) {
        LOG_ERROR(shadNet, "sceKernelGetAppInfo failed!");
        return 1;
    }

    LOG_INFO(shadNet, "Currently running {}", app_info.TitleId);
    sceKernelGetCompiledSdkVersion(reinterpret_cast<s32*>(&game_info.sdk_ver));
    game_info.initialized = true;
    game_info.game_serial = std::string{app_info.TitleId};

    // Initialize NpHandler
    Libraries::Np::NpHandler::Instance().Initialize();
    return 0;
}