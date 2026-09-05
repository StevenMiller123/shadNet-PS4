// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::Np::NpScore {

s32 sceNpScoreCreateNpTitleCtx();
s32 sceNpScoreGetFriendsRanking();
s32 sceNpScoreGetFriendsRankingAsync();
s32 sceNpScoreGetGameData();
s32 sceNpScoreGetGameDataAsync();
s32 sceNpScoreGetRankingByNpId();
s32 sceNpScoreGetRankingByNpIdAsync();
s32 sceNpScoreGetRankingByNpIdPcId();
s32 sceNpScoreGetRankingByNpIdPcIdAsync();
s32 sceNpScoreGetRankingByRange();
s32 sceNpScoreGetRankingByRangeAsync();
s32 sceNpScoreAbortRequest();
s32 sceNpScoreCensorComment();
s32 sceNpScoreCensorCommentAsync();
s32 sceNpScoreChangeModeForOtherSaveDataOwners();
s32 sceNpScoreCreateNpTitleCtxA();
s32 sceNpScoreCreateRequest();
s32 sceNpScoreCreateTitleCtx();
s32 sceNpScoreDeleteNpTitleCtx();
s32 sceNpScoreDeleteRequest();
s32 sceNpScoreGetBoardInfo();
s32 sceNpScoreGetBoardInfoAsync();
s32 sceNpScoreGetFriendsRankingA();
s32 sceNpScoreGetFriendsRankingAAsync();
s32 sceNpScoreGetFriendsRankingForCrossSave();
s32 sceNpScoreGetFriendsRankingForCrossSaveAsync();
s32 sceNpScoreGetGameDataByAccountId();
s32 sceNpScoreGetGameDataByAccountIdAsync();
s32 sceNpScoreGetRankingByAccountId();
s32 sceNpScoreGetRankingByAccountIdAsync();
s32 sceNpScoreGetRankingByAccountIdForCrossSave();
s32 sceNpScoreGetRankingByAccountIdForCrossSaveAsync();
s32 sceNpScoreGetRankingByAccountIdPcId();
s32 sceNpScoreGetRankingByAccountIdPcIdAsync();
s32 sceNpScoreGetRankingByAccountIdPcIdForCrossSave();
s32 sceNpScoreGetRankingByAccountIdPcIdForCrossSaveAsync();
s32 sceNpScoreGetRankingByRangeA();
s32 sceNpScoreGetRankingByRangeAAsync();
s32 sceNpScoreGetRankingByRangeForCrossSave();
s32 sceNpScoreGetRankingByRangeForCrossSaveAsync();
s32 sceNpScorePollAsync();
s32 sceNpScoreRecordGameData();
s32 sceNpScoreRecordGameDataAsync();
s32 sceNpScoreRecordScore();
s32 sceNpScoreRecordScoreAsync();
s32 sceNpScoreSanitizeComment();
s32 sceNpScoreSanitizeCommentAsync();
s32 sceNpScoreSetPlayerCharacterId();
s32 sceNpScoreSetThreadParam();
s32 sceNpScoreSetTimeout();
s32 sceNpScoreWaitAsync();

void RegisterHooks();
} // namespace Libraries::NpScore