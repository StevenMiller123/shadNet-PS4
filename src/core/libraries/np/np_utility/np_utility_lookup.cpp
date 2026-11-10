// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>

#include "common/logging/log.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_utility/np_utility.h"
#include "core/libraries/np/np_utility/np_utility_ctx.h"

namespace Libraries::Np::NpUtility {

std::mutex g_lookup_mutex;
std::map<OrbisNpLookupTitleCtxId, RequestTitleCtx> g_lookup_title_ctxs;
std::map<OrbisNpLookupRequestId, std::shared_ptr<RequestCtx>> g_lookup_requests;
OrbisNpLookupTitleCtxId g_lookup_next_ctx_id = 1;
OrbisNpLookupRequestId g_lookup_next_req_id = 1;

s32 sceNpLookupCreateTitleCtx(const OrbisNpId* self_np_id) {
    if (self_np_id == nullptr) {
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    std::lock_guard lock(g_lookup_mutex);
    if (static_cast<s32>(g_lookup_title_ctxs.size()) >= ORBIS_NP_LOOKUP_MAX_CTX_NUM) {
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
    }
    const s32 user_id =
        Libraries::Np::NpHandler::GetInstance().GetUserIdByOnlineId(self_np_id->handle);
    const OrbisNpLookupTitleCtxId id = g_lookup_next_ctx_id++;
    g_lookup_title_ctxs[id] = RequestTitleCtx{.user_id = user_id, .self_np_id = *self_np_id};
    LOG_INFO(Lib_NpUtility, "id={} user_id={} online_id='{}'", id, user_id,
             ViewNpOnlineId(self_np_id->handle));
    return id;
}

s32 sceNpLookupCreateTitleCtxA(UserService::OrbisUserServiceUserId user_id) {
    std::lock_guard lock(g_lookup_mutex);
    if (static_cast<s32>(g_lookup_title_ctxs.size()) >= ORBIS_NP_LOOKUP_MAX_CTX_NUM) {
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
    }
    const OrbisNpId self_np_id = Libraries::Np::NpHandler::GetInstance().GetNpId(user_id);
    const OrbisNpLookupTitleCtxId id = g_lookup_next_ctx_id++;
    g_lookup_title_ctxs[id] = RequestTitleCtx{.user_id = user_id, .self_np_id = self_np_id};
    LOG_INFO(Lib_NpUtility, "id={} user_id={}", id, user_id);
    return id;
}

s32 sceNpLookupDeleteTitleCtx(OrbisNpLookupTitleCtxId title_ctx_id) {
    std::lock_guard lock(g_lookup_mutex);
    if (!g_lookup_title_ctxs.contains(title_ctx_id)) {
        LOG_ERROR(Lib_NpUtility, "Invalid title_ctx_id {}", title_ctx_id);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    for (auto it = g_lookup_requests.begin(); it != g_lookup_requests.end();) {
        if (it->second->title_ctx_id == title_ctx_id) {
            it->second->SetResult(ORBIS_NP_COMMUNITY_ERROR_ABORTED);
            it = g_lookup_requests.erase(it);
        } else {
            ++it;
        }
    }
    g_lookup_title_ctxs.erase(title_ctx_id);
    return ORBIS_OK;
}

static s32 CreateLookupRequest(OrbisNpLookupTitleCtxId title_ctx_id, bool is_async) {
    std::lock_guard lock(g_lookup_mutex);
    auto ctx_it = g_lookup_title_ctxs.find(title_ctx_id);
    if (ctx_it == g_lookup_title_ctxs.end()) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    if (static_cast<s32>(g_lookup_requests.size()) >= ORBIS_NP_LOOKUP_MAX_REQUEST_NUM) {
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
    }
    const OrbisNpLookupRequestId id = g_lookup_next_req_id++;
    auto req = std::make_shared<RequestCtx>();
    req->title_ctx_id = title_ctx_id;
    req->is_async = is_async;
    req->timeouts = ctx_it->second.timeouts;
    g_lookup_requests[id] = std::move(req);
    LOG_INFO(Lib_NpUtility, "id={} title_ctx_id={} async={}", id, title_ctx_id, is_async);
    return id;
}

s32 sceNpLookupCreateRequest(OrbisNpLookupTitleCtxId title_ctx_id) {
    return CreateLookupRequest(title_ctx_id, false);
}

s32 sceNpLookupCreateAsyncRequest(OrbisNpLookupTitleCtxId title_ctx_id,
                                  const OrbisNpLookupCreateAsyncRequestParameter* param) {
    return CreateLookupRequest(title_ctx_id, true);
}

s32 sceNpLookupDeleteRequest(OrbisNpLookupRequestId req_id) {
    std::lock_guard lock(g_lookup_mutex);
    auto it = g_lookup_requests.find(req_id);
    if (it == g_lookup_requests.end()) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    {
        std::lock_guard rlock(it->second->mutex);
        it->second->aborted = true;
    }
    it->second->SetResult(ORBIS_NP_COMMUNITY_ERROR_ABORTED);
    g_lookup_requests.erase(it);
    return ORBIS_OK;
}

s32 sceNpLookupAbortRequest(OrbisNpLookupRequestId req_id) {
    std::shared_ptr<RequestCtx> req;
    {
        std::lock_guard lock(g_lookup_mutex);
        auto it = g_lookup_requests.find(req_id);
        if (it == g_lookup_requests.end()) {
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        req = it->second;
    }
    {
        std::lock_guard rlock(req->mutex);
        req->aborted = true;
    }
    req->SetResult(ORBIS_NP_COMMUNITY_ERROR_ABORTED);
    return ORBIS_OK;
}

s32 sceNpLookupNpId(OrbisNpLookupRequestId req_id, const OrbisNpOnlineId* online_id,
                    OrbisNpId* np_id, void* option) {
    LOG_INFO(Lib_NpUtility, "req_id={} online_id='{}' np_id={} option={}", req_id,
             online_id != nullptr ? ViewNpOnlineId(*online_id) : std::string_view("(null)"),
             static_cast<const void*>(np_id), static_cast<const void*>(option));
    if (option != nullptr) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (online_id == nullptr || np_id == nullptr) {
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }

    std::shared_ptr<RequestCtx> req;
    s32 searching_user_id = -1;
    {
        std::lock_guard lock(g_lookup_mutex);
        auto it = g_lookup_requests.find(req_id);
        if (it == g_lookup_requests.end()) {
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        req = it->second;
        if (auto ctx_it = g_lookup_title_ctxs.find(req->title_ctx_id);
            ctx_it != g_lookup_title_ctxs.end()) {
            searching_user_id = ctx_it->second.user_id;
        }
    }
    {
        std::lock_guard rlock(req->mutex);
        if (req->aborted) {
            return ORBIS_NP_COMMUNITY_ERROR_ABORTED;
        }
        if (req->used) {
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_TYPE;
        }
        req->used = true;
    }

    const std::string target_online_id(ViewNpOnlineId(*online_id));
    NpHandler::GetInstance().ResolveOnlineId(
        searching_user_id, target_online_id,
        [req, np_id](s32 result, u64 account_id, const std::string& canonical_online_id) {
            if (result == ORBIS_OK && np_id != nullptr) {
                SetNpId(*np_id, canonical_online_id);
            }
            req->SetResult(result);
        });

    if (!req->is_async) {
        return req->Wait();
    }
    return ORBIS_OK;
}

s32 sceNpLookupPollAsync(OrbisNpLookupRequestId req_id, s32* result) {
    std::shared_ptr<RequestCtx> req;
    {
        std::lock_guard lock(g_lookup_mutex);
        auto it = g_lookup_requests.find(req_id);
        if (it == g_lookup_requests.end()) {
            LOG_ERROR(Lib_NpUtility, "Invalid req_id {}", req_id);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        req = it->second;
    }
    std::lock_guard rlock(req->mutex);
    if (!req->result.has_value()) {
        return ORBIS_NP_LOOKUP_POLL_ASYNC_RET_RUNNING;
    }
    if (result != nullptr) {
        *result = *req->result;
    }
    return ORBIS_OK;
}

s32 sceNpLookupSetTimeout(s32 id, s32 resolve_retry, u32 resolve_timeout, u32 conn_timeout,
                          u32 send_timeout, u32 recv_timeout) {
    if (resolve_retry < 0) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (resolve_timeout != ORBIS_NP_LOOKUP_TIMEOUT_NO_EFFECT && resolve_timeout < 1'000'000) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (conn_timeout != ORBIS_NP_LOOKUP_TIMEOUT_NO_EFFECT && conn_timeout < 10'000'000) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (send_timeout != ORBIS_NP_LOOKUP_TIMEOUT_NO_EFFECT && send_timeout < 10'000'000) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (recv_timeout != ORBIS_NP_LOOKUP_TIMEOUT_NO_EFFECT && recv_timeout < 10'000'000) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (resolve_retry == 0 && resolve_timeout == ORBIS_NP_LOOKUP_TIMEOUT_NO_EFFECT &&
        conn_timeout == ORBIS_NP_LOOKUP_TIMEOUT_NO_EFFECT &&
        send_timeout == ORBIS_NP_LOOKUP_TIMEOUT_NO_EFFECT &&
        recv_timeout == ORBIS_NP_LOOKUP_TIMEOUT_NO_EFFECT) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    const RequestTimeouts timeouts{
        .resolve_retry = resolve_retry,
        .resolve_timeout = resolve_timeout,
        .conn_timeout = conn_timeout,
        .send_timeout = send_timeout,
        .recv_timeout = recv_timeout,
    };

    std::lock_guard lock(g_lookup_mutex);
    if (auto ctx_it = g_lookup_title_ctxs.find(id); ctx_it != g_lookup_title_ctxs.end()) {
        ctx_it->second.timeouts = timeouts;
        return ORBIS_OK;
    }
    if (auto req_it = g_lookup_requests.find(id); req_it != g_lookup_requests.end()) {
        std::lock_guard rlock(req_it->second->mutex);
        req_it->second->timeouts = timeouts;
        return ORBIS_OK;
    }
    return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
}

s32 sceNpLookupWaitAsync(OrbisNpLookupRequestId req_id, s32* result) {
    std::shared_ptr<RequestCtx> req;
    {
        std::lock_guard lock(g_lookup_mutex);
        auto it = g_lookup_requests.find(req_id);
        if (it == g_lookup_requests.end()) {
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        req = it->second;
    }
    const s32 r = req->Wait();
    if (result != nullptr) {
        *result = r;
    }
    return ORBIS_OK;
}

} // namespace Libraries::Np::NpUtility