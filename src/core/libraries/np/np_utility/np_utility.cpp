// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string_view>
#include <orbis/NpUtility.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_utility/np_utility.h"
#include "core/libraries/np/np_utility/np_utility_ctx.h"

SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterAbortRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterCensorComment);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterCreateAsyncRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterCreateRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterCreateTitleCtx);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterCreateTitleCtxA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterDeleteRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterDeleteTitleCtx);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterPollAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterSanitizeComment);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterSetTimeout);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterWaitAsync);

static void RegisterNpUtilityHooks() {
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterAbortRequest);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterCensorComment);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterCreateAsyncRequest);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterCreateRequest);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterCreateTitleCtx);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterCreateTitleCtxA);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterDeleteRequest);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterDeleteTitleCtx);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterPollAsync);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterSanitizeComment);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterSetTimeout);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterWaitAsync);
}

namespace Libraries::Np::NpUtility {

static std::mutex g_word_filter_mutex;
static std::map<OrbisNpWordFilterTitleCtxId, WordFilterTitleCtx> g_word_filter_title_ctxs;
static std::map<OrbisNpWordFilterRequestId, std::shared_ptr<WordFilterRequestCtx>>
    g_word_filter_requests;
static OrbisNpWordFilterTitleCtxId g_word_filter_next_ctx_id =
    ORBIS_NP_WORD_FILTER_TITLE_CTX_ID_OFFSET + 1;
static OrbisNpWordFilterRequestId g_word_filter_next_req_id =
    ORBIS_NP_WORD_FILTER_REQUEST_ID_OFFSET + 1;

static std::string_view OnlineIdView(const OrbisNpOnlineId& id) {
    return std::string_view(id.data, strnlen(id.data, ORBIS_NP_ONLINEID_MAX_LENGTH));
}

static std::shared_ptr<WordFilterRequestCtx> GetWordFilterRequest(
    OrbisNpWordFilterRequestId reqId) {
    std::lock_guard lock(g_word_filter_mutex);
    const auto it = g_word_filter_requests.find(reqId);
    return it != g_word_filter_requests.end() ? it->second : nullptr;
}

static s32 StartWordFilterRequest(const std::shared_ptr<WordFilterRequestCtx>& req) {
    std::lock_guard lock(req->mutex);
    if (req->aborted) {
        return ORBIS_NP_COMMUNITY_ERROR_ABORTED;
    }
    if (req->used) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_TYPE;
    }
    req->used = true;
    return ORBIS_OK;
}

static s32 CompleteWordFilterRequest(const std::shared_ptr<WordFilterRequestCtx>& req, s32 result) {
    req->SetResult(result);
    return req->isAsync ? ORBIS_OK : result;
}

s32 sceNpWordFilterAbortRequest(OrbisNpWordFilterRequestId reqId) {
    const auto req = GetWordFilterRequest(reqId);
    if (!req) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    {
        std::lock_guard lock(req->mutex);
        req->aborted = true;
    }
    req->SetResult(ORBIS_NP_COMMUNITY_ERROR_ABORTED);
    return ORBIS_OK;
}

s32 sceNpWordFilterCensorComment(OrbisNpWordFilterRequestId reqId, const char* comment,
                                 void* option) {
    if (comment == nullptr) {
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option != nullptr) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const u64 length = strnlen(comment, ORBIS_NP_WORD_FILTER_COMMENT_MAX_LENGTH + 1);
    if (length > ORBIS_NP_WORD_FILTER_COMMENT_MAX_LENGTH) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const auto req = GetWordFilterRequest(reqId);
    if (!req) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    const s32 result = StartWordFilterRequest(req);
    if (result < 0) {
        return result;
    }
    LOG_INFO(Lib_NpUtility, "reqId={} len={}", reqId, length);
    return CompleteWordFilterRequest(req, ORBIS_OK);
}

static s32 CreateWordFilterRequest(OrbisNpWordFilterTitleCtxId titleCtxId, bool isAsync) {
    std::lock_guard lock(g_word_filter_mutex);
    const auto ctx_it = g_word_filter_title_ctxs.find(titleCtxId);
    if (ctx_it == g_word_filter_title_ctxs.end()) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    if (static_cast<s32>(g_word_filter_requests.size()) >= ORBIS_NP_WORD_FILTER_MAX_REQUEST_NUM) {
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
    }
    const OrbisNpWordFilterRequestId id = g_word_filter_next_req_id++;
    auto req = std::make_shared<WordFilterRequestCtx>();
    req->titleCtxId = titleCtxId;
    req->isAsync = isAsync;
    req->timeouts = ctx_it->second.timeouts;
    g_word_filter_requests.emplace(id, std::move(req));
    LOG_INFO(Lib_NpUtility, "id={} titleCtxId={} async={}", id, titleCtxId, isAsync);
    return id;
}

