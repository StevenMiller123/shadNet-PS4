// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "common/plugin_common.h"
#include "core/libraries/np/np_score.h"

#include "orbis/NpScore.h"

HOOK_INIT(sceNpScoreCreateNpTitleCtx);
static s32 sceNpScoreCreateNpTitleCtx_hook() {
    return Libraries::Np::NpScore::sceNpScoreCreateNpTitleCtx();
}

HOOK_INIT(sceNpScoreGetFriendsRanking);
static s32 sceNpScoreGetFriendsRanking_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetFriendsRanking();
}

HOOK_INIT(sceNpScoreGetFriendsRankingAsync);
static s32 sceNpScoreGetFriendsRankingAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetFriendsRankingAsync();
}

HOOK_INIT(sceNpScoreGetGameData);
static s32 sceNpScoreGetGameData_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetGameData();
}

HOOK_INIT(sceNpScoreGetGameDataAsync);
static s32 sceNpScoreGetGameDataAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetGameDataAsync();
}

HOOK_INIT(sceNpScoreGetRankingByNpId);
static s32 sceNpScoreGetRankingByNpId_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetRankingByNpId();
}

HOOK_INIT(sceNpScoreGetRankingByNpIdAsync);
static s32 sceNpScoreGetRankingByNpIdAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetRankingByNpIdAsync();
}

HOOK_INIT(sceNpScoreGetRankingByNpIdPcId);
static s32 sceNpScoreGetRankingByNpIdPcId_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetRankingByNpIdPcId();
}

HOOK_INIT(sceNpScoreGetRankingByNpIdPcIdAsync);
static s32 sceNpScoreGetRankingByNpIdPcIdAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetRankingByNpIdPcIdAsync();
}

HOOK_INIT(sceNpScoreGetRankingByRange);
static s32 sceNpScoreGetRankingByRange_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetRankingByRange();
}

HOOK_INIT(sceNpScoreGetRankingByRangeAsync);
static s32 sceNpScoreGetRankingByRangeAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetRankingByRangeAsync();
}

HOOK_INIT(sceNpScoreAbortRequest);
static s32 sceNpScoreAbortRequest_hook() {
    return Libraries::Np::NpScore::sceNpScoreAbortRequest();
}

HOOK_INIT(sceNpScoreCensorComment);
static s32 sceNpScoreCensorComment_hook() {
    return Libraries::Np::NpScore::sceNpScoreCensorComment();
}

HOOK_INIT(sceNpScoreCensorCommentAsync);
static s32 sceNpScoreCensorCommentAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreCensorCommentAsync();
}

HOOK_INIT(sceNpScoreChangeModeForOtherSaveDataOwners);
static s32 sceNpScoreChangeModeForOtherSaveDataOwners_hook() {
    return Libraries::Np::NpScore::sceNpScoreChangeModeForOtherSaveDataOwners();
}

HOOK_INIT(sceNpScoreCreateNpTitleCtxA);
static s32 sceNpScoreCreateNpTitleCtxA_hook() {
    return Libraries::Np::NpScore::sceNpScoreCreateNpTitleCtxA();
}

HOOK_INIT(sceNpScoreCreateRequest);
static s32 sceNpScoreCreateRequest_hook() {
    return Libraries::Np::NpScore::sceNpScoreCreateRequest();
}

HOOK_INIT(sceNpScoreCreateTitleCtx);
static s32 sceNpScoreCreateTitleCtx_hook() {
    return Libraries::Np::NpScore::sceNpScoreCreateTitleCtx();
}

HOOK_INIT(sceNpScoreDeleteNpTitleCtx);
static s32 sceNpScoreDeleteNpTitleCtx_hook() {
    return Libraries::Np::NpScore::sceNpScoreDeleteNpTitleCtx();
}

HOOK_INIT(sceNpScoreDeleteRequest);
static s32 sceNpScoreDeleteRequest_hook() {
    return Libraries::Np::NpScore::sceNpScoreDeleteRequest();
}

HOOK_INIT(sceNpScoreGetBoardInfo);
static s32 sceNpScoreGetBoardInfo_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetBoardInfo();
}

HOOK_INIT(sceNpScoreGetBoardInfoAsync);
static s32 sceNpScoreGetBoardInfoAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetBoardInfoAsync();
}

HOOK_INIT(sceNpScoreGetFriendsRankingA);
static s32 sceNpScoreGetFriendsRankingA_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetFriendsRankingA();
}

HOOK_INIT(sceNpScoreGetFriendsRankingAAsync);
static s32 sceNpScoreGetFriendsRankingAAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetFriendsRankingAAsync();
}

HOOK_INIT(sceNpScoreGetFriendsRankingForCrossSave);
static s32 sceNpScoreGetFriendsRankingForCrossSave_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetFriendsRankingForCrossSave();
}

