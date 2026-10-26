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
    u32 rankLimit;
    u32 updateMode;
    u32 sortMode;
    OrbisNpScoreRankNumber uploadNumLimit;
    u64 uploadSizeLimit;
};
static_assert(sizeof(OrbisNpScoreBoardInfo) == 24, "OrbisNpScoreBoardInfo must be 24 bytes");

struct OrbisNpScoreComment {
    char utf8Comment[ORBIS_NP_SCORE_COMMENT_MAXLEN + 1];
};
static_assert(sizeof(OrbisNpScoreComment) == 64, "OrbisNpScoreComment must be 64 bytes");

struct OrbisNpScoreGameInfo {
    u64 infoSize;
    u8 data[ORBIS_NP_SCORE_GAMEINFO_MAXSIZE];
    u8 pad2[3];
};
static_assert(sizeof(OrbisNpScoreGameInfo) == 200, "OrbisNpScoreGameInfo must be 200 bytes");

struct OrbisNpScoreGetFriendRankingOptParam {
    u64 size;
    u32* startSerialRank;
    u32* hits;
};

struct OrbisNpScoreAccountIdPcId {
    OrbisNpAccountId accountId;
    OrbisNpScorePcId pcId;
    u8 pad[4];
};

struct OrbisNpScoreNpIdPcId {
    OrbisNpId npId;
    OrbisNpScorePcId pcId;
    u8 pad[4];
};

struct OrbisNpScoreRankData {
    OrbisNpId npId;
    u8 reserved[49];
    u8 pad0[3];
    OrbisNpScorePcId pcId;
    OrbisNpScoreRankNumber serialRank;
    OrbisNpScoreRankNumber rank;
    OrbisNpScoreRankNumber highestRank;
    s32 hasGameData;
    u8 pad1[4];
    OrbisNpScoreValue scoreValue;
    Rtc::OrbisRtcTick recordDate;
};
static_assert(sizeof(OrbisNpScoreRankData) == 0x80, "OrbisNpScoreRankData must be 128 bytes");

struct OrbisNpScoreRankDataA {
    OrbisNpOnlineId onlineId;
    u8 reserved0[16];
    u8 reserved[49];
    u8 pad0[3];
    OrbisNpScorePcId pcId;
    OrbisNpScoreRankNumber serialRank;
    OrbisNpScoreRankNumber rank;
    OrbisNpScoreRankNumber highestRank;
    s32 hasGameData;
    u8 pad1[4];
    OrbisNpScoreValue scoreValue;
    Rtc::OrbisRtcTick recordDate;
    OrbisNpAccountId accountId;
    u8 pad2[8];
};
static_assert(sizeof(OrbisNpScoreRankDataA) == 0x90, "OrbisNpScoreRankDataA must be 144 bytes");

struct OrbisNpScorePlayerRankData {
    s32 hasData;
    u8 pad0[4];
    OrbisNpScoreRankData rankData;
};
static_assert(sizeof(OrbisNpScorePlayerRankData) == 0x88,
              "OrbisNpScorePlayerRankData must be 136 bytes");

struct OrbisNpScorePlayerRankDataA {
    s32 hasData;
    u8 pad0[4];
    OrbisNpScoreRankDataA rankData;
};
static_assert(sizeof(OrbisNpScorePlayerRankDataA) == 0x98,
              "OrbisNpScorePlayerRankDataA must be 152 bytes");

struct OrbisNpScoreRankDataForCrossSave {
    OrbisNpId npId;
    u8 reserved[49];
    u8 pad0[3];
    OrbisNpScorePcId pcId;
    OrbisNpScoreRankNumber serialRank;
    OrbisNpScoreRankNumber rank;
    OrbisNpScoreRankNumber highestRank;
    s32 hasGameData;
    u8 pad1[4];
    OrbisNpScoreValue scoreValue;
    Rtc::OrbisRtcTick recordDate;
    OrbisNpAccountId accountId;
    u8 pad2[8];
};

struct OrbisNpScorePlayerRankDataForCrossSave {
    s32 hasData;
    u8 pad0[4];
    OrbisNpScoreRankDataForCrossSave rankData;
};

s32 sceNpScoreCreateNpTitleCtx(OrbisNpServiceLabel serviceLabel, const OrbisNpId* selfNpId);
s32 sceNpScoreCreateNpTitleCtxA(OrbisNpServiceLabel npServiceLabel,
                                UserService::OrbisUserServiceUserId selfId);