s32 sceNpWordFilterCreateAsyncRequest(OrbisNpWordFilterTitleCtxId titleCtxId,
                                      const OrbisNpWordFilterCreateAsyncRequestParameter* param) {
    if (param == nullptr || param->size != sizeof(OrbisNpWordFilterCreateAsyncRequestParameter)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    return CreateWordFilterRequest(titleCtxId, true);
}

s32 sceNpWordFilterCreateRequest(OrbisNpWordFilterTitleCtxId titleCtxId) {
    return CreateWordFilterRequest(titleCtxId, false);
}

static s32 CreateWordFilterTitleCtx(s32 userId, const OrbisNpId& selfNpId) {
    std::lock_guard lock(g_word_filter_mutex);
    if (static_cast<s32>(g_word_filter_title_ctxs.size()) >= ORBIS_NP_WORD_FILTER_MAX_CTX_NUM) {
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
    }
    const OrbisNpWordFilterTitleCtxId id = g_word_filter_next_ctx_id++;
    g_word_filter_title_ctxs.emplace(id,
                                     WordFilterTitleCtx{.userId = userId, .selfNpId = selfNpId});
    LOG_INFO(Lib_NpUtility, "id={} userId={} onlineId='{}'", id, userId,
             OnlineIdView(selfNpId.handle));
    return id;
}

s32 sceNpWordFilterCreateTitleCtx(const OrbisNpId* selfNpId) {
    if (selfNpId == nullptr) {
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    const s32 userId = NpHandler::Instance().GetUserIdByOnlineId(selfNpId->handle);
    return CreateWordFilterTitleCtx(userId, *selfNpId);
}

s32 sceNpWordFilterCreateTitleCtxA(UserService::OrbisUserServiceUserId userId) {
    return CreateWordFilterTitleCtx(userId, NpHandler::Instance().GetNpId(userId));
}

s32 sceNpWordFilterDeleteRequest(OrbisNpWordFilterRequestId reqId) {
    std::shared_ptr<WordFilterRequestCtx> req;
    {
        std::lock_guard lock(g_word_filter_mutex);
        const auto it = g_word_filter_requests.find(reqId);
        if (it == g_word_filter_requests.end()) {
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        req = it->second;
        g_word_filter_requests.erase(it);
    }
    {
        std::lock_guard lock(req->mutex);
        req->aborted = true;
    }
    req->SetResult(ORBIS_NP_COMMUNITY_ERROR_ABORTED);
    return ORBIS_OK;
}

s32 sceNpWordFilterDeleteTitleCtx(OrbisNpWordFilterTitleCtxId titleCtxId) {
    std::lock_guard lock(g_word_filter_mutex);
    if (!g_word_filter_title_ctxs.contains(titleCtxId)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    for (auto it = g_word_filter_requests.begin(); it != g_word_filter_requests.end();) {
        if (it->second->titleCtxId == titleCtxId) {
            {
                std::lock_guard req_lock(it->second->mutex);
                it->second->aborted = true;
            }
            it->second->SetResult(ORBIS_NP_COMMUNITY_ERROR_ABORTED);
            it = g_word_filter_requests.erase(it);
        } else {
            ++it;
        }
    }
    g_word_filter_title_ctxs.erase(titleCtxId);
    return ORBIS_OK;
}

s32 sceNpWordFilterPollAsync(OrbisNpWordFilterRequestId reqId, s32* result) {
    if (result == nullptr) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const auto req = GetWordFilterRequest(reqId);
    if (!req || !req->isAsync) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    std::lock_guard lock(req->mutex);
    if (!req->result.has_value()) {
        return ORBIS_NP_WORD_FILTER_POLL_ASYNC_RET_RUNNING;
    }
    *result = *req->result;
    return ORBIS_NP_WORD_FILTER_POLL_ASYNC_RET_FINISHED;
}

s32 sceNpWordFilterSanitizeComment(OrbisNpWordFilterRequestId reqId, const char* comment,
                                   char* sanitizedComment, void* option) {
    if (comment == nullptr || sanitizedComment == nullptr) {
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option != nullptr) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const u64 length = strnlen(comment, ORBIS_NP_WORD_FILTER_COMMENT_MAX_LENGTH + 1);
    if (length > ORBIS_NP_WORD_FILTER_COMMENT_MAX_LENGTH) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const auto req = GetWordFilterRequest(reqId);
    if (!req) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    const s32 result = StartWordFilterRequest(req);
    if (result < 0) {
        return result;
    }
    std::memcpy(sanitizedComment, comment, length + 1);
    LOG_INFO(Lib_NpUtility, "reqId={} len={}", reqId, length);
    return CompleteWordFilterRequest(req, ORBIS_OK);
}

s32 sceNpWordFilterSetTimeout(s32 id, s32 resolveRetry, u32 resolveTimeout, u32 connTimeout,
                              u32 sendTimeout, u32 recvTimeout) {
    if (resolveRetry < 0 || (resolveTimeout != 0 && resolveTimeout < 1000000) ||
        (connTimeout != 0 && connTimeout < 10000000) ||
        (sendTimeout != 0 && sendTimeout < 10000000) ||
        (recvTimeout != 0 && recvTimeout < 10000000) ||
        (resolveRetry == 0 && resolveTimeout == 0 && connTimeout == 0 && sendTimeout == 0 &&
         recvTimeout == 0)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const LookupTimeouts timeouts{
        .resolveRetry = resolveRetry,
        .resolveTimeout = resolveTimeout,
        .connTimeout = connTimeout,
        .sendTimeout = sendTimeout,
        .recvTimeout = recvTimeout,
    };
    std::lock_guard lock(g_word_filter_mutex);
    if (auto ctx_it = g_word_filter_title_ctxs.find(id); ctx_it != g_word_filter_title_ctxs.end()) {
        ctx_it->second.timeouts = timeouts;
        return ORBIS_OK;
    }
    if (auto req_it = g_word_filter_requests.find(id); req_it != g_word_filter_requests.end()) {
        std::lock_guard req_lock(req_it->second->mutex);
        req_it->second->timeouts = timeouts;
        return ORBIS_OK;
    }
    return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
}

s32 sceNpWordFilterWaitAsync(OrbisNpWordFilterRequestId reqId, s32* result) {
    if (result == nullptr) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const auto req = GetWordFilterRequest(reqId);
    if (!req || !req->isAsync) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    *result = req->Wait();
    return ORBIS_OK;
}

void RegisterHooks() {
    return RegisterNpUtilityHooks();
}

} // namespace Libraries::Np::NpUtility