HOOK_INIT(sceNpScoreGetFriendsRankingForCrossSaveAsync);
static s32 sceNpScoreGetFriendsRankingForCrossSaveAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetFriendsRankingForCrossSaveAsync();
}

HOOK_INIT(sceNpScoreGetGameDataByAccountId);
static s32 sceNpScoreGetGameDataByAccountId_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetGameDataByAccountId();
}

HOOK_INIT(sceNpScoreGetGameDataByAccountIdAsync);
static s32 sceNpScoreGetGameDataByAccountIdAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetGameDataByAccountIdAsync();
}

HOOK_INIT(sceNpScoreGetRankingByAccountId);
static s32 sceNpScoreGetRankingByAccountId_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetRankingByAccountId();
}

HOOK_INIT(sceNpScoreGetRankingByAccountIdAsync);
static s32 sceNpScoreGetRankingByAccountIdAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetRankingByAccountIdAsync();
}

HOOK_INIT(sceNpScoreGetRankingByAccountIdForCrossSave);
static s32 sceNpScoreGetRankingByAccountIdForCrossSave_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetRankingByAccountIdForCrossSave();
}

HOOK_INIT(sceNpScoreGetRankingByAccountIdForCrossSaveAsync);
static s32 sceNpScoreGetRankingByAccountIdForCrossSaveAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetRankingByAccountIdForCrossSaveAsync();
}

HOOK_INIT(sceNpScoreGetRankingByAccountIdPcId);
static s32 sceNpScoreGetRankingByAccountIdPcId_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetRankingByAccountIdPcId();
}

HOOK_INIT(sceNpScoreGetRankingByAccountIdPcIdAsync);
static s32 sceNpScoreGetRankingByAccountIdPcIdAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetRankingByAccountIdPcIdAsync();
}

HOOK_INIT(sceNpScoreGetRankingByAccountIdPcIdForCrossSave);
static s32 sceNpScoreGetRankingByAccountIdPcIdForCrossSave_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetRankingByAccountIdPcIdForCrossSave();
}

HOOK_INIT(sceNpScoreGetRankingByAccountIdPcIdForCrossSaveAsync);
static s32 sceNpScoreGetRankingByAccountIdPcIdForCrossSaveAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetRankingByAccountIdPcIdForCrossSaveAsync();
}

HOOK_INIT(sceNpScoreGetRankingByRangeA);
static s32 sceNpScoreGetRankingByRangeA_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetRankingByRangeA();
}

HOOK_INIT(sceNpScoreGetRankingByRangeAAsync);
static s32 sceNpScoreGetRankingByRangeAAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetRankingByRangeAAsync();
}

HOOK_INIT(sceNpScoreGetRankingByRangeForCrossSave);
static s32 sceNpScoreGetRankingByRangeForCrossSave_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetRankingByRangeForCrossSave();
}

HOOK_INIT(sceNpScoreGetRankingByRangeForCrossSaveAsync);
static s32 sceNpScoreGetRankingByRangeForCrossSaveAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreGetRankingByRangeForCrossSaveAsync();
}

HOOK_INIT(sceNpScorePollAsync);
static s32 sceNpScorePollAsync_hook() {
    return Libraries::Np::NpScore::sceNpScorePollAsync();
}

HOOK_INIT(sceNpScoreRecordGameData);
static s32 sceNpScoreRecordGameData_hook() {
    return Libraries::Np::NpScore::sceNpScoreRecordGameData();
}

HOOK_INIT(sceNpScoreRecordGameDataAsync);
static s32 sceNpScoreRecordGameDataAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreRecordGameDataAsync();
}

HOOK_INIT(sceNpScoreRecordScore);
static s32 sceNpScoreRecordScore_hook() {
    return Libraries::Np::NpScore::sceNpScoreRecordScore();
}

HOOK_INIT(sceNpScoreRecordScoreAsync);
static s32 sceNpScoreRecordScoreAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreRecordScoreAsync();
}

HOOK_INIT(sceNpScoreSanitizeComment);
static s32 sceNpScoreSanitizeComment_hook() {
    return Libraries::Np::NpScore::sceNpScoreSanitizeComment();
}

HOOK_INIT(sceNpScoreSanitizeCommentAsync);
static s32 sceNpScoreSanitizeCommentAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreSanitizeCommentAsync();
}

HOOK_INIT(sceNpScoreSetPlayerCharacterId);
static s32 sceNpScoreSetPlayerCharacterId_hook() {
    return Libraries::Np::NpScore::sceNpScoreSetPlayerCharacterId();
}

HOOK_INIT(sceNpScoreSetThreadParam);
static s32 sceNpScoreSetThreadParam_hook() {
    return Libraries::Np::NpScore::sceNpScoreSetThreadParam();
}

