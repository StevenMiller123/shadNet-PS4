// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <mutex>
#include <orbis/NpTrophy.h>

#include "common/shadnet_hook.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_trophy/np_trophy.h"

SHADNET_HOOK_DECLARE(Libraries::Np::NpTrophy, sceNpTrophyCreateContext);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTrophy, sceNpTrophyDestroyContext);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTrophy, sceNpTrophyRegisterContext);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTrophy, sceNpTrophyUnlockTrophy);

static void RegisterNpTrophyHooks() {
    SHADNET_HOOK(Libraries::Np::NpTrophy, sceNpTrophyCreateContext);
    SHADNET_HOOK(Libraries::Np::NpTrophy, sceNpTrophyDestroyContext);
    SHADNET_HOOK(Libraries::Np::NpTrophy, sceNpTrophyRegisterContext);
    SHADNET_HOOK(Libraries::Np::NpTrophy, sceNpTrophyUnlockTrophy);
}

namespace Libraries::Np::NpTrophy {

std::mutex lock{};
std::map<OrbisNpTrophyContext, std::pair<UserService::OrbisUserServiceUserId, u32>> contexts{};

s32 sceNpTrophyCreateContext(OrbisNpTrophyContext* context,
                             UserService::OrbisUserServiceUserId user_id, u32 service_label,
                             u64 options) {
    s32 ret =
        SHADNET_HOOK_CONTINUE(sceNpTrophyCreateContext, context, user_id, service_label, options);
    if (ret == 0) {
        std::scoped_lock lk{lock};
        contexts[*context] = {user_id, service_label};
        LOG_INFO(Lib_NpTrophy, "user_id = {}, service_label = {}", s32(user_id), service_label);
    }
    return ret;
}

s32 sceNpTrophyDestroyContext(OrbisNpTrophyContext context) {
    s32 ret = SHADNET_HOOK_CONTINUE(sceNpTrophyDestroyContext, context);
    if (ret == 0) {
        std::scoped_lock lk{lock};
        contexts.erase(context);
    }
    return ret;
}

static bool IsFlagSet(s32 trophy_id, OrbisNpTrophyFlagArray* p) {
    u32 array_index = trophy_id / 32;
    u32 bit_position = trophy_id % 32;
    return (p->flag_bits[array_index] & (1U << bit_position)) ? 1 : 0;
}

static std::vector<std::pair<s32, u64>> ReadUnlockedTrophies(OrbisNpTrophyContext context,
                                                             OrbisNpTrophyHandle handle) {
    std::vector<std::pair<s32, u64>> out;
    ::OrbisNpTrophyFlagArray flags{};
    u32 count{};
    s32 ret = sceNpTrophyGetTrophyUnlockState(context, handle, &flags, &count);
    if (ret == 0) {
        for (s32 trophy_id = 0; trophy_id < ORBIS_NP_TROPHY_NUM_MAX && count > 0; trophy_id++) {
            if (IsFlagSet(trophy_id, &flags)) {
                // Need to get the timestamp
                u64 timestamp{};
                ::OrbisNpTrophyDetails details{};
                details.size = sizeof(::OrbisNpTrophyDetails);
                ::OrbisNpTrophyData data{};
                data.size = sizeof(::OrbisNpTrophyData);
                ret = sceNpTrophyGetTrophyInfo(context, handle, trophy_id, &details, &data);
                if (ret == 0) {
                    // Convert rtc ticks to seconds from unix epoch
                    timestamp = (data.timestamp - Rtc::UNIX_EPOCH_TICKS) / 1000000;
                } else {
                    LOG_ERROR(Lib_NpTrophy, "sceNpTrophyGetTrophyInfo returned {:#x}", u32(ret));
                }
                out.emplace_back(trophy_id, timestamp);
            }
        }
    } else {
        LOG_ERROR(Lib_NpTrophy, "sceNpTrophyGetTrophyUnlockState returned {:#x}", u32(ret));
    }
    return out;
}

// Need to differentiate between server-applied trophies and actual game unlocks.
static bool server_applied = false;

static u32 ApplyServerTrophies(OrbisNpTrophyContext context,
                               const std::vector<std::pair<s32, u64>>& server_trophies) {
    if (server_trophies.empty()) {
        return 0;
    }

    OrbisNpTrophyHandle handle{};
    s32 ret = sceNpTrophyCreateHandle(&handle);
    if (ret != 0) {
        LOG_ERROR(Lib_NpTrophy, "sceNpTrophyCreateHandle returned {:#x}", u32(ret));
        return 0;
    }

    // Unlock all the trophies shadNet reports
    u32 applied = 0;
    for (auto& [trophy_id, timestamp] : server_trophies) {
        // We don't have control over unlock timestamps, so ignore them.
        server_applied = true;
        OrbisNpTrophyId plat_id = ORBIS_NP_TROPHY_INVALID_TROPHY_ID;
        ret = sceNpTrophyUnlockTrophy(context, handle, trophy_id, &plat_id);
        if (ret != 0 && ret != ORBIS_NP_TROPHY_ERROR_TROPHY_ALREADY_UNLOCKED) {
            LOG_ERROR(Lib_NpTrophy, "sceNpTrophyUnlockTrophy returned {:#x}", u32(ret));
        } else {
            applied++;
        }
        server_applied = false;
    }

    sceNpTrophyDestroyHandle(handle);
    return applied;
}

s32 sceNpTrophyRegisterContext(OrbisNpTrophyContext context, OrbisNpTrophyHandle handle,
                               u64 options) {
    s32 ret = SHADNET_HOOK_CONTINUE(sceNpTrophyRegisterContext, context, handle, options);
    if (ret == 0) {
        std::scoped_lock lk{lock};
        UserService::OrbisUserServiceUserId user_id = contexts[context].first;
        u32 service_label = contexts[context].second;

        NpHandler::GetInstance().SyncTrophies(
            user_id, service_label, ReadUnlockedTrophies(context, handle),
            [context](const std::vector<std::pair<s32, u64>>& merged) {
                const u32 applied = ApplyServerTrophies(context, merged);
                if (applied > 0) {
                    LOG_INFO(Lib_NpTrophy, "Applied {} trophies from shadNet", applied);
                }
            });
    }
    return ret;
}

s32 sceNpTrophyUnlockTrophy(OrbisNpTrophyContext context, OrbisNpTrophyHandle handle,
                            OrbisNpTrophyId trophy_id, OrbisNpTrophyId* platinum_id) {
    s32 ret =
        SHADNET_HOOK_CONTINUE(sceNpTrophyUnlockTrophy, context, handle, trophy_id, platinum_id);
    if (ret == 0 && !server_applied) {
        // Game reported a trophy unlock, report this to shadNet.
        std::scoped_lock lk{lock};
        UserService::OrbisUserServiceUserId user_id = contexts[context].first;
        u32 service_label = contexts[context].second;

        // Retrieve the real timestamp
        u64 timestamp{};
        ::OrbisNpTrophyDetails details{};
        details.size = sizeof(::OrbisNpTrophyDetails);
        ::OrbisNpTrophyData data{};
        data.size = sizeof(::OrbisNpTrophyData);
        ret = sceNpTrophyGetTrophyInfo(context, handle, trophy_id, &details, &data);
        if (ret == 0) {
            // Convert rtc ticks to seconds from unix epoch
            timestamp = (data.timestamp - Rtc::UNIX_EPOCH_TICKS) / 1000000;
        } else {
            LOG_ERROR(Lib_NpTrophy, "sceNpTrophyGetTrophyInfo returned {:#x}", ret);
        }
        auto& np = NpHandler::GetInstance();
        np.ReportTrophyUnlock(user_id, service_label, trophy_id, timestamp);
        if (*platinum_id != ORBIS_NP_TROPHY_INVALID_TROPHY_ID) {
            // Platinum trophy unlocked
            np.ReportTrophyUnlock(user_id, service_label, *platinum_id, timestamp);
        }
    }
    return ret;
}

void RegisterHooks() {
    return RegisterNpTrophyHooks();
}
} // namespace Libraries::Np::NpTrophy