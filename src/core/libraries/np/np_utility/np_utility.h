// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/user_service/user_service.h"

namespace Libraries::Np::NpUtility {

constexpr s32 ORBIS_NP_LOOKUP_MAX_CTX_NUM = 32;
constexpr s32 ORBIS_NP_LOOKUP_MAX_REQUEST_NUM = 32;
constexpr s32 ORBIS_NP_LOOKUP_POLL_ASYNC_RET_FINISHED = 0;
constexpr s32 ORBIS_NP_LOOKUP_POLL_ASYNC_RET_RUNNING = 1;
constexpr u32 ORBIS_NP_LOOKUP_TIMEOUT_NO_EFFECT = 0;
constexpr s32 ORBIS_NP_WORD_FILTER_MAX_CTX_NUM = 32;
constexpr s32 ORBIS_NP_WORD_FILTER_MAX_REQUEST_NUM = 32;
constexpr s32 ORBIS_NP_WORD_FILTER_TITLE_CTX_ID_OFFSET = 0x1000000;
constexpr s32 ORBIS_NP_WORD_FILTER_REQUEST_ID_OFFSET = 0x2000000;
constexpr u64 ORBIS_NP_WORD_FILTER_COMMENT_MAX_LENGTH = 0x400;
constexpr s32 ORBIS_NP_WORD_FILTER_POLL_ASYNC_RET_FINISHED = 0;
constexpr s32 ORBIS_NP_WORD_FILTER_POLL_ASYNC_RET_RUNNING = 1;

using OrbisNpLookupTitleCtxId = s32;
using OrbisNpLookupRequestId = s32;
using OrbisNpWordFilterTitleCtxId = s32;
using OrbisNpWordFilterRequestId = s32;

struct OrbisNpLookupCreateAsyncRequestParameter {
    u64 size;
    u64 cpu_affinity_mask;
    s32 thread_priority;
    u8 padding[4];
};

struct OrbisNpWordFilterCreateAsyncRequestParameter {
    u64 size;
    u64 cpu_affinity_mask;
    s32 thread_priority;
    u8 padding[4];
};

s32 sceNpBandwidthTestAbort();
s32 sceNpBandwidthTestDownloadOnlyInitStart();
s32 sceNpBandwidthTestGetStatus();
s32 sceNpBandwidthTestInitStart();
s32 sceNpBandwidthTestInitStartDownload();
s32 sceNpBandwidthTestInitStartUpload();
s32 sceNpBandwidthTestShutdown();
s32 sceNpBandwidthTestShutdownWithDetailedInfo();
s32 sceNpBandwidthTestUploadOnlyInitStart();
s32 sceNpLookupAbortRequest(OrbisNpLookupRequestId reqId);
s32 sceNpLookupCreateAsyncRequest(OrbisNpLookupTitleCtxId titleCtxId,
                                  const OrbisNpLookupCreateAsyncRequestParameter* param);
s32 sceNpLookupCreateRequest(OrbisNpLookupTitleCtxId titleCtxId);
s32 sceNpLookupCreateTitleCtx(const OrbisNpId* selfNpId);
s32 sceNpLookupCreateTitleCtxA(UserService::OrbisUserServiceUserId userId);
s32 sceNpLookupDeleteRequest(OrbisNpLookupRequestId reqId);
s32 sceNpLookupDeleteTitleCtx(OrbisNpLookupTitleCtxId titleCtxId);
s32 sceNpLookupNetAbortRequest();
s32 sceNpLookupNetCensorComment();
s32 sceNpLookupNetConvertJidToNpId();
s32 sceNpLookupNetConvertNpIdToJid();
s32 sceNpLookupNetCreateRequest();
s32 sceNpLookupNetCreateTitleCtx();
s32 sceNpLookupNetDeleteRequest();
s32 sceNpLookupNetDeleteTitleCtx();
s32 sceNpLookupNetInit();
s32 sceNpLookupNetInitWithFunctionPointer();
s32 sceNpLookupNetInitWithMemoryPool();
s32 sceNpLookupNetIsInit();
s32 sceNpLookupNetNpId();
s32 sceNpLookupNetSanitizeComment();
s32 sceNpLookupNetSetTimeout();
s32 sceNpLookupNetTerm();
s32 sceNpLookupNpId(OrbisNpLookupRequestId reqId, const OrbisNpOnlineId* onlineId, OrbisNpId* npId,
                    void* option);
s32 sceNpLookupPollAsync(OrbisNpLookupRequestId reqId, s32* result);
s32 sceNpLookupSetTimeout(s32 id, s32 resolveRetry, u32 resolveTimeout, u32 connTimeout,
                          u32 sendTimeout, u32 recvTimeout);
s32 sceNpLookupWaitAsync(OrbisNpLookupRequestId reqId, s32* result);
s32 sceNpUtilityInit();
s32 sceNpUtilityTerm();
s32 sceNpWordFilterAbortRequest(OrbisNpWordFilterRequestId req_id);
s32 sceNpWordFilterCensorComment(OrbisNpWordFilterRequestId req_id, const char* comment,
                                 void* option);
s32 sceNpWordFilterCreateAsyncRequest(OrbisNpWordFilterTitleCtxId title_ctx_id,
                                      const OrbisNpWordFilterCreateAsyncRequestParameter* param);
s32 sceNpWordFilterCreateRequest(OrbisNpWordFilterTitleCtxId title_ctx_id);
s32 sceNpWordFilterCreateTitleCtx(const OrbisNpId* self_np_id);
s32 sceNpWordFilterCreateTitleCtxA(UserService::OrbisUserServiceUserId user_id);
s32 sceNpWordFilterDeleteRequest(OrbisNpWordFilterRequestId req_id);
s32 sceNpWordFilterDeleteTitleCtx(OrbisNpWordFilterTitleCtxId title_ctx_id);
s32 sceNpWordFilterPollAsync(OrbisNpWordFilterRequestId req_id, s32* result);
s32 sceNpWordFilterSanitizeComment(OrbisNpWordFilterRequestId req_id, const char* comment,
                                   char* sanitized_comment, void* option);
s32 sceNpWordFilterSetTimeout(s32 id, s32 resolve_retry, u32 resolve_timeout, u32 conn_timeout,
                              u32 send_timeout, u32 recv_timeout);
s32 sceNpWordFilterWaitAsync(OrbisNpWordFilterRequestId req_id, s32* result);

void RegisterHooks();
} // namespace Libraries::Np::NpUtility