HOOK_INIT(sceNpScoreSetTimeout);
static s32 sceNpScoreSetTimeout_hook() {
    return Libraries::Np::NpScore::sceNpScoreSetTimeout();
}

HOOK_INIT(sceNpScoreWaitAsync);
static s32 sceNpScoreWaitAsync_hook() {
    return Libraries::Np::NpScore::sceNpScoreWaitAsync();
}

static void RegisterLibraryHooks() {
    HOOK(sceNpScoreCreateNpTitleCtx);
    HOOK(sceNpScoreGetFriendsRanking);
    HOOK(sceNpScoreGetFriendsRankingAsync);
    HOOK(sceNpScoreGetGameData);
    HOOK(sceNpScoreGetGameDataAsync);
    HOOK(sceNpScoreGetRankingByNpId);
    HOOK(sceNpScoreGetRankingByNpIdAsync);
    HOOK(sceNpScoreGetRankingByNpIdPcId);
    HOOK(sceNpScoreGetRankingByNpIdPcIdAsync);
    HOOK(sceNpScoreGetRankingByRange);
    HOOK(sceNpScoreGetRankingByRangeAsync);
    HOOK(sceNpScoreAbortRequest);
    HOOK(sceNpScoreCensorComment);
    HOOK(sceNpScoreCensorCommentAsync);
    HOOK(sceNpScoreChangeModeForOtherSaveDataOwners);
    HOOK(sceNpScoreCreateNpTitleCtx);
    HOOK(sceNpScoreCreateNpTitleCtxA);
    HOOK(sceNpScoreCreateRequest);
    HOOK(sceNpScoreCreateTitleCtx);
    HOOK(sceNpScoreDeleteNpTitleCtx);
    HOOK(sceNpScoreDeleteRequest);
    HOOK(sceNpScoreGetBoardInfo);
    HOOK(sceNpScoreGetBoardInfoAsync);
    HOOK(sceNpScoreGetFriendsRanking);
    HOOK(sceNpScoreGetFriendsRankingA);
    HOOK(sceNpScoreGetFriendsRankingAAsync);
    HOOK(sceNpScoreGetFriendsRankingAsync);
    // HOOK(sceNpScoreGetFriendsRankingForCrossSave);
    // HOOK(sceNpScoreGetFriendsRankingForCrossSaveAsync);
    HOOK(sceNpScoreGetGameData);
    HOOK(sceNpScoreGetGameDataAsync);
    // HOOK(sceNpScoreGetGameDataByAccountId);
    // HOOK(sceNpScoreGetGameDataByAccountIdAsync);
    HOOK(sceNpScoreGetRankingByAccountId);
    HOOK(sceNpScoreGetRankingByAccountIdAsync);
    // HOOK(sceNpScoreGetRankingByAccountIdForCrossSave);
    // HOOK(sceNpScoreGetRankingByAccountIdForCrossSaveAsync);
    // HOOK(sceNpScoreGetRankingByAccountIdPcId);
    // HOOK(sceNpScoreGetRankingByAccountIdPcIdAsync);
    // HOOK(sceNpScoreGetRankingByAccountIdPcIdForCrossSave);
    // HOOK(sceNpScoreGetRankingByAccountIdPcIdForCrossSaveAsync);
    HOOK(sceNpScoreGetRankingByNpId);
    HOOK(sceNpScoreGetRankingByNpIdAsync);
    HOOK(sceNpScoreGetRankingByNpIdPcId);
    HOOK(sceNpScoreGetRankingByNpIdPcIdAsync);
    HOOK(sceNpScoreGetRankingByRange);
    HOOK(sceNpScoreGetRankingByRangeA);
    HOOK(sceNpScoreGetRankingByRangeAAsync);
    HOOK(sceNpScoreGetRankingByRangeAsync);
    // HOOK(sceNpScoreGetRankingByRangeForCrossSave);
    // HOOK(sceNpScoreGetRankingByRangeForCrossSaveAsync);
    HOOK(sceNpScorePollAsync);
    HOOK(sceNpScoreRecordGameData);
    HOOK(sceNpScoreRecordGameDataAsync);
    HOOK(sceNpScoreRecordScore);
    HOOK(sceNpScoreRecordScoreAsync);
    HOOK(sceNpScoreSanitizeComment);
    HOOK(sceNpScoreSanitizeCommentAsync);
    HOOK(sceNpScoreSetPlayerCharacterId);
    // HOOK(sceNpScoreSetThreadParam);
    HOOK(sceNpScoreSetTimeout);
    HOOK(sceNpScoreWaitAsync);
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
    return RegisterLibraryHooks();
}

} // namespace Libraries::Np::NpScore