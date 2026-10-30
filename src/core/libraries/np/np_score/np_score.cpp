// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <orbis/NpScore.h>

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_score/np_score.h"
#include "core/libraries/np/np_score/np_score_ctx.h"

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

#define PTR(ptr) static_cast<const void*>(ptr)

struct ScoreTitleCtx {
    OrbisNpServiceLabel service_label = 0;
    s32 user_id = -1;
    OrbisNpScorePcId pc_id = 0;
};

static std::mutex g_mutex;
static std::map<OrbisNpScoreTitleCtxId, ScoreTitleCtx> g_title_ctxs;
static std::map<OrbisNpScoreRequestId, std::shared_ptr<ScoreRequestCtx>> g_requests;
static OrbisNpScoreTitleCtxId g_next_ctx_id = 1;
static OrbisNpScoreRequestId g_next_req_id = 1;
static s32 g_firmware_version = -1;

static ScoreTitleCtx* LookupTitleCtxUnlocked(OrbisNpScoreTitleCtxId id) {
    auto it = g_title_ctxs.find(id);
    return it == g_title_ctxs.end() ? nullptr : &it->second;
}

static std::shared_ptr<ScoreRequestCtx> LookupRequestUnlocked(OrbisNpScoreRequestId id) {
    auto it = g_requests.find(id);
    return it == g_requests.end() ? nullptr : it->second;
}

static s32 ServiceLabelForRequest(const std::shared_ptr<ScoreRequestCtx>& req) {
    if (!req) {
        return 0;
    }
    std::lock_guard lock(g_mutex);
    auto* tc = LookupTitleCtxUnlocked(req->title_ctx_id);
    return tc != nullptr ? tc->service_label : 0;
}

