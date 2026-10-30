// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/Sysmodule.h>
#include <orbis/UserService.h>
#include <orbis/libkernel.h>

#include "client_main.h"
#include "common/elf_info.h"
#include "common/libjbc/jailbreak.h"
#include "common/logging/log.h"
#include "common/types.h"
#include "core/file_format/npbind.h"
#include "core/file_format/psf.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/network/http/http.h"
#include "core/libraries/network/http2/http2.h"
#include "core/libraries/network/net/net.h"
#include "core/libraries/network/net_ctl/net_ctl.h"
#include "core/libraries/np/np_auth/np_auth.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_manager/np_manager.h"
#include "core/libraries/np/np_matching2/np_matching2.h"
#include "core/libraries/np/np_score/np_score.h"
#include "core/libraries/np/np_signaling/np_signaling.h"
#include "core/libraries/np/np_tus/np_tus.h"
#include "core/libraries/np/np_utility/np_utility.h"
#include "core/libraries/np/np_web_api/np_web_api.h"
#include "core/libraries/user_service/user_service.h"
#include "shadnet/config.h"

extern "C" s32 client_start() {
    LOG_INFO(shadNet, "Initializing shadNet Client");
    // Preload modules used by the plugin
    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NET);
    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NETCTL);
    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SYS_UTIL);
    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_USER_SERVICE);
    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NP_MANAGER);
    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NP_WEB_API);
    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_HTTP);
    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_HTTP2);

    sceSysmoduleLoadModule(ORBIS_SYSMODULE_NP_SCORE_RANKING);
    sceSysmoduleLoadModule(ORBIS_SYSMODULE_NP_AUTH);
    sceSysmoduleLoadModule(ORBIS_SYSMODULE_NP_SIGNALING);
    sceSysmoduleLoadModule(ORBIS_SYSMODULE_NP_MATCHING2);
    sceSysmoduleLoadModule(ORBIS_SYSMODULE_NP_UTILITY);
    sceSysmoduleLoadModule(ORBIS_SYSMODULE_NP_TUS);

    // Init system and network library hooks
    Libraries::Kernel::RegisterHooks();
    Libraries::UserService::RegisterHooks();
    Libraries::Network::Net::RegisterHooks();
    Libraries::Network::NetCtl::RegisterHooks();
    Libraries::Network::Http::RegisterHooks();
    Libraries::Network::Http2::RegisterHooks();

    // Init config backend
    ShadNet::Settings::GetInstance().Initialize();

    // Initialize elfinfo
    auto& game_info = Common::ElfInfo::Instance();

    // Retrieve as much game metadata as we can
    OrbisAppInfo app_info{};
    s32 result = sceKernelGetAppInfo(getpid(), &app_info);
    if (result != 0) {
        LOG_ERROR(shadNet, "sceKernelGetAppInfo failed!");
        return 1;
    }

    sceKernelGetCompiledSdkVersion(reinterpret_cast<s32*>(&game_info.sdk_ver));
    game_info.initialized = true;
    game_info.game_serial = std::string{app_info.TitleId};

    bool sandbox_psf = false;
    bool sandbox_npbind = false;

    // Define helpers for loading these files, since we're checking multiple paths.
    auto load_psf = [&](std::string path) {
        PSF* param_sfo = Common::Singleton<PSF>::Instance();
        if (!param_sfo->Open(path)) {
            return false;
        }

        // Load game name, version, required firmware, and other param.sfo attributes
        game_info.title = param_sfo->GetString("TITLE").value_or("Unknown Game");
        game_info.firmware_ver = param_sfo->GetInteger("SYSTEM_VER").value_or(game_info.sdk_ver);
        game_info.app_ver = param_sfo->GetString("APP_VER").value_or("1.00");
        if (const auto raw_attributes = param_sfo->GetInteger("ATTRIBUTE")) {
            game_info.psf_attributes.raw = *raw_attributes;
        }
        return true;
    };
    auto load_npbind = [&](std::string path) {
        NPBindFile npbind;
        if (!npbind.Load(path)) {
            return false;
        }
        game_info.npCommIds = npbind.GetNpCommIds();
        return true;
    };

    // Start by checking within the sandbox.
    // Mounted PKGs won't mount these files here, but unpacked dumps usually contain them.
    // std::filesystem::exists causes crashes when reading from PKGs, so use sceKernelStat instead.
    OrbisKernelStat statbuf{};
    if (sceKernelStat("/app0/sce_sys/param.sfo", &statbuf) == 0) {
        sandbox_psf = load_psf("/app0/sce_sys/param.sfo");
        if (!sandbox_psf) {
            LOG_WARNING(shadNet, "Failed to load param.sfo from sandbox");
        }
    }
    if (sceKernelStat("/app0/sce_sys/npbind.dat", &statbuf) == 0) {
        sandbox_npbind = load_npbind("/app0/sce_sys/npbind.dat");
        if (!sandbox_npbind) {
            LOG_WARNING(shadNet, "Failed to load npbind.dat from sandbox");
        }
    }

    // Sandbox doesn't contain our files, jailbreak this process and look in the correct places.
    if (!sandbox_psf || !sandbox_npbind) {
        jbc_cred cred;
        jbc_get_cred(&cred);
        jbc_cred root_cred = cred;
        jbc_jailbreak_cred(&root_cred);
        root_cred.cdir = cred.rdir;
        jbc_set_cred(&root_cred);

        // Check files in appmeta
        std::string meta_path = "/system_data/priv/appmeta/" + std::string(app_info.TitleId);
        if (!sandbox_psf) {
            if (std::filesystem::exists(meta_path + "/param.sfo")) {
                if (!load_psf(meta_path + "/param.sfo")) {
                    LOG_WARNING(shadNet, "Failed to load param.sfo from system_data");
                }
            } else {
                LOG_WARNING(shadNet, "no param.sfo in {}", meta_path);
            }
        }
        if (!sandbox_npbind) {
            if (std::filesystem::exists(meta_path + "/npbind.dat")) {
                if (!load_npbind(meta_path + "/npbind.dat")) {
                    LOG_WARNING(shadNet, "Failed to load npbind.dat from system_data");
                }
            } else {
                LOG_WARNING(shadNet, "no npbind.dat in {}", meta_path);
            }
        }

        // Un-jailbreak the process by restoring the original cred.
        jbc_set_cred(&cred);
    }

    // Log the important info we retrieve
    if (game_info.title.size() > 0) {
        LOG_INFO(shadNet, "Currently running {} ({}) v{}", game_info.title, game_info.game_serial,
                 game_info.app_ver);
    } else {
        LOG_INFO(shadNet, "Currently running {} v{}", game_info.game_serial, game_info.app_ver);
    }

    if (game_info.npCommIds.size() > 0) {
        LOG_INFO(shadNet, "NP Communication IDs:");
        for (s32 i = 0; i < game_info.npCommIds.size(); i++) {
            LOG_INFO(shadNet, "{}: {}", i, game_info.npCommIds[i]);
        }
    } else {
        LOG_WARNING(shadNet, "No NP Communication IDs loaded");
    }

    // Init NP library hooks
    Libraries::Np::NpManager::RegisterHooks();
    Libraries::Np::NpScore::RegisterHooks();
    Libraries::Np::NpWebApi::RegisterHooks();
    Libraries::Np::NpAuth::RegisterHooks();
    Libraries::Np::NpSignaling::RegisterHooks();
    Libraries::Np::NpMatching2::RegisterHooks();
    Libraries::Np::NpUtility::RegisterHooks();
    Libraries::Np::NpTus::RegisterHooks();
    return 0;
}
