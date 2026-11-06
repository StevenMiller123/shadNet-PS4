// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/rtc/rtc.h"
#include "core/libraries/user_service/user_service.h"

namespace Libraries::Np::NpTrophy {

constexpr s32 ORBIS_NP_TROPHY_FLAG_SETSIZE = 128;
constexpr s32 ORBIS_NP_TROPHY_FLAG_BITS_SHIFT = 5;

constexpr s32 ORBIS_NP_TROPHY_GAME_TITLE_MAX_SIZE = 128;
constexpr s32 ORBIS_NP_TROPHY_GAME_DESCR_MAX_SIZE = 1024;
constexpr s32 ORBIS_NP_TROPHY_GROUP_TITLE_MAX_SIZE = 128;
constexpr s32 ORBIS_NP_TROPHY_GROUP_DESCR_MAX_SIZE = 1024;
constexpr s32 ORBIS_NP_TROPHY_NAME_MAX_SIZE = 128;
constexpr s32 ORBIS_NP_TROPHY_DESCR_MAX_SIZE = 1024;
constexpr s32 ORBIS_NP_TROPHY_NUM_MAX = 128;

constexpr s32 ORBIS_NP_TROPHY_INVALID_HANDLE = -1;
constexpr s32 ORBIS_NP_TROPHY_INVALID_CONTEXT = -1;
constexpr s32 ORBIS_NP_TROPHY_INVALID_TROPHY_ID = -1;

using OrbisNpTrophyHandle = s32;
using OrbisNpTrophyContext = s32;
using OrbisNpTrophyId = s32;
using OrbisNpTrophyFlagMask = u32;

using OrbisNpTrophyGroupId = s32;
constexpr s32 ORBIS_NP_TROPHY_BASE_GAME_GROUP_ID = -1;
constexpr s32 ORBIS_NP_TROPHY_INVALID_GROUP_ID = -2;

enum OrbisNpTrophyGrade : s32 {
    ORBIS_NP_TROPHY_GRADE_UNKNOWN,
    ORBIS_NP_TROPHY_GRADE_PLATINUM,
    ORBIS_NP_TROPHY_GRADE_GOLD,
    ORBIS_NP_TROPHY_GRADE_SILVER,
    ORBIS_NP_TROPHY_GRADE_BRONZE
};

struct OrbisNpTrophyData {
    u64 size;
    OrbisNpTrophyId trophy_id;
    bool unlocked;
    u8 reserved[3];
    Rtc::OrbisRtcTick timestamp;
};

struct OrbisNpTrophyDetails {
    u64 size;
    OrbisNpTrophyId trophy_id;
    OrbisNpTrophyGrade trophy_grade;
    OrbisNpTrophyGroupId group_id;
    bool hidden;
    u8 reserved[3];
    char name[ORBIS_NP_TROPHY_NAME_MAX_SIZE];
    char description[ORBIS_NP_TROPHY_DESCR_MAX_SIZE];
};

struct OrbisNpTrophyGameData {
    u64 size;
    u32 unlocked_trophies;
    u32 unlocked_platinum;
    u32 unlocked_gold;
    u32 unlocked_silver;
    u32 unlocked_bronze;
    u32 progress_percentage;
};

struct OrbisNpTrophyGameDetails {
    u64 size;
    u32 num_groups;
    u32 num_trophies;
    u32 num_platinum;
    u32 num_gold;
    u32 num_silver;
    u32 num_bronze;
    char title[ORBIS_NP_TROPHY_GAME_TITLE_MAX_SIZE];
    char description[ORBIS_NP_TROPHY_GAME_DESCR_MAX_SIZE];
};

struct OrbisNpTrophyGroupData {
    u64 size;
    OrbisNpTrophyGroupId group_id;
    u32 unlocked_trophies;
    u32 unlocked_platinum;
    u32 unlocked_gold;
    u32 unlocked_silver;
    u32 unlocked_bronze;
    u32 progress_percentage;
    u8 reserved[4];
};

struct OrbisNpTrophyGroupDetails {
    u64 size;
    OrbisNpTrophyGroupId group_id;
    u32 num_trophies;
    u32 num_platinum;
    u32 num_gold;
    u32 num_silver;
    u32 num_bronze;
    char title[ORBIS_NP_TROPHY_GROUP_TITLE_MAX_SIZE];
    char description[ORBIS_NP_TROPHY_GROUP_DESCR_MAX_SIZE];
};

s32 sceNpTrophyCreateContext(OrbisNpTrophyContext* context,
                             UserService::OrbisUserServiceUserId user_id, u32 service_label,
                             u64 options);
s32 sceNpTrophyDestroyContext(OrbisNpTrophyContext context);
s32 sceNpTrophyRegisterContext(OrbisNpTrophyContext context, OrbisNpTrophyHandle handle,
                               u64 options);
s32 sceNpTrophyUnlockTrophy(OrbisNpTrophyContext context, OrbisNpTrophyHandle handle,
                            OrbisNpTrophyId trophy_id, OrbisNpTrophyId* platinum_id);

void RegisterHooks();
} // namespace Libraries::Np::NpTrophy