s32 sceNpScoreDeleteNpTitleCtx(s32 titleCtxId);
s32 sceNpScoreCreateRequest(s32 titleCtxId);
s32 sceNpScoreDeleteRequest(s32 reqId);
s32 sceNpScoreAbortRequest(s32 reqId);
s32 sceNpScorePollAsync(s32 reqId, s32* result);
s32 sceNpScoreWaitAsync(s32 reqId, s32* result);
s32 sceNpScoreRecordScore(s32 reqId, OrbisNpScoreBoardId boardId, OrbisNpScoreValue score,
                          const OrbisNpScoreComment* scoreComment,
                          const OrbisNpScoreGameInfo* gameInfo, OrbisNpScoreRankNumber* tmpRank,
                          const Rtc::OrbisRtcTick* compareDate, void* option);
s32 sceNpScoreRecordScoreAsync(s32 reqId, OrbisNpScoreBoardId boardId, OrbisNpScoreValue score,
                               const OrbisNpScoreComment* scoreComment,
                               const OrbisNpScoreGameInfo* gameInfo,
                               OrbisNpScoreRankNumber* tmpRank,
                               const Rtc::OrbisRtcTick* compareDate, void* option);
s32 sceNpScoreGetRankingByRange(s32 reqId, OrbisNpScoreBoardId boardId,
                                OrbisNpScoreRankNumber startSerialRank,
                                OrbisNpScoreRankData* rankArray, u64 rankArraySize,
                                OrbisNpScoreComment* commentArray, u64 commentArraySize,
                                OrbisNpScoreGameInfo* infoArray, u64 infoArraySize, u64 arrayNum,
                                Rtc::OrbisRtcTick* lastSortDate,
                                OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreGetRankingByRangeAsync(s32 reqId, OrbisNpScoreBoardId boardId,
                                     OrbisNpScoreRankNumber startSerialRank,
                                     OrbisNpScoreRankData* rankArray, u64 rankArraySize,
                                     OrbisNpScoreComment* commentArray, u64 commentArraySize,
                                     OrbisNpScoreGameInfo* infoArray, u64 infoArraySize,
                                     u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
                                     OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreGetRankingByRangeA(s32 reqId, OrbisNpScoreBoardId boardId,
                                 OrbisNpScoreRankNumber startSerialRank,
                                 OrbisNpScoreRankDataA* rankArray, u64 rankArraySize,
                                 OrbisNpScoreComment* commentArray, u64 commentArraySize,
                                 OrbisNpScoreGameInfo* infoArray, u64 infoArraySize, u64 arrayNum,
                                 Rtc::OrbisRtcTick* lastSortDate,
                                 OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreGetRankingByRangeAAsync(s32 reqId, OrbisNpScoreBoardId boardId,
                                      OrbisNpScoreRankNumber startSerialRank,
                                      OrbisNpScoreRankDataA* rankArray, u64 rankArraySize,
                                      OrbisNpScoreComment* commentArray, u64 commentArraySize,
                                      OrbisNpScoreGameInfo* infoArray, u64 infoArraySize,
                                      u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
                                      OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreGetRankingByNpId(s32 reqId, OrbisNpScoreBoardId boardId, const OrbisNpId* npIdArray,
                               u64 npIdArraySize, OrbisNpScorePlayerRankData* rankArray,
                               u64 rankArraySize, OrbisNpScoreComment* commentArray,
                               u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
                               u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
                               OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreGetRankingByNpIdAsync(s32 reqId, OrbisNpScoreBoardId boardId,
                                    const OrbisNpId* npIdArray, u64 npIdArraySize,
                                    OrbisNpScorePlayerRankData* rankArray, u64 rankArraySize,
                                    OrbisNpScoreComment* commentArray, u64 commentArraySize,
                                    OrbisNpScoreGameInfo* infoArray, u64 infoArraySize,
                                    u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
                                    OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreGetFriendsRanking(s32 reqId, OrbisNpScoreBoardId boardId, s32 includeSelf,
                                OrbisNpScoreRankData* rankArray, u64 rankArraySize,
                                OrbisNpScoreComment* commentArray, u64 commentArraySize,
                                OrbisNpScoreGameInfo* infoArray, u64 infoArraySize, u64 arrayNum,
                                Rtc::OrbisRtcTick* lastSortDate,
                                OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreGetFriendsRankingAsync(s32 reqId, OrbisNpScoreBoardId boardId, s32 includeSelf,
                                     OrbisNpScoreRankData* rankArray, u64 rankArraySize,
                                     OrbisNpScoreComment* commentArray, u64 commentArraySize,
                                     OrbisNpScoreGameInfo* infoArray, u64 infoArraySize,
                                     u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
                                     OrbisNpScoreRankNumber* totalRecord,
                                     OrbisNpScoreGetFriendRankingOptParam* option);
s32 sceNpScoreGetFriendsRankingA(s32 reqId, OrbisNpScoreBoardId boardId, s32 includeSelf,
                                 OrbisNpScoreRankDataA* rankArray, u64 rankArraySize,
                                 OrbisNpScoreComment* commentArray, u64 commentArraySize,
                                 OrbisNpScoreGameInfo* infoArray, u64 infoArraySize, u64 arrayNum,
                                 Rtc::OrbisRtcTick* lastSortDate,
                                 OrbisNpScoreRankNumber* totalRecord,
                                 OrbisNpScoreGetFriendRankingOptParam* option);
s32 sceNpScoreGetFriendsRankingAAsync(s32 reqId, OrbisNpScoreBoardId boardId, s32 includeSelf,
                                      OrbisNpScoreRankDataA* rankArray, u64 rankArraySize,
                                      OrbisNpScoreComment* commentArray, u64 commentArraySize,
                                      OrbisNpScoreGameInfo* infoArray, u64 infoArraySize,
                                      u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
                                      OrbisNpScoreRankNumber* totalRecord,
                                      OrbisNpScoreGetFriendRankingOptParam* option);
s32 sceNpScoreGetRankingByNpIdPcId(s32 reqId, OrbisNpScoreBoardId boardId,
                                   const OrbisNpScoreNpIdPcId* idArray, u64 idArraySize,
                                   OrbisNpScorePlayerRankData* rankArray, u64 rankArraySize,
                                   OrbisNpScoreComment* commentArray, u64 commentArraySize,
                                   OrbisNpScoreGameInfo* infoArray, u64 infoArraySize, u64 arrayNum,
                                   Rtc::OrbisRtcTick* lastSortDate,
                                   OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreGetRankingByNpIdPcIdAsync(s32 reqId, OrbisNpScoreBoardId boardId,
                                        const OrbisNpScoreNpIdPcId* idArray, u64 idArraySize,
                                        OrbisNpScorePlayerRankData* rankArray, u64 rankArraySize,
                                        OrbisNpScoreComment* commentArray, u64 commentArraySize,
                                        OrbisNpScoreGameInfo* infoArray, u64 infoArraySize,
                                        u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
                                        OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreGetRankingByAccountId(s32 reqId, OrbisNpScoreBoardId boardId,
                                    const OrbisNpAccountId* accountIdArray, u64 accountIdArraySize,
                                    OrbisNpScorePlayerRankDataA* rankArray, u64 rankArraySize,
                                    OrbisNpScoreComment* commentArray, u64 commentArraySize,
                                    OrbisNpScoreGameInfo* infoArray, u64 infoArraySize,
                                    u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
                                    OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreGetRankingByAccountIdAsync(s32 reqId, OrbisNpScoreBoardId boardId,
                                         const OrbisNpAccountId* accountIdArray,
                                         u64 accountIdArraySize,
                                         OrbisNpScorePlayerRankDataA* rankArray, u64 rankArraySize,
                                         OrbisNpScoreComment* commentArray, u64 commentArraySize,
                                         OrbisNpScoreGameInfo* infoArray, u64 infoArraySize,
                                         u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
                                         OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreGetRankingByAccountIdPcId(s32 reqId, OrbisNpScoreBoardId boardId,
                                        const OrbisNpScoreAccountIdPcId* idArray, u64 idArraySize,
                                        OrbisNpScorePlayerRankDataA* rankArray, u64 rankArraySize,
                                        OrbisNpScoreComment* commentArray, u64 commentArraySize,
                                        OrbisNpScoreGameInfo* infoArray, u64 infoArraySize,
                                        u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
                                        OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreGetRankingByAccountIdPcIdAsync(
    s32 reqId, OrbisNpScoreBoardId boardId, const OrbisNpScoreAccountIdPcId* idArray,
    u64 idArraySize, OrbisNpScorePlayerRankDataA* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreGetBoardInfo(s32 reqId, OrbisNpScoreBoardId boardId, OrbisNpScoreBoardInfo* boardInfo,
                           void* option);
s32 sceNpScoreGetBoardInfoAsync(s32 reqId, OrbisNpScoreBoardId boardId,
                                OrbisNpScoreBoardInfo* boardInfo, void* option);
s32 sceNpScoreGetGameData(s32 reqId, OrbisNpScoreBoardId boardId, const OrbisNpId* npId,
                          u64* totalSize, u64 recvSize, void* data, void* option);
s32 sceNpScoreGetGameDataAsync(s32 reqId, OrbisNpScoreBoardId boardId, const OrbisNpId* npId,
                               u64* totalSize, u64 recvSize, void* data, void* option);
s32 sceNpScoreGetGameDataByAccountId(s32 reqId, OrbisNpScoreBoardId boardId,
                                     OrbisNpAccountId accountId, u64* totalSize, u64 recvSize,
                                     void* data, void* option);
s32 sceNpScoreGetGameDataByAccountIdAsync(s32 reqId, OrbisNpScoreBoardId boardId,
                                          OrbisNpAccountId accountId, u64* totalSize, u64 recvSize,
                                          void* data, void* option);
s32 sceNpScoreRecordGameData(s32 reqId, OrbisNpScoreBoardId boardId, OrbisNpScoreValue score,
                             u64 totalSize, u64 sendSize, const void* data, void* option);
s32 sceNpScoreRecordGameDataAsync(s32 reqId, OrbisNpScoreBoardId boardId, OrbisNpScoreValue score,
                                  u64 totalSize, u64 sendSize, const void* data, void* option);
s32 sceNpScoreCensorComment(s32 reqId, const char* comment, void* option);
s32 sceNpScoreCensorCommentAsync(s32 reqId, const char* comment, void* option);

s32 sceNpScoreSanitizeComment(s32 reqId, const char* comment, char* sanitizedComment, void* option);
s32 sceNpScoreSanitizeCommentAsync(s32 reqId, const char* comment, char* sanitizedComment,
                                   void* option);
s32 sceNpScoreSetPlayerCharacterId(s32 ctxId, OrbisNpScorePcId pcId);
s32 sceNpScoreChangeModeForOtherSaveDataOwners();
s32 sceNpScoreCreateTitleCtx();
s32 sceNpScoreGetFriendsRankingForCrossSave(s32 reqId, OrbisNpScoreBoardId boardId, s32 includeSelf,
                                            OrbisNpScoreRankDataForCrossSave* rankArray,
                                            u64 rankArraySize, OrbisNpScoreComment* commentArray,
                                            u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
                                            u64 infoArraySize, u64 arrayNum,
                                            Rtc::OrbisRtcTick* lastSortDate,
                                            OrbisNpScoreRankNumber* totalRecord,
                                            OrbisNpScoreGetFriendRankingOptParam* option);
s32 sceNpScoreGetFriendsRankingForCrossSaveAsync(
    s32 reqId, OrbisNpScoreBoardId boardId, s32 includeSelf,
    OrbisNpScoreRankDataForCrossSave* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, OrbisNpScoreGetFriendRankingOptParam* option);
s32 sceNpScoreGetRankingByAccountIdForCrossSave(
    s32 reqId, OrbisNpScoreBoardId boardId, const OrbisNpAccountId* accountIdArray,
    u64 accountIdArraySize, OrbisNpScorePlayerRankDataForCrossSave* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreGetRankingByAccountIdForCrossSaveAsync(
    s32 reqId, OrbisNpScoreBoardId boardId, const OrbisNpAccountId* accountIdArray,
    u64 accountIdArraySize, OrbisNpScorePlayerRankDataForCrossSave* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreGetRankingByAccountIdPcIdForCrossSave(
    s32 reqId, OrbisNpScoreBoardId boardId, const OrbisNpScoreAccountIdPcId* idArray,
    u64 idArraySize, OrbisNpScorePlayerRankDataForCrossSave* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreGetRankingByAccountIdPcIdForCrossSaveAsync(
    s32 reqId, OrbisNpScoreBoardId boardId, const OrbisNpScoreAccountIdPcId* idArray,
    u64 idArraySize, OrbisNpScorePlayerRankDataForCrossSave* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreGetRankingByRangeForCrossSave(s32 reqId, OrbisNpScoreBoardId boardId,
                                            OrbisNpScoreRankNumber startSerialRank,
                                            OrbisNpScoreRankDataForCrossSave* rankArray,
                                            u64 rankArraySize, OrbisNpScoreComment* commentArray,
                                            u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
                                            u64 infoArraySize, u64 arrayNum,
                                            Rtc::OrbisRtcTick* lastSortDate,
                                            OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreGetRankingByRangeForCrossSaveAsync(
    s32 reqId, OrbisNpScoreBoardId boardId, OrbisNpScoreRankNumber startSerialRank,
    OrbisNpScoreRankDataForCrossSave* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, void* option);
s32 sceNpScoreSetThreadParam(s32 threadPriority, u64 cpuAffinityMask);
s32 sceNpScoreSetTimeout(s32 id, s32 resolveRetry, s32 resolveTimeout, s32 connTimeout,
                         s32 sendTimeout, s32 recvTimeout);
void RegisterHooks();
} // namespace Libraries::Np::NpScore