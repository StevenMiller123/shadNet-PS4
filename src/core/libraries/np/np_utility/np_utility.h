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
    u64 cpu_affinity_mask;
    s32 thread_priority;
    u8 padding[4];
};
static_assert(sizeof(OrbisNpWordFilterCreateAsyncRequestParameter) == 0x18);

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