s32 sceNpScoreCreateNpTitleCtx(OrbisNpServiceLabel service_label, const OrbisNpId* self_np_id) {
    if (service_label == static_cast<OrbisNpServiceLabel>(ORBIS_NP_INVALID_SERVICE_LABEL)) {
        LOG_ERROR(Lib_NpScore, "invalid service label");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!self_np_id) {
        LOG_ERROR(Lib_NpScore, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    std::lock_guard lock(g_mutex);
    if (static_cast<s32>(g_title_ctxs.size()) >= ORBIS_NP_SCORE_MAX_CTX_NUM) {
        LOG_ERROR(Lib_NpScore, "too many title contexts");
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
    }
    const s32 user_id = NpHandler::GetInstance().GetUserIdByOnlineId(self_np_id->handle);
    const OrbisNpScoreTitleCtxId id = g_next_ctx_id++;
    g_title_ctxs[id] = ScoreTitleCtx{.service_label = service_label, .user_id = user_id};
    LOG_INFO(Lib_NpScore, "id={} service_label={} user_id={}", id, service_label, user_id);
    return id;
}

s32 sceNpScoreCreateNpTitleCtxA(OrbisNpServiceLabel np_service_label,
                                UserService::OrbisUserServiceUserId self_id) {

    if (!NpHandler::GetInstance().IsPsnSignedIn(self_id)) {
        LOG_ERROR(Lib_NpScore, "user {} not signed in", self_id);
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    if (np_service_label == static_cast<OrbisNpServiceLabel>(ORBIS_NP_INVALID_SERVICE_LABEL)) {
        LOG_ERROR(Lib_NpScore, "invalid service label");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    std::lock_guard lock(g_mutex);
    if (static_cast<s32>(g_title_ctxs.size()) >= ORBIS_NP_SCORE_MAX_CTX_NUM) {
        LOG_ERROR(Lib_NpScore, "too many title contexts");
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
    }
    const OrbisNpScoreTitleCtxId id = g_next_ctx_id++;
    g_title_ctxs[id] = ScoreTitleCtx{.service_label = np_service_label, .user_id = self_id};
    LOG_INFO(Lib_NpScore, "id={} service_label={} user_id={}", id, np_service_label, self_id);
    return id;
}

s32 sceNpScoreDeleteNpTitleCtx(s32 title_ctx_id) {
    std::lock_guard lock(g_mutex);
    if (!g_title_ctxs.contains(title_ctx_id)) {
        LOG_ERROR(Lib_NpScore, "invalid title context {}", title_ctx_id);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }

    for (auto it = g_requests.begin(); it != g_requests.end();) {
        if (it->second->title_ctx_id == title_ctx_id) {
            it->second->SetResult(ORBIS_NP_COMMUNITY_ERROR_ABORTED);
            it = g_requests.erase(it);
        } else {
            ++it;
        }
    }
    g_title_ctxs.erase(title_ctx_id);
    LOG_INFO(Lib_NpScore, "id={}", title_ctx_id);
    return ORBIS_OK;
}

s32 sceNpScoreCreateRequest(s32 title_ctx_id) {
    std::lock_guard lock(g_mutex);
    auto* tc = LookupTitleCtxUnlocked(title_ctx_id);
    if (!tc) {
        LOG_ERROR(Lib_NpScore, "invalid title context {}", title_ctx_id);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    if (static_cast<s32>(g_requests.size()) >= ORBIS_NP_SCORE_MAX_CTX_NUM) {
        LOG_ERROR(Lib_NpScore, "too many requests");
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
    }
    const OrbisNpScoreRequestId id = g_next_req_id++;
    auto req = std::make_shared<ScoreRequestCtx>();
    req->title_ctx_id = title_ctx_id;
    req->user_id = tc->user_id;
    req->pc_id = tc->pc_id;
    g_requests[id] = std::move(req);
    LOG_INFO(Lib_NpScore, "id={} title_ctx_id={}", id, title_ctx_id);
    return id;
}

s32 sceNpScoreDeleteRequest(s32 req_id) {
    LOG_INFO(Lib_NpScore, "req_id={}", req_id);
    std::lock_guard lock(g_mutex);
    auto req = LookupRequestUnlocked(req_id);
    if (!req) {
        LOG_ERROR(Lib_NpScore, "invalid request id {}", req_id);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    req->SetResult(ORBIS_NP_COMMUNITY_ERROR_ABORTED);
    g_requests.erase(req_id);
    return ORBIS_OK;
}

s32 sceNpScoreAbortRequest(s32 req_id) {
    LOG_INFO(Lib_NpScore, "req_id={}", req_id);
    std::shared_ptr<ScoreRequestCtx> req;
    {
        std::lock_guard lock(g_mutex);
        req = LookupRequestUnlocked(req_id);
    }
    if (!req) {
        LOG_ERROR(Lib_NpScore, "invalid request id {}", req_id);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    req->SetResult(ORBIS_NP_COMMUNITY_ERROR_ABORTED);
    return ORBIS_OK;
}

s32 sceNpScorePollAsync(s32 req_id, s32* result) {
    std::shared_ptr<ScoreRequestCtx> req;
    {
        std::lock_guard lock(g_mutex);
        req = LookupRequestUnlocked(req_id);
    }
    if (!req) {
        LOG_ERROR(Lib_NpScore, "invalid request id {}", req_id);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    std::lock_guard rlock(req->mutex);
    if (!req->result.has_value()) {
        return 1;
    }
    if (result != nullptr) {
        *result = *req->result;
    }
    LOG_INFO(Lib_NpScore, "req_id={} completed (result={:#x})", req_id, *req->result);
    return 0;
}

s32 sceNpScoreWaitAsync(s32 req_id, s32* result) {
    std::shared_ptr<ScoreRequestCtx> req;
    {
        std::lock_guard lock(g_mutex);
        req = LookupRequestUnlocked(req_id);
    }
    if (!req) {
        LOG_ERROR(Lib_NpScore, "invalid request id {}", req_id);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    std::unique_lock rlock(req->mutex);
    req->cv.wait(rlock, [&] { return req->result.has_value(); });
    if (result != nullptr) {
        *result = *req->result;
    }
    LOG_INFO(Lib_NpScore, "req_id={} completed (result={:#x})", req_id, *req->result);
    return 0;
}

s32 sceNpScoreRecordScore(s32 req_id, OrbisNpScoreBoardId board_id, OrbisNpScoreValue score,
                          const OrbisNpScoreComment* score_comment,
                          const OrbisNpScoreGameInfo* game_info, OrbisNpScoreRankNumber* tmp_rank,
                          const Rtc::OrbisRtcTick* compare_date, void* option) {
    LOG_INFO(Lib_NpScore,
             "req_id={} board_id={} score={} score_comment={} game_info={} tmp_rank={} "
             "compare_date={} option={}",
             req_id, board_id, score, PTR(score_comment), PTR(game_info), PTR(tmp_rank),
             PTR(compare_date), PTR(option));

    if (option != nullptr) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (game_info != nullptr && game_info->info_size > ORBIS_NP_SCORE_GAMEINFO_MAXSIZE) {
        LOG_ERROR(Lib_NpScore, "invalid game info size {}", game_info->info_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::shared_ptr<ScoreRequestCtx> req;
    {
        std::lock_guard lock(g_mutex);
        req = LookupRequestUnlocked(req_id);
        if (!req) {
            LOG_ERROR(Lib_NpScore, "invalid request id {}", req_id);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        if (!req->BeginOp()) {
            return ORBIS_NP_COMMUNITY_ERROR_ABORTED;
        }
    }

    req->tmp_rank_out = tmp_rank;

    const char* comment_bytes = (score_comment != nullptr) ? score_comment->utf8_comment : nullptr;
    const u64 comment_len = (score_comment != nullptr) ? strnlen(score_comment->utf8_comment,
                                                                 ORBIS_NP_SCORE_COMMENT_MAXLEN)
                                                       : 0;
    const u8* game_info_bytes = (game_info != nullptr) ? game_info->data : nullptr;
    const u64 game_info_len = (game_info != nullptr) ? game_info->info_size : 0;

    const s32 dispatch_err = NpHandler::GetInstance().RecordScore(
        req->user_id, ServiceLabelForRequest(req), board_id, req->pc_id, score, comment_bytes,
        comment_len, game_info_bytes, game_info_len,
        compare_date != nullptr ? compare_date->tick : 0, req);
    if (dispatch_err != ORBIS_OK) {
        req->SetResult(dispatch_err);
        return dispatch_err;
    }
    return req->Wait();
}

s32 sceNpScoreRecordScoreAsync(s32 req_id, OrbisNpScoreBoardId board_id, OrbisNpScoreValue score,
                               const OrbisNpScoreComment* score_comment,
                               const OrbisNpScoreGameInfo* game_info,
                               OrbisNpScoreRankNumber* tmp_rank,
                               const Rtc::OrbisRtcTick* compare_date, void* option) {
    LOG_INFO(Lib_NpScore,
             "called, req_id={}, board_id={}, score={}, "
             "score_comment={}, game_info={}, tmp_rank={}, compare_date={}, option={}",
             req_id, board_id, score, PTR(score_comment), PTR(game_info), PTR(tmp_rank),
             PTR(compare_date), PTR(option));
    if (option != nullptr) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (game_info != nullptr && game_info->info_size > ORBIS_NP_SCORE_GAMEINFO_MAXSIZE) {
        LOG_ERROR(Lib_NpScore, "invalid game info size {}", game_info->info_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::shared_ptr<ScoreRequestCtx> req;
    {
        std::lock_guard lock(g_mutex);
        req = LookupRequestUnlocked(req_id);
        if (!req) {
            LOG_ERROR(Lib_NpScore, "invalid request id {}", req_id);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        if (!req->BeginOp()) {
            return ORBIS_NP_COMMUNITY_ERROR_ABORTED;
        }
    }

    req->tmp_rank_out = tmp_rank;

    const char* comment_bytes = (score_comment != nullptr) ? score_comment->utf8_comment : nullptr;
    const u64 comment_len = (score_comment != nullptr) ? strnlen(score_comment->utf8_comment,
                                                                 ORBIS_NP_SCORE_COMMENT_MAXLEN)
                                                       : 0;
    const u8* game_info_bytes = (game_info != nullptr) ? game_info->data : nullptr;
    const u64 game_info_len = (game_info != nullptr) ? game_info->info_size : 0;

    const s32 dispatch_err = NpHandler::GetInstance().RecordScore(
        req->user_id, ServiceLabelForRequest(req), board_id, req->pc_id, score, comment_bytes,
        comment_len, game_info_bytes, game_info_len,
        compare_date != nullptr ? compare_date->tick : 0, req);
    if (dispatch_err != ORBIS_OK) {
        req->SetResult(dispatch_err);
        return dispatch_err;
    }
    return ORBIS_OK;
}

static s32 GetRankingByRangeImpl(
    s32 req_id, OrbisNpScoreBoardId board_id, OrbisNpScoreRankNumber start_serial_rank,
    OrbisNpScoreRankData* rank_array, OrbisNpScoreRankDataA* a_rank_array_in, u64 rank_array_size,
    OrbisNpScoreComment* comment_array, u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
    u64 info_array_size, u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
    OrbisNpScoreRankNumber* total_record, void* option, bool is_async) {
    if (option != nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid option");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_num > ORBIS_NP_SCORE_MAX_RANGE_NUM_PER_REQUEST) {
        LOG_ERROR(Lib_NpScore, "too many entries {}", array_num);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (start_serial_rank == 0) {
        LOG_ERROR(Lib_NpScore, "invalid startSerialRank");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    const bool is_a_variant = (a_rank_array_in != nullptr);
    if (is_a_variant == (rank_array != nullptr)) {
        LOG_ERROR(Lib_NpScore, "invalid rank array");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (array_num == 0) {
        LOG_ERROR(Lib_NpScore, "invalid arrayNum");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }

    const u64 expected_size =
        array_num * (is_a_variant ? sizeof(OrbisNpScoreRankDataA) : sizeof(OrbisNpScoreRankData));
    if (rank_array_size != expected_size) {
        LOG_ERROR(Lib_NpScore, "invalid size {}", rank_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    // Zero whichever output buffer the caller supplied.
    if (is_a_variant) {
        std::memset(a_rank_array_in, 0, rank_array_size);
    } else {
        std::memset(rank_array, 0, rank_array_size);
    }
    if (comment_array != nullptr) {
        if (comment_array_size != array_num * sizeof(OrbisNpScoreComment)) {
            LOG_ERROR(Lib_NpScore, "invalid size");
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        std::memset(comment_array, 0, comment_array_size);
    }
    if (info_array != nullptr) {
        if (info_array_size != array_num * sizeof(OrbisNpScoreGameInfo)) {
            LOG_ERROR(Lib_NpScore, "invalid size");
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        std::memset(info_array, 0, info_array_size);
    }

    std::shared_ptr<ScoreRequestCtx> req;
    {
        std::lock_guard lock(g_mutex);
        req = LookupRequestUnlocked(req_id);
        if (!req) {
            LOG_ERROR(Lib_NpScore, "invalid request id {}", req_id);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        if (!req->BeginOp()) {
            return ORBIS_NP_COMMUNITY_ERROR_ABORTED;
        }
    }

    s32 dispatch_err;
    if (is_a_variant) {
        dispatch_err = NpHandler::GetInstance().GetRankingByRangeA(
            req->user_id, ServiceLabelForRequest(req), board_id, start_serial_rank,
            static_cast<u32>(array_num), a_rank_array_in, comment_array, info_array, last_sort_date,
            total_record, req);
    } else {
        dispatch_err = NpHandler::GetInstance().GetRankingByRange(
            req->user_id, ServiceLabelForRequest(req), board_id, start_serial_rank,
            static_cast<u32>(array_num), rank_array, comment_array, info_array, last_sort_date,
            total_record, req);
    }
    if (dispatch_err != ORBIS_OK) {
        req->SetResult(dispatch_err);
        return dispatch_err;
    }
    if (is_async) {
        return ORBIS_OK;
    }
    return req->Wait();
}
s32 sceNpScoreGetRankingByRange(s32 req_id, OrbisNpScoreBoardId board_id,
                                OrbisNpScoreRankNumber start_serial_rank,
                                OrbisNpScoreRankData* rank_array, u64 rank_array_size,
                                OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_INFO(Lib_NpScore, "called, req_id={}, board_id={}, start_serial_rank={}, array_num={}",
             req_id, board_id, start_serial_rank, array_num);
    return GetRankingByRangeImpl(req_id, board_id, start_serial_rank, rank_array, nullptr,
                                 rank_array_size, comment_array, comment_array_size, info_array,
                                 info_array_size, array_num, last_sort_date, total_record, option,
                                 false);
}

s32 sceNpScoreGetRankingByRangeAsync(s32 req_id, OrbisNpScoreBoardId board_id,
                                     OrbisNpScoreRankNumber start_serial_rank,
                                     OrbisNpScoreRankData* rank_array, u64 rank_array_size,
                                     OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                     OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                     u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                     OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_INFO(Lib_NpScore,
             "called, req_id={}, board_id={}, start_serial_rank={}, "
             "rank_array={}, rank_array_size={}, comment_array={}, comment_array_size={}, "
             "info_array={}, "
             "info_array_size={}, array_num={}, last_sort_date={}, total_record={}, option={}",
             req_id, board_id, start_serial_rank, PTR(rank_array), rank_array_size,
             PTR(comment_array), comment_array_size, PTR(info_array), info_array_size, array_num,
             PTR(last_sort_date), PTR(total_record), PTR(option));
    return GetRankingByRangeImpl(req_id, board_id, start_serial_rank, rank_array, nullptr,
                                 rank_array_size, comment_array, comment_array_size, info_array,
                                 info_array_size, array_num, last_sort_date, total_record, option,
                                 true);
}

s32 sceNpScoreGetRankingByRangeA(s32 req_id, OrbisNpScoreBoardId board_id,
                                 OrbisNpScoreRankNumber start_serial_rank,
                                 OrbisNpScoreRankDataA* rank_array, u64 rank_array_size,
                                 OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                 OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                 u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                 OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_INFO(Lib_NpScore,
             "called, req_id={}, board_id={}, start_serial_rank={}, "
             "rank_array={}, rank_array_size={}, comment_array={}, comment_array_size={}, "
             "info_array={}, "
             "info_array_size={}, array_num={}, last_sort_date={}, total_record={}, option={}",
             req_id, board_id, start_serial_rank, PTR(rank_array), rank_array_size,
             PTR(comment_array), comment_array_size, PTR(info_array), info_array_size, array_num,
             PTR(last_sort_date), PTR(total_record), PTR(option));
    return GetRankingByRangeImpl(req_id, board_id, start_serial_rank, nullptr, rank_array,
                                 rank_array_size, comment_array, comment_array_size, info_array,
                                 info_array_size, array_num, last_sort_date, total_record, option,
                                 false);
}

s32 sceNpScoreGetRankingByRangeAAsync(s32 req_id, OrbisNpScoreBoardId board_id,
                                      OrbisNpScoreRankNumber start_serial_rank,
                                      OrbisNpScoreRankDataA* rank_array, u64 rank_array_size,
                                      OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                      OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                      u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                      OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_INFO(Lib_NpScore,
             "called, req_id={}, board_id={}, start_serial_rank={}, "
             "rank_array={}, rank_array_size={}, comment_array={}, comment_array_size={}, "
             "info_array={}, "
             "info_array_size={}, array_num={}, last_sort_date={}, total_record={}, option={}",
             req_id, board_id, start_serial_rank, PTR(rank_array), rank_array_size,
             PTR(comment_array), comment_array_size, PTR(info_array), info_array_size, array_num,
             PTR(last_sort_date), PTR(total_record), PTR(option));
    return GetRankingByRangeImpl(req_id, board_id, start_serial_rank, nullptr, rank_array,
                                 rank_array_size, comment_array, comment_array_size, info_array,
                                 info_array_size, array_num, last_sort_date, total_record, option,
                                 true);
}

static s32 GetRankingByNpIdImpl(s32 req_id, OrbisNpScoreBoardId board_id,
                                const OrbisNpId* np_id_array, u64 np_id_array_size,
                                OrbisNpScorePlayerRankData* rank_array, u64 rank_array_size,
                                OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                OrbisNpScoreRankNumber* total_record, void* option, bool is_async) {
    if (option != nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_num > ORBIS_NP_SCORE_MAX_NPID_NUM_PER_REQUEST) {
        LOG_ERROR(Lib_NpScore, "too many npIds {}", array_num);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (np_id_array == nullptr || rank_array == nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (np_id_array_size != array_num * sizeof(OrbisNpId) ||
        rank_array_size != array_num * sizeof(OrbisNpScorePlayerRankData)) {
        LOG_ERROR(Lib_NpScore, "invalid size {}", rank_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    std::memset(rank_array, 0, rank_array_size);
    if (comment_array != nullptr) {
        if (comment_array_size != array_num * sizeof(OrbisNpScoreComment)) {
            LOG_ERROR(Lib_NpScore, "invalid size {}", comment_array_size);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        std::memset(comment_array, 0, comment_array_size);
    }
    if (info_array != nullptr) {
        if (info_array_size != array_num * sizeof(OrbisNpScoreGameInfo)) {
            LOG_ERROR(Lib_NpScore, "invalid size {}", info_array_size);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        std::memset(info_array, 0, info_array_size);
    }

    std::shared_ptr<ScoreRequestCtx> req;
    {
        std::lock_guard lock(g_mutex);
        req = LookupRequestUnlocked(req_id);
        if (!req) {
            LOG_ERROR(Lib_NpScore, "invalid request id {}", req_id);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        if (!req->BeginOp()) {
            return ORBIS_NP_COMMUNITY_ERROR_ABORTED;
        }
    }

    std::vector<std::string> np_ids;
    np_ids.reserve(array_num);
    for (u64 i = 0; i < array_num; ++i) {
        const char* data = np_id_array[i].handle.data;
        const u64 len = strnlen(data, sizeof(np_id_array[i].handle.data));
        np_ids.emplace_back(data, len);
    }

    const s32 dispatch_err = NpHandler::GetInstance().GetRankingByNpId(
        req->user_id, ServiceLabelForRequest(req), board_id, np_ids, std::vector<s32>{}, rank_array,
        comment_array, info_array, last_sort_date, total_record, req);
    if (dispatch_err != ORBIS_OK) {
        req->SetResult(dispatch_err);
        return dispatch_err;
    }
    if (is_async) {
        return ORBIS_OK;
    }
    return req->Wait();
}

s32 sceNpScoreGetRankingByNpId(s32 req_id, OrbisNpScoreBoardId board_id,
                               const OrbisNpId* np_id_array, u64 np_id_array_size,
                               OrbisNpScorePlayerRankData* rank_array, u64 rank_array_size,
                               OrbisNpScoreComment* comment_array, u64 comment_array_size,
                               OrbisNpScoreGameInfo* info_array, u64 info_array_size, u64 array_num,
                               Rtc::OrbisRtcTick* last_sort_date,
                               OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_INFO(Lib_NpScore,
             "called, req_id={}, board_id={}, np_id_array={}, np_id_array_size={}, "
             "rank_array={}, rank_array_size={}, comment_array={}, comment_array_size={}, "
             "info_array={}, info_array_size={}, array_num={}, last_sort_date={}, total_record={}, "
             "option={}",
             req_id, board_id, PTR(np_id_array), np_id_array_size, PTR(rank_array), rank_array_size,
             PTR(comment_array), comment_array_size, PTR(info_array), info_array_size, array_num,
             PTR(last_sort_date), PTR(total_record), PTR(option));
    return GetRankingByNpIdImpl(req_id, board_id, np_id_array, np_id_array_size, rank_array,
                                rank_array_size, comment_array, comment_array_size, info_array,
                                info_array_size, array_num, last_sort_date, total_record, option,
                                false);
}

s32 sceNpScoreGetRankingByNpIdAsync(s32 req_id, OrbisNpScoreBoardId board_id,
                                    const OrbisNpId* np_id_array, u64 np_id_array_size,
                                    OrbisNpScorePlayerRankData* rank_array, u64 rank_array_size,
                                    OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                    OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                    u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                    OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_INFO(Lib_NpScore, "called, req_id={}, board_id={}, array_num={}", req_id, board_id,
             array_num);
    return GetRankingByNpIdImpl(req_id, board_id, np_id_array, np_id_array_size, rank_array,
                                rank_array_size, comment_array, comment_array_size, info_array,
                                info_array_size, array_num, last_sort_date, total_record, option,
                                true);
}

//***********************************
// Ranking by AccountId functions
//***********************************
static s32 GetRankingByAccountIdImpl(
    s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpAccountId* account_id_array,
    u64 account_id_array_size, OrbisNpScorePlayerRankDataA* rank_array, u64 rank_array_size,
    OrbisNpScoreComment* comment_array, u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
    u64 info_array_size, u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
    OrbisNpScoreRankNumber* total_record, void* option, bool is_async) {
    if (option != nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_num > ORBIS_NP_SCORE_MAX_ID_NUM_PER_REQUEST) {
        LOG_ERROR(Lib_NpScore, "too many accountIds {}", array_num);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (array_num == 0 || rank_array == nullptr || account_id_array == nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (account_id_array_size != array_num * sizeof(OrbisNpAccountId)) {
        LOG_ERROR(Lib_NpScore, "invalid size {}", account_id_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    if (rank_array_size != array_num * sizeof(OrbisNpScorePlayerRankDataA)) {
        LOG_ERROR(Lib_NpScore, "invalid size {}", rank_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    std::memset(rank_array, 0, rank_array_size);
    if (comment_array != nullptr) {
        if (comment_array_size != array_num * sizeof(OrbisNpScoreComment)) {
            LOG_ERROR(Lib_NpScore, "invalid size {}", comment_array_size);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        std::memset(comment_array, 0, comment_array_size);
    }
    if (info_array != nullptr) {
        if (info_array_size != array_num * sizeof(OrbisNpScoreGameInfo)) {
            LOG_ERROR(Lib_NpScore, "invalid size {}", info_array_size);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        std::memset(info_array, 0, info_array_size);
    }

    std::shared_ptr<ScoreRequestCtx> req;
    {
        std::lock_guard lock(g_mutex);
        req = LookupRequestUnlocked(req_id);
        if (!req)
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        if (!req->BeginOp())
            return ORBIS_NP_COMMUNITY_ERROR_ABORTED;
    }

    std::vector<u64> account_ids;
    account_ids.reserve(array_num);
    for (u64 i = 0; i < array_num; ++i) {
        account_ids.push_back(static_cast<u64>(account_id_array[i]));
    }

    const s32 dispatch_err = NpHandler::GetInstance().GetRankingByAccountId(
        req->user_id, ServiceLabelForRequest(req), board_id, account_ids, {}, rank_array,
        comment_array, info_array, last_sort_date, total_record, req);
    if (dispatch_err != ORBIS_OK) {
        req->SetResult(dispatch_err);
        return dispatch_err;
    }
    if (is_async)
        return ORBIS_OK;
    return req->Wait();
}

s32 sceNpScoreGetRankingByAccountId(s32 req_id, OrbisNpScoreBoardId board_id,
                                    const OrbisNpAccountId* account_id_array,
                                    u64 account_id_array_size,
                                    OrbisNpScorePlayerRankDataA* rank_array, u64 rank_array_size,
                                    OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                    OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                    u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                    OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_INFO(Lib_NpScore,
             "called, req_id={}, board_id={}, account_id_array={}, "
             "account_id_array_size={}, rank_array={}, rank_array_size={}, comment_array={}, "
             "comment_array_size={}, info_array={}, info_array_size={}, array_num={}, "
             "last_sort_date={}, "
             "total_record={}, option={}",
             req_id, board_id, PTR(account_id_array), account_id_array_size, PTR(rank_array),
             rank_array_size, PTR(comment_array), comment_array_size, PTR(info_array),
             info_array_size, array_num, PTR(last_sort_date), PTR(total_record), PTR(option));
    return GetRankingByAccountIdImpl(req_id, board_id, account_id_array, account_id_array_size,
                                     rank_array, rank_array_size, comment_array, comment_array_size,
                                     info_array, info_array_size, array_num, last_sort_date,
                                     total_record, option, false);
}

s32 sceNpScoreGetRankingByAccountIdAsync(
    s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpAccountId* account_id_array,
    u64 account_id_array_size, OrbisNpScorePlayerRankDataA* rank_array, u64 rank_array_size,
    OrbisNpScoreComment* comment_array, u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
    u64 info_array_size, u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
    OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_INFO(
        Lib_NpScore,
        "called, req_id={}, board_id={}, "
        "account_id_array={}, account_id_array_size={}, rank_array={}, rank_array_size={}, "
        "comment_array={}, comment_array_size={}, info_array={}, info_array_size={}, array_num={}, "
        "last_sort_date={}, total_record={}, option={}",
        req_id, board_id, PTR(account_id_array), account_id_array_size, PTR(rank_array),
        rank_array_size, PTR(comment_array), comment_array_size, PTR(info_array), info_array_size,
        array_num, PTR(last_sort_date), PTR(total_record), PTR(option));
    LOG_INFO(
        Lib_NpScore,
        "req_id={} board_id={} account_id_array={} account_id_array_size={} array_num={} option={}",
        req_id, board_id, PTR(account_id_array), account_id_array_size, array_num, PTR(option));
    return GetRankingByAccountIdImpl(req_id, board_id, account_id_array, account_id_array_size,
                                     rank_array, rank_array_size, comment_array, comment_array_size,
                                     info_array, info_array_size, array_num, last_sort_date,
                                     total_record, option, true);
}

static s32 GetFriendsRankingImpl(s32 req_id, OrbisNpScoreBoardId board_id, s32 include_self,
                                 OrbisNpScoreRankData* rank_array,
                                 OrbisNpScoreRankDataA* a_rank_array_in, u64 rank_array_size,
                                 OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                 OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                 u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                 OrbisNpScoreRankNumber* total_record, void* option,
                                 bool is_async) {
    const bool is_a_variant = (a_rank_array_in != nullptr);
    if (is_a_variant == (rank_array != nullptr)) {
        LOG_ERROR(Lib_NpScore, "invalid rank array");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    const u64 expected_size =
        array_num * (is_a_variant ? sizeof(OrbisNpScoreRankDataA) : sizeof(OrbisNpScoreRankData));
    if (rank_array_size != expected_size) {
        LOG_ERROR(Lib_NpScore, "invalid size {}", rank_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    auto* opt = static_cast<OrbisNpScoreGetFriendRankingOptParam*>(option);
    u32 start_serial_rank = 1;
    if (include_self == 0) {
        if (opt != nullptr) {
            LOG_ERROR(Lib_NpScore, "invalid option");
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
        }
    } else if (opt != nullptr) {
        if (opt->size != sizeof(OrbisNpScoreGetFriendRankingOptParam)) {
            LOG_ERROR(Lib_NpScore, "invalid option size {}", opt->size);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
        }
        if (opt->start_serial_rank != nullptr) {
            start_serial_rank = *opt->start_serial_rank;
        }
    }
    if (array_num > ORBIS_NP_SCORE_MAX_RANGE_NUM_PER_REQUEST) {
        LOG_ERROR(Lib_NpScore, "too many entries {}", array_num);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (start_serial_rank == 0) {
        LOG_ERROR(Lib_NpScore, "invalid startSerialRank");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_num == 0) {
        LOG_ERROR(Lib_NpScore, "invalid arrayNum");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }

    if (is_a_variant) {
        std::memset(a_rank_array_in, 0, rank_array_size);
    } else {
        std::memset(rank_array, 0, rank_array_size);
    }
    if (comment_array != nullptr) {
        if (comment_array_size != array_num * sizeof(OrbisNpScoreComment)) {
            LOG_ERROR(Lib_NpScore, "invalid size");
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        std::memset(comment_array, 0, comment_array_size);
    }
    if (info_array != nullptr) {
        if (info_array_size != array_num * sizeof(OrbisNpScoreGameInfo)) {
            LOG_ERROR(Lib_NpScore, "invalid size");
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        std::memset(info_array, 0, info_array_size);
    }

    std::shared_ptr<ScoreRequestCtx> req;
    {
        std::lock_guard lock(g_mutex);
        req = LookupRequestUnlocked(req_id);
        if (!req) {
            LOG_ERROR(Lib_NpScore, "invalid request id {}", req_id);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        if (!req->BeginOp()) {
            return ORBIS_NP_COMMUNITY_ERROR_ABORTED;
        }
    }

    s32 dispatch_err;
    if (is_a_variant) {
        dispatch_err = NpHandler::GetInstance().GetFriendsRankingA(
            req->user_id, ServiceLabelForRequest(req), board_id, include_self != 0,
            start_serial_rank, static_cast<u32>(array_num), a_rank_array_in, comment_array,
            info_array, last_sort_date, total_record, req);
    } else {
        dispatch_err = NpHandler::GetInstance().GetFriendsRanking(
            req->user_id, ServiceLabelForRequest(req), board_id, include_self != 0,
            start_serial_rank, static_cast<u32>(array_num), rank_array, comment_array, info_array,
            last_sort_date, total_record, req);
    }
    if (dispatch_err != ORBIS_OK) {
        req->SetResult(dispatch_err);
        return dispatch_err;
    }
    if (is_async) {
        return ORBIS_OK;
    }
    return req->Wait();
}

s32 sceNpScoreGetFriendsRanking(s32 req_id, OrbisNpScoreBoardId board_id, s32 include_self,
                                OrbisNpScoreRankData* rank_array, u64 rank_array_size,
                                OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_INFO(Lib_NpScore,
             "called, req_id={}, board_id={}, include_self={}, "
             "rank_array={}, rank_array_size={}, comment_array={}, comment_array_size={}, "
             "info_array={}, "
             "info_array_size={}, array_num={}, last_sort_date={}, total_record={}, option={}",
             req_id, board_id, include_self, PTR(rank_array), rank_array_size, PTR(comment_array),
             comment_array_size, PTR(info_array), info_array_size, array_num, PTR(last_sort_date),
             PTR(total_record), PTR(option));
    return GetFriendsRankingImpl(req_id, board_id, include_self, rank_array,
                                 /*aRankArrayIn=*/nullptr, rank_array_size, comment_array,
                                 comment_array_size, info_array, info_array_size, array_num,
                                 last_sort_date, total_record, option, false);
}

s32 sceNpScoreGetFriendsRankingAsync(s32 req_id, OrbisNpScoreBoardId board_id, s32 include_self,
                                     OrbisNpScoreRankData* rank_array, u64 rank_array_size,
                                     OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                     OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                     u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                     OrbisNpScoreRankNumber* total_record,
                                     OrbisNpScoreGetFriendRankingOptParam* option) {
    LOG_INFO(Lib_NpScore,
             "called, req_id={}, board_id={}, include_self={}, "
             "rank_array={}, rank_array_size={}, comment_array={}, comment_array_size={}, "
             "info_array={}, "
             "info_array_size={}, array_num={}, last_sort_date={}, total_record={}, option={}",
             req_id, board_id, include_self, PTR(rank_array), rank_array_size, PTR(comment_array),
             comment_array_size, PTR(info_array), info_array_size, array_num, PTR(last_sort_date),
             PTR(total_record), PTR(option));
    return GetFriendsRankingImpl(req_id, board_id, include_self, rank_array,
                                 /*aRankArrayIn=*/nullptr, rank_array_size, comment_array,
                                 comment_array_size, info_array, info_array_size, array_num,
                                 last_sort_date, total_record, option, true);
}
s32 sceNpScoreGetFriendsRankingA(s32 req_id, OrbisNpScoreBoardId board_id, s32 include_self,
                                 OrbisNpScoreRankDataA* rank_array, u64 rank_array_size,
                                 OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                 OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                 u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                 OrbisNpScoreRankNumber* total_record,
                                 OrbisNpScoreGetFriendRankingOptParam* option) {
    LOG_INFO(Lib_NpScore,
             "called, req_id={}, board_id={}, include_self={}, "
             "rank_array={}, rank_array_size={}, comment_array={}, comment_array_size={}, "
             "info_array={}, "
             "info_array_size={}, array_num={}, last_sort_date={}, total_record={}, option={}",
             req_id, board_id, include_self, PTR(rank_array), rank_array_size, PTR(comment_array),
             comment_array_size, PTR(info_array), info_array_size, array_num, PTR(last_sort_date),
             PTR(total_record), PTR(option));
    return GetFriendsRankingImpl(req_id, board_id, include_self, /*rankArray=*/nullptr, rank_array,
                                 rank_array_size, comment_array, comment_array_size, info_array,
                                 info_array_size, array_num, last_sort_date, total_record, option,
                                 false);
}

s32 sceNpScoreGetFriendsRankingAAsync(s32 req_id, OrbisNpScoreBoardId board_id, s32 include_self,
                                      OrbisNpScoreRankDataA* rank_array, u64 rank_array_size,
                                      OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                      OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                      u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                      OrbisNpScoreRankNumber* total_record,
                                      OrbisNpScoreGetFriendRankingOptParam* option) {
    LOG_INFO(Lib_NpScore,
             "called, req_id={}, board_id={}, include_self={}, "
             "rank_array={}, rank_array_size={}, comment_array={}, comment_array_size={}, "
             "info_array={}, "
             "info_array_size={}, array_num={}, last_sort_date={}, total_record={}, option={}",
             req_id, board_id, include_self, PTR(rank_array), rank_array_size, PTR(comment_array),
             comment_array_size, PTR(info_array), info_array_size, array_num, PTR(last_sort_date),
             PTR(total_record), PTR(option));
    return GetFriendsRankingImpl(req_id, board_id, include_self, /*rankArray=*/nullptr, rank_array,
                                 rank_array_size, comment_array, comment_array_size, info_array,
                                 info_array_size, array_num, last_sort_date, total_record, option,
                                 true);
}

static s32 GetRankingByNpIdPcIdImpl(s32 req_id, OrbisNpScoreBoardId board_id,
                                    const OrbisNpScoreNpIdPcId* id_array, u64 id_array_size,
                                    OrbisNpScorePlayerRankData* rank_array, u64 rank_array_size,
                                    OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                    OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                    u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                    OrbisNpScoreRankNumber* total_record, void* option,
                                    bool is_async) {

    if (option != nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid option");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    if (id_array == nullptr || array_num == 0 || rank_array == nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (array_num > ORBIS_NP_SCORE_MAX_NPID_NUM_PER_REQUEST) {
        LOG_ERROR(Lib_NpScore, "too many npIds {}", array_num);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (id_array_size != array_num * sizeof(OrbisNpScoreNpIdPcId) ||
        rank_array_size != array_num * sizeof(OrbisNpScorePlayerRankData)) {
        LOG_ERROR(Lib_NpScore, "invalid size {}", rank_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    std::memset(rank_array, 0, rank_array_size);
    if (comment_array != nullptr) {
        if (comment_array_size != array_num * sizeof(OrbisNpScoreComment)) {
            LOG_ERROR(Lib_NpScore, "invalid size {}", comment_array_size);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        std::memset(comment_array, 0, comment_array_size);
    }
    if (info_array != nullptr) {
        if (info_array_size != array_num * sizeof(OrbisNpScoreGameInfo)) {
            LOG_ERROR(Lib_NpScore, "invalid size {}", info_array_size);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        std::memset(info_array, 0, info_array_size);
    }

    std::shared_ptr<ScoreRequestCtx> req;
    {
        std::lock_guard lock(g_mutex);
        req = LookupRequestUnlocked(req_id);
        if (!req) {
            LOG_ERROR(Lib_NpScore, "invalid request id {}", req_id);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        if (!req->BeginOp()) {
            return ORBIS_NP_COMMUNITY_ERROR_ABORTED;
        }
    }

    std::vector<std::string> np_ids;
    std::vector<s32> pc_ids;
    np_ids.reserve(array_num);
    pc_ids.reserve(array_num);
    for (u64 i = 0; i < array_num; ++i) {
        const char* data = id_array[i].np_id.handle.data;
        const u64 len = strnlen(data, sizeof(id_array[i].np_id.handle.data));
        np_ids.emplace_back(data, len);
        pc_ids.push_back(id_array[i].pc_id);
    }

    const s32 dispatch_err = NpHandler::GetInstance().GetRankingByNpId(
        req->user_id, ServiceLabelForRequest(req), board_id, np_ids, pc_ids, rank_array,
        comment_array, info_array, last_sort_date, total_record, req);
    if (dispatch_err != ORBIS_OK) {
        req->SetResult(dispatch_err);
        return dispatch_err;
    }
    if (is_async) {
        return ORBIS_OK;
    }
    return req->Wait();
}

s32 sceNpScoreGetRankingByNpIdPcId(s32 req_id, OrbisNpScoreBoardId board_id,
                                   const OrbisNpScoreNpIdPcId* id_array, u64 id_array_size,
                                   OrbisNpScorePlayerRankData* rank_array, u64 rank_array_size,
                                   OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                   OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                   u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                   OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_ERROR(Lib_NpScore,
              "called, req_id={}, board_id={}, id_array={}, id_array_size={}, "
              "rank_array={}, rank_array_size={}, array_num={}",
              req_id, board_id, PTR(id_array), id_array_size, PTR(rank_array), rank_array_size,
              array_num);
    return GetRankingByNpIdPcIdImpl(req_id, board_id, id_array, id_array_size, rank_array,
                                    rank_array_size, comment_array, comment_array_size, info_array,
                                    info_array_size, array_num, last_sort_date, total_record,
                                    option, false);
}

s32 sceNpScoreGetRankingByNpIdPcIdAsync(s32 req_id, OrbisNpScoreBoardId board_id,
                                        const OrbisNpScoreNpIdPcId* id_array, u64 id_array_size,
                                        OrbisNpScorePlayerRankData* rank_array, u64 rank_array_size,
                                        OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                        OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                        u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                        OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_ERROR(Lib_NpScore, "called, req_id={}, board_id={}, array_num={}", req_id, board_id,
              array_num);
    return GetRankingByNpIdPcIdImpl(req_id, board_id, id_array, id_array_size, rank_array,
                                    rank_array_size, comment_array, comment_array_size, info_array,
                                    info_array_size, array_num, last_sort_date, total_record,
                                    option, true);
}

static s32 GetRankingByAccountIdPcIdImpl(
    s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpScoreAccountIdPcId* id_array,
    u64 id_array_size, OrbisNpScorePlayerRankDataA* rank_array, u64 rank_array_size,
    OrbisNpScoreComment* comment_array, u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
    u64 info_array_size, u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
    OrbisNpScoreRankNumber* total_record, void* option, bool is_async) {
    if (option != nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid option");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_num > ORBIS_NP_SCORE_MAX_ID_NUM_PER_REQUEST) {
        LOG_ERROR(Lib_NpScore, "too many accountIds {}", array_num);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (array_num == 0 || rank_array == nullptr || id_array == nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (id_array_size != array_num * sizeof(OrbisNpScoreAccountIdPcId)) {
        LOG_ERROR(Lib_NpScore, "invalid size {}", id_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    if (rank_array_size != array_num * sizeof(OrbisNpScorePlayerRankDataA)) {
        LOG_ERROR(Lib_NpScore, "invalid size {}", rank_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    std::memset(rank_array, 0, rank_array_size);
    if (comment_array != nullptr) {
        if (comment_array_size != array_num * sizeof(OrbisNpScoreComment)) {
            LOG_ERROR(Lib_NpScore, "invalid size {}", comment_array_size);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        std::memset(comment_array, 0, comment_array_size);
    }
    if (info_array != nullptr) {
        if (info_array_size != array_num * sizeof(OrbisNpScoreGameInfo)) {
            LOG_ERROR(Lib_NpScore, "invalid size {}", info_array_size);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        std::memset(info_array, 0, info_array_size);
    }

    std::shared_ptr<ScoreRequestCtx> req;
    {
        std::lock_guard lock(g_mutex);
        req = LookupRequestUnlocked(req_id);
        if (!req)
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        if (!req->BeginOp())
            return ORBIS_NP_COMMUNITY_ERROR_ABORTED;
    }

    std::vector<u64> account_ids;
    std::vector<s32> pc_ids;
    account_ids.reserve(array_num);
    pc_ids.reserve(array_num);
    for (u64 i = 0; i < array_num; ++i) {
        account_ids.push_back(static_cast<u64>(id_array[i].account_id));
        pc_ids.push_back(id_array[i].pc_id);
    }

    const s32 dispatch_err = NpHandler::GetInstance().GetRankingByAccountId(
        req->user_id, ServiceLabelForRequest(req), board_id, account_ids, pc_ids, rank_array,
        comment_array, info_array, last_sort_date, total_record, req);
    if (dispatch_err != ORBIS_OK) {
        req->SetResult(dispatch_err);
        return dispatch_err;
    }
    if (is_async)
        return ORBIS_OK;
    return req->Wait();
}

s32 sceNpScoreGetRankingByAccountIdPcId(s32 req_id, OrbisNpScoreBoardId board_id,
                                        const OrbisNpScoreAccountIdPcId* id_array,
                                        u64 id_array_size, OrbisNpScorePlayerRankDataA* rank_array,
                                        u64 rank_array_size, OrbisNpScoreComment* comment_array,
                                        u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
                                        u64 info_array_size, u64 array_num,
                                        Rtc::OrbisRtcTick* last_sort_date,
                                        OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_INFO(Lib_NpScore,
             "called, req_id={}, board_id={}, id_array={}, "
             "id_array_size={}, rank_array={}, rank_array_size={}, comment_array={}, "
             "comment_array_size={}, "
             "info_array={}, info_array_size={}, array_num={}, last_sort_date={}, total_record={}, "
             "option={}",
             req_id, board_id, PTR(id_array), id_array_size, PTR(rank_array), rank_array_size,
             PTR(comment_array), comment_array_size, PTR(info_array), info_array_size, array_num,
             PTR(last_sort_date), PTR(total_record), PTR(option));
    return GetRankingByAccountIdPcIdImpl(req_id, board_id, id_array, id_array_size, rank_array,
                                         rank_array_size, comment_array, comment_array_size,
                                         info_array, info_array_size, array_num, last_sort_date,
                                         total_record, option, false);
}

s32 sceNpScoreGetRankingByAccountIdPcIdAsync(
    s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpScoreAccountIdPcId* id_array,
    u64 id_array_size, OrbisNpScorePlayerRankDataA* rank_array, u64 rank_array_size,
    OrbisNpScoreComment* comment_array, u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
    u64 info_array_size, u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
    OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_INFO(Lib_NpScore,
             "called, req_id={}, board_id={}, id_array={}, "
             "id_array_size={}, rank_array={}, rank_array_size={}, comment_array={}, "
             "comment_array_size={}, "
             "info_array={}, info_array_size={}, array_num={}, last_sort_date={}, total_record={}, "
             "option={}",
             req_id, board_id, PTR(id_array), id_array_size, PTR(rank_array), rank_array_size,
             PTR(comment_array), comment_array_size, PTR(info_array), info_array_size, array_num,
             PTR(last_sort_date), PTR(total_record), PTR(option));
    return GetRankingByAccountIdPcIdImpl(req_id, board_id, id_array, id_array_size, rank_array,
                                         rank_array_size, comment_array, comment_array_size,
                                         info_array, info_array_size, array_num, last_sort_date,
                                         total_record, option, true);
}

static s32 GetBoardInfoImpl(s32 req_id, OrbisNpScoreBoardId board_id,
                            OrbisNpScoreBoardInfo* board_info, void* option, bool is_async) {
    if (option != nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid option");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (g_firmware_version >= 0 && g_firmware_version >= Common::ElfInfo::FW_250 &&
        board_info == nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }

    std::shared_ptr<ScoreRequestCtx> req;
    {
        std::lock_guard lock(g_mutex);
        req = LookupRequestUnlocked(req_id);
        if (!req) {
            LOG_ERROR(Lib_NpScore, "invalid request id {}", req_id);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        if (!req->BeginOp()) {
            return ORBIS_NP_COMMUNITY_ERROR_ABORTED;
        }
    }

    if (board_info == nullptr) {
        LOG_INFO(Lib_NpScore, "legacy SDK with null boardInfo - no-op success");
        req->SetResult(ORBIS_OK);
        return ORBIS_OK;
    }

    std::memset(board_info, 0, sizeof(OrbisNpScoreBoardInfo));

    const s32 dispatch_err = NpHandler::GetInstance().GetBoardInfo(
        req->user_id, ServiceLabelForRequest(req), board_id, board_info, req);
    if (dispatch_err != ORBIS_OK) {
        req->SetResult(dispatch_err);
        return dispatch_err;
    }
    if (is_async) {
        return ORBIS_OK;
    }
    return req->Wait();
}

s32 sceNpScoreGetBoardInfo(s32 req_id, OrbisNpScoreBoardId board_id,
                           OrbisNpScoreBoardInfo* board_info, void* option) {
    LOG_INFO(Lib_NpScore, "called, req_id={}, board_id={}, board_info={}, option={}", req_id,
             board_id, PTR(board_info), PTR(option));
    return GetBoardInfoImpl(req_id, board_id, board_info, option, false);
}

s32 sceNpScoreGetBoardInfoAsync(s32 req_id, OrbisNpScoreBoardId board_id,
                                OrbisNpScoreBoardInfo* board_info, void* option) {
    LOG_INFO(Lib_NpScore, "called, req_id={}, board_id={}, board_info={}, option={}", req_id,
             board_id, PTR(board_info), PTR(option));
    return GetBoardInfoImpl(req_id, board_id, board_info, option, true);
}

static bool ServeGameDataFromCache(const std::shared_ptr<ScoreRequestCtx>& req,
                                   const std::string& key, u64* total_size, u64 recv_size,
                                   void* data, bool is_async, s32* ret) {
    if (req->game_data_recv_offset == 0 || req->game_data_recv_key != key) {
        req->game_data_recv_offset = 0;
        req->game_data_recv_cache.clear();
        req->game_data_recv_key = key;
        return false;
    }
    const auto& cache = req->game_data_recv_cache;
    const u64 start = std::min<u64>(req->game_data_recv_offset, cache.size());
    const u64 n = (data != nullptr) ? std::min<u64>(cache.size() - start, recv_size) : 0;
    if (n > 0) {
        std::memcpy(data, cache.data() + start, n);
    }
    if (total_size != nullptr) {
        *total_size = cache.size();
    }
    if (n < recv_size) {
        req->game_data_recv_offset = 0;
        req->game_data_recv_cache.clear();
    } else {
        req->game_data_recv_offset = start + n;
    }
    LOG_INFO(Lib_NpScore, "served {} cached bytes from offset {} (total {})", n, start,
             cache.size());
    req->SetResult(static_cast<s32>(n));
    *ret = is_async ? ORBIS_OK : static_cast<s32>(n);
    return true;
}

static s32 GetGameDataImpl(s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpId* np_id,
                           u64* total_size, u64 recv_size, void* data, void* option,
                           bool is_async) {
    if (np_id == nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option != nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid option");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::shared_ptr<ScoreRequestCtx> req;
    {
        std::lock_guard lock(g_mutex);
        req = LookupRequestUnlocked(req_id);
        if (!req) {
            LOG_ERROR(Lib_NpScore, "invalid request id {}", req_id);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        if (!req->BeginOp()) {
            return ORBIS_NP_COMMUNITY_ERROR_ABORTED;
        }
    }

    const std::string np_id_str(np_id->handle.data,
                                strnlen(np_id->handle.data, sizeof(np_id->handle.data)));
    if (np_id_str.empty()) {
        LOG_ERROR(Lib_NpScore, "invalid npId");
        req->SetResult(ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    const s32 pc_id = req->pc_id;

    s32 cached_ret;
    if (ServeGameDataFromCache(req, std::to_string(board_id) + ":npid:" + np_id_str, total_size,
                               recv_size, data, is_async, &cached_ret)) {
        return cached_ret;
    }

    const s32 dispatch_err =
        NpHandler::GetInstance().GetGameData(req->user_id, ServiceLabelForRequest(req), board_id,
                                             np_id_str, pc_id, data, recv_size, total_size, req);
    if (dispatch_err != ORBIS_OK) {
        req->SetResult(dispatch_err);
        return dispatch_err;
    }
    if (is_async) {
        return ORBIS_OK;
    }
    return req->Wait();
}

static s32 GetGameDataByAccountIdImpl(s32 req_id, OrbisNpScoreBoardId board_id,
                                      OrbisNpAccountId account_id, u64* total_size, u64 recv_size,
                                      void* data, void* option, bool is_async) {
    if (option != nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid option");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (account_id == 0) {
        LOG_ERROR(Lib_NpScore, "invalid accountId");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::shared_ptr<ScoreRequestCtx> req;
    {
        std::lock_guard lock(g_mutex);
        req = LookupRequestUnlocked(req_id);
        if (!req) {
            LOG_ERROR(Lib_NpScore, "invalid request id {}", req_id);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        if (!req->BeginOp()) {
            return ORBIS_NP_COMMUNITY_ERROR_ABORTED;
        }
    }

    const s32 pc_id = req->pc_id;

    s32 cached_ret;
    if (ServeGameDataFromCache(req, std::to_string(board_id) + ":acc:" + std::to_string(account_id),
                               total_size, recv_size, data, is_async, &cached_ret)) {
        return cached_ret;
    }

    const s32 dispatch_err = NpHandler::GetInstance().GetGameDataByAccountId(
        req->user_id, ServiceLabelForRequest(req), board_id, account_id, pc_id, data, recv_size,
        total_size, req);
    if (dispatch_err != ORBIS_OK) {
        req->SetResult(dispatch_err);
        return dispatch_err;
    }
    if (is_async) {
        return ORBIS_OK;
    }
    return req->Wait();
}

s32 sceNpScoreGetGameData(s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpId* np_id,
                          u64* total_size, u64 recv_size, void* data, void* option) {
    LOG_INFO(
        Lib_NpScore,
        "called, req_id={}, board_id={}, np_id={}, total_size={}, recv_size={}, data={}, option={}",
        req_id, board_id, PTR(np_id), PTR(total_size), recv_size, PTR(data), PTR(option));
    return GetGameDataImpl(req_id, board_id, np_id, total_size, recv_size, data, option, false);
}

s32 sceNpScoreGetGameDataAsync(s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpId* np_id,
                               u64* total_size, u64 recv_size, void* data, void* option) {
    LOG_INFO(
        Lib_NpScore,
        "called, req_id={}, board_id={}, np_id={}, total_size={}, recv_size={}, data={}, option={}",
        req_id, board_id, PTR(np_id), PTR(total_size), recv_size, PTR(data), PTR(option));
    return GetGameDataImpl(req_id, board_id, np_id, total_size, recv_size, data, option, true);
}

s32 sceNpScoreGetGameDataByAccountId(s32 req_id, OrbisNpScoreBoardId board_id,
                                     OrbisNpAccountId account_id, u64* total_size, u64 recv_size,
                                     void* data, void* option) {
    LOG_INFO(Lib_NpScore,
             "called, req_id={}, board_id={}, account_id={}, total_size={}, recv_size={}, data={}, "
             "option={}",
             req_id, board_id, account_id, PTR(total_size), recv_size, PTR(data), PTR(option));
    return GetGameDataByAccountIdImpl(req_id, board_id, account_id, total_size, recv_size, data,
                                      option, false);
}

s32 sceNpScoreGetGameDataByAccountIdAsync(s32 req_id, OrbisNpScoreBoardId board_id,
                                          OrbisNpAccountId account_id, u64* total_size,
                                          u64 recv_size, void* data, void* option) {
    LOG_INFO(Lib_NpScore,
             "called, req_id={}, board_id={}, account_id={}, total_size={}, recv_size={}, data={}, "
             "option={}",
             req_id, board_id, account_id, PTR(total_size), recv_size, PTR(data), PTR(option));
    return GetGameDataByAccountIdImpl(req_id, board_id, account_id, total_size, recv_size, data,
                                      option, true);
}

static s32 RecordGameDataImpl(s32 req_id, OrbisNpScoreBoardId board_id, OrbisNpScoreValue score,
                              u64 total_size, u64 send_size, const void* data, void* option,
                              bool is_async) {
    if (option != nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid option");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (total_size == 0) {
        LOG_ERROR(Lib_NpScore, "invalid totalSize");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (data == nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (send_size == 0) {
        LOG_ERROR(Lib_NpScore, "invalid sendSize");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::shared_ptr<ScoreRequestCtx> req;
    {
        std::lock_guard lock(g_mutex);
        req = LookupRequestUnlocked(req_id);
        if (!req) {
            LOG_ERROR(Lib_NpScore, "invalid request id {}", req_id);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        if (!req->BeginOp()) {
            return ORBIS_NP_COMMUNITY_ERROR_ABORTED;
        }
    }

    const s32 pc_id = req->pc_id;
    auto& buf = req->game_data_send_buf;
    if (buf.empty()) {
        req->game_data_send_total = total_size;
    }
    const u64 remaining = req->game_data_send_total - buf.size();
    const u64 take = std::min<u64>(send_size, remaining);
    const auto* bytes = static_cast<const u8*>(data);
    buf.insert(buf.end(), bytes, bytes + take);
    if (buf.size() < req->game_data_send_total) {
        LOG_INFO(Lib_NpScore, "buffered {}/{} bytes", buf.size(), req->game_data_send_total);
        req->SetResult(ORBIS_OK);
        return ORBIS_OK;
    }
    std::vector<u8> blob = std::move(buf);
    buf.clear();
    req->game_data_send_total = 0;

    const s32 dispatch_err =
        NpHandler::GetInstance().RecordGameData(req->user_id, ServiceLabelForRequest(req), board_id,
                                                pc_id, score, blob.data(), blob.size(), req);
    if (dispatch_err != ORBIS_OK) {
        req->SetResult(dispatch_err);
        return dispatch_err;
    }
    if (is_async) {
        return ORBIS_OK;
    }
    return req->Wait();
}

s32 sceNpScoreRecordGameData(s32 req_id, OrbisNpScoreBoardId board_id, OrbisNpScoreValue score,
                             u64 total_size, u64 send_size, const void* data, void* option) {
    LOG_INFO(
        Lib_NpScore,
        "called, req_id={}, board_id={}, score={}, total_size={}, send_size={}, data={}, option={}",
        req_id, board_id, score, total_size, send_size, PTR(data), PTR(option));
    return RecordGameDataImpl(req_id, board_id, score, total_size, send_size, data, option, false);
}

s32 sceNpScoreRecordGameDataAsync(s32 req_id, OrbisNpScoreBoardId board_id, OrbisNpScoreValue score,
                                  u64 total_size, u64 send_size, const void* data, void* option) {
    LOG_INFO(
        Lib_NpScore,
        "called, req_id={}, board_id={}, score={}, total_size={}, send_size={}, data={}, option={}",
        req_id, board_id, score, total_size, send_size, PTR(data), PTR(option));
    return RecordGameDataImpl(req_id, board_id, score, total_size, send_size, data, option, true);
}

static s32 CensorCommentImpl(s32 req_id, const char* comment, void* option, bool is_async) {
    if (option != nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid option");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (comment == nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }

    std::shared_ptr<ScoreRequestCtx> req;
    {
        std::lock_guard lock(g_mutex);
        req = LookupRequestUnlocked(req_id);
        if (!req) {
            LOG_ERROR(Lib_NpScore, "invalid request id {}", req_id);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        if (!req->BeginOp()) {
            return ORBIS_NP_COMMUNITY_ERROR_ABORTED;
        }
    }

    constexpr u64 probe = ORBIS_NP_SCORE_CENSOR_COMMENT_MAXLEN + 1;
    const u64 len = strnlen(comment, probe);
    if (len > ORBIS_NP_SCORE_CENSOR_COMMENT_MAXLEN) {
        LOG_ERROR(Lib_NpScore, "invalid comment length");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    LOG_INFO(Lib_NpScore, "req_id={} len={}", req_id, len);
    req->SetResult(ORBIS_OK);
    if (is_async) {
        return ORBIS_OK;
    }
    return req->Wait();
}

s32 sceNpScoreCensorComment(s32 req_id, const char* comment, void* option) {
    LOG_INFO(Lib_NpScore, "called, req_id={}, comment={}, option={}", req_id,
             comment ? comment : "null", PTR(option));
    return CensorCommentImpl(req_id, comment, option, false);
}

s32 sceNpScoreCensorCommentAsync(s32 req_id, const char* comment, void* option) {
    LOG_INFO(Lib_NpScore, "called, req_id={}, comment={}, option={}", req_id,
             comment ? comment : "null", PTR(option));
    return CensorCommentImpl(req_id, comment, option, true);
}

static s32 SanitizeCommentImpl(s32 req_id, const char* comment, char* sanitized_comment,
                               void* option, bool is_async) {
    if (option != nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid option");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (comment == nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }

    if (sanitized_comment == nullptr) {
        LOG_ERROR(Lib_NpScore, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }

    std::shared_ptr<ScoreRequestCtx> req;
    {
        std::lock_guard lock(g_mutex);
        req = LookupRequestUnlocked(req_id);
        if (!req) {
            LOG_ERROR(Lib_NpScore, "invalid request id {}", req_id);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        if (!req->BeginOp()) {
            return ORBIS_NP_COMMUNITY_ERROR_ABORTED;
        }
    }

    constexpr u64 probe = ORBIS_NP_SCORE_SANITIZE_COMMENT_MAXLEN + 1;
    const u64 len = strnlen(comment, probe);
    if (len > ORBIS_NP_SCORE_SANITIZE_COMMENT_MAXLEN) {
        LOG_ERROR(Lib_NpScore, "invalid comment length");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::memcpy(sanitized_comment, comment, len);
    sanitized_comment[len] = '\0';
    LOG_INFO(Lib_NpScore, "req_id={} len={} (passthrough)", req_id, len);
    req->SetResult(ORBIS_OK);
    if (is_async) {
        return ORBIS_OK;
    }
    return req->Wait();
}

s32 sceNpScoreSanitizeComment(s32 req_id, const char* comment, char* sanitized_comment,
                              void* option) {
    LOG_INFO(Lib_NpScore, "called, req_id={}, comment={}, sanitized_comment={}, option={}", req_id,
             comment ? comment : "null", PTR(sanitized_comment), PTR(option));
    return SanitizeCommentImpl(req_id, comment, sanitized_comment, option, false);
}

s32 sceNpScoreSanitizeCommentAsync(s32 req_id, const char* comment, char* sanitized_comment,
                                   void* option) {
    LOG_INFO(Lib_NpScore, "called, req_id={}, comment={}, sanitized_comment={}, option={}", req_id,
             comment ? comment : "null", PTR(sanitized_comment), PTR(option));
    return SanitizeCommentImpl(req_id, comment, sanitized_comment, option, true);
}

s32 sceNpScoreSetPlayerCharacterId(s32 ctx_id, OrbisNpScorePcId pc_id) {
    if (pc_id < 0) {
        LOG_ERROR(Lib_NpScore, "invalid pcId {}", pc_id);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    std::lock_guard lock(g_mutex);

    if (auto* tc = LookupTitleCtxUnlocked(ctx_id)) {
        tc->pc_id = pc_id;
        LOG_INFO(Lib_NpScore, "title_ctx_id={} pc_id={}", ctx_id, pc_id);
        return ORBIS_OK;
    }

    if (auto req = LookupRequestUnlocked(ctx_id)) {
        req->pc_id = pc_id;
        LOG_INFO(Lib_NpScore, "req_id={} pc_id={}", ctx_id, pc_id);
        return ORBIS_OK;
    }

    LOG_ERROR(Lib_NpScore, "invalid id {}", ctx_id);
    return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
}

s32 sceNpScoreChangeModeForOtherSaveDataOwners() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreCreateTitleCtx() {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetFriendsRankingForCrossSave(
    s32 req_id, OrbisNpScoreBoardId board_id, s32 include_self,
    OrbisNpScoreRankDataForCrossSave* rank_array, u64 rank_array_size,
    OrbisNpScoreComment* comment_array, u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
    u64 info_array_size, u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
    OrbisNpScoreRankNumber* total_record, OrbisNpScoreGetFriendRankingOptParam* option) {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetFriendsRankingForCrossSaveAsync(
    s32 req_id, OrbisNpScoreBoardId board_id, s32 include_self,
    OrbisNpScoreRankDataForCrossSave* rank_array, u64 rank_array_size,
    OrbisNpScoreComment* comment_array, u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
    u64 info_array_size, u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
    OrbisNpScoreRankNumber* total_record, OrbisNpScoreGetFriendRankingOptParam* option) {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByAccountIdForCrossSave(
    s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpAccountId* account_id_array,
    u64 account_id_array_size, OrbisNpScorePlayerRankDataForCrossSave* rank_array,
    u64 rank_array_size, OrbisNpScoreComment* comment_array, u64 comment_array_size,
    OrbisNpScoreGameInfo* info_array, u64 info_array_size, u64 array_num,
    Rtc::OrbisRtcTick* last_sort_date, OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByAccountIdForCrossSaveAsync(
    s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpAccountId* account_id_array,
    u64 account_id_array_size, OrbisNpScorePlayerRankDataForCrossSave* rank_array,
    u64 rank_array_size, OrbisNpScoreComment* comment_array, u64 comment_array_size,
    OrbisNpScoreGameInfo* info_array, u64 info_array_size, u64 array_num,
    Rtc::OrbisRtcTick* last_sort_date, OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByAccountIdPcIdForCrossSave(
    s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpScoreAccountIdPcId* id_array,
    u64 id_array_size, OrbisNpScorePlayerRankDataForCrossSave* rank_array, u64 rank_array_size,
    OrbisNpScoreComment* comment_array, u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
    u64 info_array_size, u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
    OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByAccountIdPcIdForCrossSaveAsync(
    s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpScoreAccountIdPcId* id_array,
    u64 id_array_size, OrbisNpScorePlayerRankDataForCrossSave* rank_array, u64 rank_array_size,
    OrbisNpScoreComment* comment_array, u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
    u64 info_array_size, u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
    OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByRangeForCrossSave(s32 req_id, OrbisNpScoreBoardId board_id,
                                            OrbisNpScoreRankNumber start_serial_rank,
                                            OrbisNpScoreRankDataForCrossSave* rank_array,
                                            u64 rank_array_size, OrbisNpScoreComment* comment_array,
                                            u64 comment_array_size,
                                            OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                            u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                            OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreGetRankingByRangeForCrossSaveAsync(
    s32 req_id, OrbisNpScoreBoardId board_id, OrbisNpScoreRankNumber start_serial_rank,
    OrbisNpScoreRankDataForCrossSave* rank_array, u64 rank_array_size,
    OrbisNpScoreComment* comment_array, u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
    u64 info_array_size, u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
    OrbisNpScoreRankNumber* total_record, void* option) {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreSetThreadParam(s32 thread_priority, u64 cpu_affinity_mask) {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpScoreSetTimeout(s32 id, s32 resolve_retry, s32 resolve_timeout, s32 conn_timeout,
                         s32 send_timeout, s32 recv_timeout) {
    LOG_ERROR(Lib_NpScore, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterHooks() {
    sceKernelGetCompiledSdkVersion(&g_firmware_version);
    return RegisterNpScoreHooks();
}

} // namespace Libraries::Np::NpScore
