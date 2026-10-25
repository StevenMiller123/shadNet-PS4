// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/NpScore.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/np/np_score/np_score.h"

extern "C" {
void sceNpScoreGetFriendsRankingForCrossSave();
void sceNpScoreGetFriendsRankingForCrossSaveAsync();
void sceNpScoreGetGameDataByAccountId();
void sceNpScoreGetGameDataByAccountIdAsync();
void sceNpScoreGetRankingByAccountIdForCrossSave();
void sceNpScoreGetRankingByAccountIdForCrossSaveAsync();
void sceNpScoreGetRankingByAccountIdPcId();
void sceNpScoreGetRankingByAccountIdPcIdAsync();
void sceNpScoreGetRankingByAccountIdPcIdForCrossSave();
void sceNpScoreGetRankingByAccountIdPcIdForCrossSaveAsync();
void sceNpScoreGetRankingByRangeForCrossSave();
void sceNpScoreGetRankingByRangeForCrossSaveAsync();
void sceNpScoreSetThreadParam();
}

SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreCreateNpTitleCtx);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetFriendsRanking);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetFriendsRankingAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetGameData);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetGameDataAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetRankingByNpId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetRankingByNpIdAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetRankingByNpIdPcId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetRankingByNpIdPcIdAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetRankingByRange);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetRankingByRangeAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreAbortRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreCensorComment);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreCensorCommentAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreChangeModeForOtherSaveDataOwners);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreCreateNpTitleCtxA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreCreateRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreCreateTitleCtx);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreDeleteNpTitleCtx);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreDeleteRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetBoardInfo);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetBoardInfoAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetFriendsRankingA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetFriendsRankingAAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetFriendsRankingForCrossSave);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetFriendsRankingForCrossSaveAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetGameDataByAccountId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetGameDataByAccountIdAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetRankingByAccountId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetRankingByAccountIdAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetRankingByAccountIdForCrossSave);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetRankingByAccountIdForCrossSaveAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetRankingByAccountIdPcId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetRankingByAccountIdPcIdAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetRankingByAccountIdPcIdForCrossSave);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetRankingByAccountIdPcIdForCrossSaveAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetRankingByRangeA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetRankingByRangeAAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetRankingByRangeForCrossSave);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreGetRankingByRangeForCrossSaveAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScorePollAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreRecordGameData);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreRecordGameDataAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreRecordScore);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreRecordScoreAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreSanitizeComment);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreSanitizeCommentAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreSetPlayerCharacterId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreSetThreadParam);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreSetTimeout);
SHADNET_HOOK_DECLARE(Libraries::Np::NpScore, sceNpScoreWaitAsync);

static void RegisterNpScoreHooks() {
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreCreateNpTitleCtx);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetFriendsRanking);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetFriendsRankingAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetGameData);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetGameDataAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetRankingByNpId);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetRankingByNpIdAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetRankingByNpIdPcId);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetRankingByNpIdPcIdAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetRankingByRange);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetRankingByRangeAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreAbortRequest);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreCensorComment);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreCensorCommentAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreChangeModeForOtherSaveDataOwners);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreCreateNpTitleCtxA);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreCreateRequest);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreCreateTitleCtx);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreDeleteNpTitleCtx);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreDeleteRequest);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetBoardInfo);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetBoardInfoAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetFriendsRankingA);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetFriendsRankingAAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetFriendsRankingForCrossSave);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetFriendsRankingForCrossSaveAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetGameDataByAccountId);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetGameDataByAccountIdAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetRankingByAccountId);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetRankingByAccountIdAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetRankingByAccountIdForCrossSave);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetRankingByAccountIdForCrossSaveAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetRankingByAccountIdPcId);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetRankingByAccountIdPcIdAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetRankingByAccountIdPcIdForCrossSave);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetRankingByAccountIdPcIdForCrossSaveAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetRankingByRangeA);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetRankingByRangeAAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetRankingByRangeForCrossSave);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreGetRankingByRangeForCrossSaveAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScorePollAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreRecordGameData);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreRecordGameDataAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreRecordScore);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreRecordScoreAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreSanitizeComment);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreSanitizeCommentAsync);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreSetPlayerCharacterId);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreSetThreadParam);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreSetTimeout);
    SHADNET_HOOK(Libraries::Np::NpScore, sceNpScoreWaitAsync);
}

namespace Libraries::Np::NpScore {

s32 sceNpScoreCreateNpTitleCtx() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetFriendsRanking() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetFriendsRankingAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetGameData() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetGameDataAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByNpId() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByNpIdAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByNpIdPcId() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByNpIdPcIdAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByRange() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByRangeAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreAbortRequest() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreCensorComment() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreCensorCommentAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreChangeModeForOtherSaveDataOwners() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreCreateNpTitleCtxA() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreCreateRequest() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreCreateTitleCtx() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreDeleteNpTitleCtx() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreDeleteRequest() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetBoardInfo() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetBoardInfoAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetFriendsRankingA() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetFriendsRankingAAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetFriendsRankingForCrossSave() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetFriendsRankingForCrossSaveAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetGameDataByAccountId() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetGameDataByAccountIdAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByAccountId() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByAccountIdAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByAccountIdForCrossSave() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByAccountIdForCrossSaveAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByAccountIdPcId() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByAccountIdPcIdAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByAccountIdPcIdForCrossSave() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByAccountIdPcIdForCrossSaveAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByRangeA() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByRangeAAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByRangeForCrossSave() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByRangeForCrossSaveAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScorePollAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreRecordGameData() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreRecordGameDataAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreRecordScore() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreRecordScoreAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreSanitizeComment() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreSanitizeCommentAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreSetPlayerCharacterId() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreSetThreadParam() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreSetTimeout() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreWaitAsync() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterHooks() {
    return RegisterNpScoreHooks();
}

} // namespace Libraries::Np::NpScore