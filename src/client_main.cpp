// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "client_main.h"
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/types.h"
#include "core/libraries/np/np_handler.h"

#include "orbis/SystemService.h"

extern "C" s32 client_start() {
    LOG_INFO("Starting shadNet Client");

    // Initialize elfinfo
    auto& game_info = Common::ElfInfo::Instance();

    sceKernelGetCompiledSdkVersion(reinterpret_cast<s32*>(&game_info.sdk_ver));
    game_info.initialized = true;

    // Initialize NpHandler
    Libraries::Np::NpHandler::GetInstance().Initialize();
    return 0;
}