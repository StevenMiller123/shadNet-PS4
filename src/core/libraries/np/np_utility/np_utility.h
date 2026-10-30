// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/user_service/user_service.h"

namespace Libraries::Np::NpUtility {

constexpr s32 ORBIS_NP_WORD_FILTER_MAX_CTX_NUM = 32;
constexpr s32 ORBIS_NP_WORD_FILTER_MAX_REQUEST_NUM = 32;
constexpr s32 ORBIS_NP_WORD_FILTER_TITLE_CTX_ID_OFFSET = 0x1000000;
constexpr s32 ORBIS_NP_WORD_FILTER_REQUEST_ID_OFFSET = 0x2000000;
constexpr u64 ORBIS_NP_WORD_FILTER_COMMENT_MAX_LENGTH = 0x400;
constexpr s32 ORBIS_NP_WORD_FILTER_POLL_ASYNC_RET_FINISHED = 0;
constexpr s32 ORBIS_NP_WORD_FILTER_POLL_ASYNC_RET_RUNNING = 1;

using OrbisNpWordFilterTitleCtxId = s32;
using OrbisNpWordFilterRequestId = s32;

struct OrbisNpWordFilterCreateAsyncRequestParameter {
    u64 size;
    u64 cpuAffinityMask;
    s32 threadPriority;
    u8 padding[4];
};
static_assert(sizeof(OrbisNpWordFilterCreateAsyncRequestParameter) == 0x18);

s32 sceNpWordFilterAbortRequest(OrbisNpWordFilterRequestId reqId);
s32 sceNpWordFilterCensorComment(OrbisNpWordFilterRequestId reqId, const char* comment,
                                 void* option);
s32 sceNpWordFilterCreateAsyncRequest(OrbisNpWordFilterTitleCtxId titleCtxId,
                                      const OrbisNpWordFilterCreateAsyncRequestParameter* param);
s32 sceNpWordFilterCreateRequest(OrbisNpWordFilterTitleCtxId titleCtxId);
s32 sceNpWordFilterCreateTitleCtx(const OrbisNpId* selfNpId);
s32 sceNpWordFilterCreateTitleCtxA(UserService::OrbisUserServiceUserId userId);
s32 sceNpWordFilterDeleteRequest(OrbisNpWordFilterRequestId reqId);
s32 sceNpWordFilterDeleteTitleCtx(OrbisNpWordFilterTitleCtxId titleCtxId);
s32 sceNpWordFilterPollAsync(OrbisNpWordFilterRequestId reqId, s32* result);
s32 sceNpWordFilterSanitizeComment(OrbisNpWordFilterRequestId reqId, const char* comment,
                                   char* sanitizedComment, void* option);
s32 sceNpWordFilterSetTimeout(s32 id, s32 resolveRetry, u32 resolveTimeout, u32 connTimeout,
                              u32 sendTimeout, u32 recvTimeout);
s32 sceNpWordFilterWaitAsync(OrbisNpWordFilterRequestId reqId, s32* result);

void RegisterHooks();
} // namespace Libraries::Np::NpUtility
