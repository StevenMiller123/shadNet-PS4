// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/rtc/rtc.h"
#include "core/libraries/user_service/user_service.h"

namespace Libraries::Np::NpScore {

using OrbisNpScoreTitleCtxId = s32;
using OrbisNpScoreRequestId = s32;
using OrbisNpScoreBoardId = u32;
using OrbisNpScorePcId = s32;
using OrbisNpScoreRankNumber = u32;
using OrbisNpScoreValue = s64;

constexpr s32 ORBIS_NP_SCORE_COMMENT_MAXLEN = 63;
constexpr s32 ORBIS_NP_SCORE_GAMEINFO_MAXSIZE = 189;
constexpr s32 ORBIS_NP_SCORE_CENSOR_COMMENT_MAXLEN = 255;
constexpr s32 ORBIS_NP_SCORE_SANITIZE_COMMENT_MAXLEN = 255;

constexpr s32 ORBIS_NP_SCORE_NORMAL_UPDATE = 0;
constexpr s32 ORBIS_NP_SCORE_FORCE_UPDATE = 1;
constexpr s32 ORBIS_NP_SCORE_DESCENDING_ORDER = 0;
constexpr s32 ORBIS_NP_SCORE_ASCENDING_ORDER = 1;

constexpr s32 ORBIS_NP_SCORE_MAX_RANGE_NUM_PER_REQUEST = 100;
constexpr s32 ORBIS_NP_SCORE_MAX_ID_NUM_PER_REQUEST = 101;
constexpr s32 ORBIS_NP_SCORE_MAX_SELECTED_FRIENDS_NUM = 100;

constexpr s32 ORBIS_NP_SCORE_MAX_RANGE_NUM_PER_TRANS = 100;
constexpr s32 ORBIS_NP_SCORE_MAX_NPID_NUM_PER_TRANS = 101;
constexpr s32 ORBIS_NP_SCORE_MAX_NPID_NUM_PER_REQUEST = 101;

constexpr s32 ORBIS_NP_SCORE_MAX_CTX_NUM = 32;

constexpr s32 ORBIS_NP_SCORE_BINDMODE_ALL_FORBIDDEN = 0x0000;
constexpr s32 ORBIS_NP_SCORE_BINDMODE_RDONLY = 0x0001;
constexpr s32 ORBIS_NP_SCORE_BINDMODE_WRONLY = 0x0002;
constexpr s32 ORBIS_NP_SCORE_BINDMODE_RDWR =
    ORBIS_NP_SCORE_BINDMODE_RDONLY | ORBIS_NP_SCORE_BINDMODE_WRONLY;
constexpr s32 ORBIS_NP_SCORE_BINDMODE_DEFAULT = ORBIS_NP_SCORE_BINDMODE_RDWR;

struct OrbisNpScoreBoardInfo {
    u32 rank_limit;
    u32 update_mode;
    u32 sort_mode;
    OrbisNpScoreRankNumber upload_num_limit;
    u64 upload_size_limit;
};
static_assert(sizeof(OrbisNpScoreBoardInfo) == 24, "OrbisNpScoreBoardInfo must be 24 bytes");

struct OrbisNpScoreComment {
    char utf8_comment[ORBIS_NP_SCORE_COMMENT_MAXLEN + 1];
};
static_assert(sizeof(OrbisNpScoreComment) == 64, "OrbisNpScoreComment must be 64 bytes");

struct OrbisNpScoreGameInfo {
    u64 info_size;
    u8 data[ORBIS_NP_SCORE_GAMEINFO_MAXSIZE];
    u8 pad2[3];
};
static_assert(sizeof(OrbisNpScoreGameInfo) == 200, "OrbisNpScoreGameInfo must be 200 bytes");

struct OrbisNpScoreGetFriendRankingOptParam {
    u64 size;
    u32* start_serial_rank;
    u32* hits;
};

struct OrbisNpScoreAccountIdPcId {
    OrbisNpAccountId account_id;
    OrbisNpScorePcId pc_id;
    u8 pad[4];
};

struct OrbisNpScoreNpIdPcId {
    OrbisNpId np_id;
    OrbisNpScorePcId pc_id;
    u8 pad[4];
};

struct OrbisNpScoreRankData {
    OrbisNpId np_id;
    u8 reserved[49];
    u8 pad0[3];
    OrbisNpScorePcId pc_id;
    OrbisNpScoreRankNumber serial_rank;
    OrbisNpScoreRankNumber rank;
    OrbisNpScoreRankNumber highest_rank;
    s32 has_game_data;
    u8 pad1[4];
    OrbisNpScoreValue score_value;
    Rtc::OrbisRtcTick record_date;
};
static_assert(sizeof(OrbisNpScoreRankData) == 0x80, "OrbisNpScoreRankData must be 128 bytes");

struct OrbisNpScoreRankDataA {
    OrbisNpOnlineId online_id;
    u8 reserved0[16];
    u8 reserved[49];
    u8 pad0[3];
    OrbisNpScorePcId pc_id;
    OrbisNpScoreRankNumber serial_rank;
    OrbisNpScoreRankNumber rank;
    OrbisNpScoreRankNumber highest_rank;
    s32 has_game_data;
    u8 pad1[4];
    OrbisNpScoreValue score_value;
    Rtc::OrbisRtcTick record_date;
    OrbisNpAccountId account_id;
    u8 pad2[8];
};
static_assert(sizeof(OrbisNpScoreRankDataA) == 0x90, "OrbisNpScoreRankDataA must be 144 bytes");

struct OrbisNpScorePlayerRankData {
    s32 has_data;
    u8 pad0[4];
    OrbisNpScoreRankData rank_data;
};
static_assert(sizeof(OrbisNpScorePlayerRankData) == 0x88,
              "OrbisNpScorePlayerRankData must be 136 bytes");

struct OrbisNpScorePlayerRankDataA {
    s32 has_data;
    u8 pad0[4];
    OrbisNpScoreRankDataA rank_data;
};
static_assert(sizeof(OrbisNpScorePlayerRankDataA) == 0x98,
              "OrbisNpScorePlayerRankDataA must be 152 bytes");

struct OrbisNpScoreRankDataForCrossSave {
    OrbisNpId np_id;
    u8 reserved[49];
    u8 pad0[3];
    OrbisNpScorePcId pc_id;
    OrbisNpScoreRankNumber serial_rank;
    OrbisNpScoreRankNumber rank;
    OrbisNpScoreRankNumber highest_rank;
    s32 has_game_data;
    u8 pad1[4];
    OrbisNpScoreValue score_value;
    Rtc::OrbisRtcTick record_date;
    OrbisNpAccountId account_id;
    u8 pad2[8];
};

struct OrbisNpScorePlayerRankDataForCrossSave {
    s32 has_data;
    u8 pad0[4];
    OrbisNpScoreRankDataForCrossSave rank_data;
};

s32 sceNpScoreCreateNpTitleCtx(OrbisNpServiceLabel service_label, const OrbisNpId* self_np_id);
s32 sceNpScoreCreateNpTitleCtxA(OrbisNpServiceLabel np_service_label,
                                UserService::OrbisUserServiceUserId self_id);
s32 sceNpScoreDeleteNpTitleCtx(s32 title_ctx_id);
s32 sceNpScoreCreateRequest(s32 title_ctx_id);
s32 sceNpScoreDeleteRequest(s32 req_id);
s32 sceNpScoreAbortRequest(s32 req_id);
s32 sceNpScorePollAsync(s32 req_id, s32* result);
s32 sceNpScoreWaitAsync(s32 req_id, s32* result);
s32 sceNpScoreRecordScore(s32 req_id, OrbisNpScoreBoardId board_id, OrbisNpScoreValue score,
                          const OrbisNpScoreComment* score_comment,
                          const OrbisNpScoreGameInfo* game_info, OrbisNpScoreRankNumber* tmp_rank,
                          const Rtc::OrbisRtcTick* compare_date, void* option);
s32 sceNpScoreRecordScoreAsync(s32 req_id, OrbisNpScoreBoardId board_id, OrbisNpScoreValue score,
                               const OrbisNpScoreComment* score_comment,
                               const OrbisNpScoreGameInfo* game_info,
                               OrbisNpScoreRankNumber* tmp_rank,
                               const Rtc::OrbisRtcTick* compare_date, void* option);
s32 sceNpScoreGetRankingByRange(s32 req_id, OrbisNpScoreBoardId board_id,
                                OrbisNpScoreRankNumber start_serial_rank,
                                OrbisNpScoreRankData* rank_array, u64 rank_array_size,
                                OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreGetRankingByRangeAsync(s32 req_id, OrbisNpScoreBoardId board_id,
                                     OrbisNpScoreRankNumber start_serial_rank,
                                     OrbisNpScoreRankData* rank_array, u64 rank_array_size,
                                     OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                     OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                     u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                     OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreGetRankingByRangeA(s32 req_id, OrbisNpScoreBoardId board_id,
                                 OrbisNpScoreRankNumber start_serial_rank,
                                 OrbisNpScoreRankDataA* rank_array, u64 rank_array_size,
                                 OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                 OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                 u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                 OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreGetRankingByRangeAAsync(s32 req_id, OrbisNpScoreBoardId board_id,
                                      OrbisNpScoreRankNumber start_serial_rank,
                                      OrbisNpScoreRankDataA* rank_array, u64 rank_array_size,
                                      OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                      OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                      u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                      OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreGetRankingByNpId(s32 req_id, OrbisNpScoreBoardId board_id,
                               const OrbisNpId* np_id_array, u64 np_id_array_size,
                               OrbisNpScorePlayerRankData* rank_array, u64 rank_array_size,
                               OrbisNpScoreComment* comment_array, u64 comment_array_size,
                               OrbisNpScoreGameInfo* info_array, u64 info_array_size, u64 array_num,
                               Rtc::OrbisRtcTick* last_sort_date,
                               OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreGetRankingByNpIdAsync(s32 req_id, OrbisNpScoreBoardId board_id,
                                    const OrbisNpId* np_id_array, u64 np_id_array_size,
                                    OrbisNpScorePlayerRankData* rank_array, u64 rank_array_size,
                                    OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                    OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                    u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                    OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreGetFriendsRanking(s32 req_id, OrbisNpScoreBoardId board_id, s32 include_self,
                                OrbisNpScoreRankData* rank_array, u64 rank_array_size,
                                OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreGetFriendsRankingAsync(s32 req_id, OrbisNpScoreBoardId board_id, s32 include_self,
                                     OrbisNpScoreRankData* rank_array, u64 rank_array_size,
                                     OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                     OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                     u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                     OrbisNpScoreRankNumber* total_record,
                                     OrbisNpScoreGetFriendRankingOptParam* option);
s32 sceNpScoreGetFriendsRankingA(s32 req_id, OrbisNpScoreBoardId board_id, s32 include_self,
                                 OrbisNpScoreRankDataA* rank_array, u64 rank_array_size,
                                 OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                 OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                 u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                 OrbisNpScoreRankNumber* total_record,
                                 OrbisNpScoreGetFriendRankingOptParam* option);
s32 sceNpScoreGetFriendsRankingAAsync(s32 req_id, OrbisNpScoreBoardId board_id, s32 include_self,
                                      OrbisNpScoreRankDataA* rank_array, u64 rank_array_size,
                                      OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                      OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                      u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                      OrbisNpScoreRankNumber* total_record,
                                      OrbisNpScoreGetFriendRankingOptParam* option);
s32 sceNpScoreGetRankingByNpIdPcId(s32 req_id, OrbisNpScoreBoardId board_id,
                                   const OrbisNpScoreNpIdPcId* id_array, u64 id_array_size,
                                   OrbisNpScorePlayerRankData* rank_array, u64 rank_array_size,
                                   OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                   OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                   u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                   OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreGetRankingByNpIdPcIdAsync(s32 req_id, OrbisNpScoreBoardId board_id,
                                        const OrbisNpScoreNpIdPcId* id_array, u64 id_array_size,
                                        OrbisNpScorePlayerRankData* rank_array, u64 rank_array_size,
                                        OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                        OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                        u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                        OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreGetRankingByAccountId(s32 req_id, OrbisNpScoreBoardId board_id,
                                    const OrbisNpAccountId* account_id_array,
                                    u64 account_id_array_size,
                                    OrbisNpScorePlayerRankDataA* rank_array, u64 rank_array_size,
                                    OrbisNpScoreComment* comment_array, u64 comment_array_size,
                                    OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                    u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                    OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreGetRankingByAccountIdAsync(
    s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpAccountId* account_id_array,
    u64 account_id_array_size, OrbisNpScorePlayerRankDataA* rank_array, u64 rank_array_size,
    OrbisNpScoreComment* comment_array, u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
    u64 info_array_size, u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
    OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreGetRankingByAccountIdPcId(s32 req_id, OrbisNpScoreBoardId board_id,
                                        const OrbisNpScoreAccountIdPcId* id_array,
                                        u64 id_array_size, OrbisNpScorePlayerRankDataA* rank_array,
                                        u64 rank_array_size, OrbisNpScoreComment* comment_array,
                                        u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
                                        u64 info_array_size, u64 array_num,
                                        Rtc::OrbisRtcTick* last_sort_date,
                                        OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreGetRankingByAccountIdPcIdAsync(
    s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpScoreAccountIdPcId* id_array,
    u64 id_array_size, OrbisNpScorePlayerRankDataA* rank_array, u64 rank_array_size,
    OrbisNpScoreComment* comment_array, u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
    u64 info_array_size, u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
    OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreGetBoardInfo(s32 req_id, OrbisNpScoreBoardId board_id,
                           OrbisNpScoreBoardInfo* board_info, void* option);
s32 sceNpScoreGetBoardInfoAsync(s32 req_id, OrbisNpScoreBoardId board_id,
                                OrbisNpScoreBoardInfo* board_info, void* option);
s32 sceNpScoreGetGameData(s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpId* np_id,
                          u64* total_size, u64 recv_size, void* data, void* option);
s32 sceNpScoreGetGameDataAsync(s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpId* np_id,
                               u64* total_size, u64 recv_size, void* data, void* option);
s32 sceNpScoreGetGameDataByAccountId(s32 req_id, OrbisNpScoreBoardId board_id,
                                     OrbisNpAccountId account_id, u64* total_size, u64 recv_size,
                                     void* data, void* option);
s32 sceNpScoreGetGameDataByAccountIdAsync(s32 req_id, OrbisNpScoreBoardId board_id,
                                          OrbisNpAccountId account_id, u64* total_size,
                                          u64 recv_size, void* data, void* option);
s32 sceNpScoreRecordGameData(s32 req_id, OrbisNpScoreBoardId board_id, OrbisNpScoreValue score,
                             u64 total_size, u64 send_size, const void* data, void* option);
s32 sceNpScoreRecordGameDataAsync(s32 req_id, OrbisNpScoreBoardId board_id, OrbisNpScoreValue score,
                                  u64 total_size, u64 send_size, const void* data, void* option);
s32 sceNpScoreCensorComment(s32 req_id, const char* comment, void* option);
s32 sceNpScoreCensorCommentAsync(s32 req_id, const char* comment, void* option);

s32 sceNpScoreSanitizeComment(s32 req_id, const char* comment, char* sanitized_comment,
                              void* option);
s32 sceNpScoreSanitizeCommentAsync(s32 req_id, const char* comment, char* sanitized_comment,
                                   void* option);
s32 sceNpScoreSetPlayerCharacterId(s32 ctx_id, OrbisNpScorePcId pc_id);
s32 sceNpScoreChangeModeForOtherSaveDataOwners();
s32 sceNpScoreCreateTitleCtx();
s32 sceNpScoreGetFriendsRankingForCrossSave(
    s32 req_id, OrbisNpScoreBoardId board_id, s32 include_self,
    OrbisNpScoreRankDataForCrossSave* rank_array, u64 rank_array_size,
    OrbisNpScoreComment* comment_array, u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
    u64 info_array_size, u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
    OrbisNpScoreRankNumber* total_record, OrbisNpScoreGetFriendRankingOptParam* option);
s32 sceNpScoreGetFriendsRankingForCrossSaveAsync(
    s32 req_id, OrbisNpScoreBoardId board_id, s32 include_self,
    OrbisNpScoreRankDataForCrossSave* rank_array, u64 rank_array_size,
    OrbisNpScoreComment* comment_array, u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
    u64 info_array_size, u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
    OrbisNpScoreRankNumber* total_record, OrbisNpScoreGetFriendRankingOptParam* option);
s32 sceNpScoreGetRankingByAccountIdForCrossSave(
    s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpAccountId* account_id_array,
    u64 account_id_array_size, OrbisNpScorePlayerRankDataForCrossSave* rank_array,
    u64 rank_array_size, OrbisNpScoreComment* comment_array, u64 comment_array_size,
    OrbisNpScoreGameInfo* info_array, u64 info_array_size, u64 array_num,
    Rtc::OrbisRtcTick* last_sort_date, OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreGetRankingByAccountIdForCrossSaveAsync(
    s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpAccountId* account_id_array,
    u64 account_id_array_size, OrbisNpScorePlayerRankDataForCrossSave* rank_array,
    u64 rank_array_size, OrbisNpScoreComment* comment_array, u64 comment_array_size,
    OrbisNpScoreGameInfo* info_array, u64 info_array_size, u64 array_num,
    Rtc::OrbisRtcTick* last_sort_date, OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreGetRankingByAccountIdPcIdForCrossSave(
    s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpScoreAccountIdPcId* id_array,
    u64 id_array_size, OrbisNpScorePlayerRankDataForCrossSave* rank_array, u64 rank_array_size,
    OrbisNpScoreComment* comment_array, u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
    u64 info_array_size, u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
    OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreGetRankingByAccountIdPcIdForCrossSaveAsync(
    s32 req_id, OrbisNpScoreBoardId board_id, const OrbisNpScoreAccountIdPcId* id_array,
    u64 id_array_size, OrbisNpScorePlayerRankDataForCrossSave* rank_array, u64 rank_array_size,
    OrbisNpScoreComment* comment_array, u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
    u64 info_array_size, u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
    OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreGetRankingByRangeForCrossSave(s32 req_id, OrbisNpScoreBoardId board_id,
                                            OrbisNpScoreRankNumber start_serial_rank,
                                            OrbisNpScoreRankDataForCrossSave* rank_array,
                                            u64 rank_array_size, OrbisNpScoreComment* comment_array,
                                            u64 comment_array_size,
                                            OrbisNpScoreGameInfo* info_array, u64 info_array_size,
                                            u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
                                            OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreGetRankingByRangeForCrossSaveAsync(
    s32 req_id, OrbisNpScoreBoardId board_id, OrbisNpScoreRankNumber start_serial_rank,
    OrbisNpScoreRankDataForCrossSave* rank_array, u64 rank_array_size,
    OrbisNpScoreComment* comment_array, u64 comment_array_size, OrbisNpScoreGameInfo* info_array,
    u64 info_array_size, u64 array_num, Rtc::OrbisRtcTick* last_sort_date,
    OrbisNpScoreRankNumber* total_record, void* option);
s32 sceNpScoreSetThreadParam(s32 thread_priority, u64 cpu_affinity_mask);
s32 sceNpScoreSetTimeout(s32 id, s32 resolve_retry, s32 resolve_timeout, s32 conn_timeout,
                         s32 send_timeout, s32 recv_timeout);
void RegisterHooks();
} // namespace Libraries::Np::NpScore
