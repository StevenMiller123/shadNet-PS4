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
    OrbisNpWordFilterRequestId req_id) {
    std::lock_guard lock(g_word_filter_mutex);
    const auto it = g_word_filter_requests.find(req_id);
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
    return req->is_async ? ORBIS_OK : result;
}

s32 sceNpWordFilterAbortRequest(OrbisNpWordFilterRequestId req_id) {
    const auto req = GetWordFilterRequest(req_id);
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

s32 sceNpWordFilterCensorComment(OrbisNpWordFilterRequestId req_id, const char* comment,
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
    const auto req = GetWordFilterRequest(req_id);
    if (!req) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    const s32 result = StartWordFilterRequest(req);
    if (result < 0) {
        return result;
    }
    LOG_INFO(Lib_NpUtility, "req_id={} len={}", req_id, length);
    return CompleteWordFilterRequest(req, ORBIS_OK);
}

static s32 CreateWordFilterRequest(OrbisNpWordFilterTitleCtxId title_ctx_id, bool is_async) {
    std::lock_guard lock(g_word_filter_mutex);
    const auto ctx_it = g_word_filter_title_ctxs.find(title_ctx_id);
    if (ctx_it == g_word_filter_title_ctxs.end()) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    if (static_cast<s32>(g_word_filter_requests.size()) >= ORBIS_NP_WORD_FILTER_MAX_REQUEST_NUM) {
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
    }
    const OrbisNpWordFilterRequestId id = g_word_filter_next_req_id++;
    auto req = std::make_shared<WordFilterRequestCtx>();
    req->title_ctx_id = title_ctx_id;
    req->is_async = is_async;
    req->timeouts = ctx_it->second.timeouts;
    g_word_filter_requests.emplace(id, std::move(req));
    LOG_INFO(Lib_NpUtility, "id={} title_ctx_id={} async={}", id, title_ctx_id, is_async);
    return id;
}

s32 sceNpWordFilterCreateAsyncRequest(OrbisNpWordFilterTitleCtxId title_ctx_id,
                                      const OrbisNpWordFilterCreateAsyncRequestParameter* param) {
    if (param == nullptr || param->size != sizeof(OrbisNpWordFilterCreateAsyncRequestParameter)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    return CreateWordFilterRequest(title_ctx_id, true);
}

s32 sceNpWordFilterCreateRequest(OrbisNpWordFilterTitleCtxId title_ctx_id) {
    return CreateWordFilterRequest(title_ctx_id, false);
}

static s32 CreateWordFilterTitleCtx(s32 user_id, const OrbisNpId& self_np_id) {
    std::lock_guard lock(g_word_filter_mutex);
    if (static_cast<s32>(g_word_filter_title_ctxs.size()) >= ORBIS_NP_WORD_FILTER_MAX_CTX_NUM) {
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
    }
    const OrbisNpWordFilterTitleCtxId id = g_word_filter_next_ctx_id++;
    g_word_filter_title_ctxs.emplace(
        id, WordFilterTitleCtx{.user_id = user_id, .self_np_id = self_np_id});
    LOG_INFO(Lib_NpUtility, "id={} user_id={} online_id='{}'", id, user_id,
             OnlineIdView(self_np_id.handle));
    return id;
}

s32 sceNpWordFilterCreateTitleCtx(const OrbisNpId* self_np_id) {
    if (self_np_id == nullptr) {
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    const s32 user_id = NpHandler::Instance().GetUserIdByOnlineId(self_np_id->handle);
    return CreateWordFilterTitleCtx(user_id, *self_np_id);
}

s32 sceNpWordFilterCreateTitleCtxA(UserService::OrbisUserServiceUserId user_id) {
    return CreateWordFilterTitleCtx(user_id, NpHandler::Instance().GetNpId(user_id));
}

s32 sceNpWordFilterDeleteRequest(OrbisNpWordFilterRequestId req_id) {
    std::shared_ptr<WordFilterRequestCtx> req;
    {
        std::lock_guard lock(g_word_filter_mutex);
        const auto it = g_word_filter_requests.find(req_id);
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

s32 sceNpWordFilterDeleteTitleCtx(OrbisNpWordFilterTitleCtxId title_ctx_id) {
    std::lock_guard lock(g_word_filter_mutex);
    if (!g_word_filter_title_ctxs.contains(title_ctx_id)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    for (auto it = g_word_filter_requests.begin(); it != g_word_filter_requests.end();) {
        if (it->second->title_ctx_id == title_ctx_id) {
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
    g_word_filter_title_ctxs.erase(title_ctx_id);
    return ORBIS_OK;
}

s32 sceNpWordFilterPollAsync(OrbisNpWordFilterRequestId req_id, s32* result) {
    if (result == nullptr) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const auto req = GetWordFilterRequest(req_id);
    if (!req || !req->is_async) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    std::lock_guard lock(req->mutex);
    if (!req->result.has_value()) {
        return ORBIS_NP_WORD_FILTER_POLL_ASYNC_RET_RUNNING;
    }
    *result = *req->result;
    return ORBIS_NP_WORD_FILTER_POLL_ASYNC_RET_FINISHED;
}

s32 sceNpWordFilterSanitizeComment(OrbisNpWordFilterRequestId req_id, const char* comment,
                                   char* sanitized_comment, void* option) {
    if (comment == nullptr || sanitized_comment == nullptr) {
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option != nullptr) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const u64 length = strnlen(comment, ORBIS_NP_WORD_FILTER_COMMENT_MAX_LENGTH + 1);
    if (length > ORBIS_NP_WORD_FILTER_COMMENT_MAX_LENGTH) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const auto req = GetWordFilterRequest(req_id);
    if (!req) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    const s32 result = StartWordFilterRequest(req);
    if (result < 0) {
        return result;
    }
    std::memcpy(sanitized_comment, comment, length + 1);
    LOG_INFO(Lib_NpUtility, "req_id={} len={}", req_id, length);
    return CompleteWordFilterRequest(req, ORBIS_OK);
}

s32 sceNpWordFilterSetTimeout(s32 id, s32 resolve_retry, u32 resolve_timeout, u32 conn_timeout,
                              u32 send_timeout, u32 recv_timeout) {
    if (resolve_retry < 0 || (resolve_timeout != 0 && resolve_timeout < 1000000) ||
        (conn_timeout != 0 && conn_timeout < 10000000) ||
        (send_timeout != 0 && send_timeout < 10000000) ||
        (recv_timeout != 0 && recv_timeout < 10000000) ||
        (resolve_retry == 0 && resolve_timeout == 0 && conn_timeout == 0 && send_timeout == 0 &&
         recv_timeout == 0)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const LookupTimeouts timeouts{
        .resolve_retry = resolve_retry,
        .resolve_timeout = resolve_timeout,
        .conn_timeout = conn_timeout,
        .send_timeout = send_timeout,
        .recv_timeout = recv_timeout,
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

s32 sceNpWordFilterWaitAsync(OrbisNpWordFilterRequestId req_id, s32* result) {
    if (result == nullptr) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const auto req = GetWordFilterRequest(req_id);
    if (!req || !req->is_async) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    *result = req->Wait();
    return ORBIS_OK;
}

void RegisterHooks() {
    return RegisterNpUtilityHooks();
}

} // namespace Libraries::Np::NpUtility
