// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include <vector>

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/types.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_manager/np_manager.h"

namespace Libraries::Np::NpManager {

// Internal types for storing request-related information
enum class NpRequestState {
    None = 0,
    Ready = 1,
    Aborted = 2,
    Complete = 3,
};

struct NpRequest {
    NpRequestState state;
    bool async;
    s32 result;
};

static s32 g_active_requests = 0;
static std::mutex g_request_mutex;
static std::vector<NpRequest> g_requests;

static s32 CreateNpRequest(bool async) {
    std::scoped_lock lk{g_request_mutex};

    if (g_active_requests == ORBIS_NP_MANAGER_REQUEST_LIMIT) {
        return ORBIS_NP_ERROR_REQUEST_MAX;
    }

    s32 req_index = 0;
    while (req_index < static_cast<s32>(g_requests.size())) {
        if (g_requests[req_index].state == NpRequestState::None) {
            g_requests[req_index].state = NpRequestState::Ready;
            g_requests[req_index].async = async;
            break;
        }
        req_index++;
    }

    if (req_index == static_cast<s32>(g_requests.size())) {
        NpRequest new_request{NpRequestState::Ready, async, 0};
        g_requests.emplace_back(new_request);
    }

    g_active_requests++;
    return req_index + ORBIS_NP_MANAGER_REQUEST_ID_OFFSET + 1;
}

static NpRequest* GetRequest(s32 req_id, s32* out_err) {
    if (req_id == 0) {
        *out_err = ORBIS_NP_ERROR_INVALID_ARGUMENT;
        return nullptr;
    }
    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || req_index < 0 ||
        req_index >= static_cast<s32>(g_requests.size()) ||
        g_requests[req_index].state == NpRequestState::None) {
        *out_err = ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
        return nullptr;
    }
    auto& req = g_requests[req_index];
    if (req.state == NpRequestState::Complete) {
        req.result = ORBIS_NP_ERROR_INVALID_ARGUMENT;
        *out_err = ORBIS_NP_ERROR_INVALID_ARGUMENT;
        return nullptr;
    }
    if (req.state == NpRequestState::Aborted) {
        req.result = ORBIS_NP_ERROR_ABORTED;
        *out_err = ORBIS_NP_ERROR_ABORTED;
        return nullptr;
    }
    return &req;
}

static s32 CompleteRequest(NpRequest& req, s32 result) {
    req.state = NpRequestState::Complete;
    req.result = result;
    if (result != ORBIS_OK && req.async) {
        return ORBIS_OK;
    }
    return result;
}

s32 sceNpCreateRequest() {
    LOG_DEBUG(Lib_NpManager, "called");
    return CreateNpRequest(false);
}

s32 sceNpCreateAsyncRequest(const OrbisNpCreateAsyncRequestParameter* param) {
    LOG_DEBUG(Lib_NpManager, "called");
    if (param == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    if (param->size != sizeof(OrbisNpCreateAsyncRequestParameter)) {
        return ORBIS_NP_ERROR_INVALID_SIZE;
    }

    return CreateNpRequest(true);
}

s32 sceNpCheckNpAvailability(s32 req_id, OrbisNpOnlineId* online_id) {
    if (online_id == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    const s32 user_id = NpHandler::GetInstance().GetUserIdByOnlineId(*online_id);
    if (user_id == -1) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    return sceNpCheckNpAvailabilityA(req_id, user_id);
}

s32 sceNpCheckNpAvailabilityA(s32 req_id, UserService::OrbisUserServiceUserId user_id) {
    if (user_id == -1) {
        LOG_ERROR(Lib_NpManager, "invalid user_id {}", user_id);
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{g_request_mutex};
    s32 err;
    NpRequest* req = GetRequest(req_id, &err);
    if (!req) {
        return err;
    }
    if (!NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return CompleteRequest(*req, ORBIS_NP_ERROR_SIGNED_OUT);
    }
    LOG_DEBUG(Lib_NpManager, "req_id = {:#x}, user_id = {}", req_id, user_id);
    return CompleteRequest(*req, ORBIS_OK);
}

s32 sceNpCheckNpReachability(s32 req_id, UserService::OrbisUserServiceUserId user_id) {
    if (user_id == -1) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{g_request_mutex};
    s32 err;
    NpRequest* req = GetRequest(req_id, &err);
    if (!req) {
        return err;
    }
    if (!NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return CompleteRequest(*req, ORBIS_NP_ERROR_SIGNED_OUT);
    }
    LOG_DEBUG(Lib_NpManager, "req_id = {:#x}, user_id = {}", req_id, user_id);
    return CompleteRequest(*req, ORBIS_OK);
}

s32 sceNpCheckPlus(s32 req_id, const OrbisNpCheckPlusParameter* param,
                   OrbisNpCheckPlusResult* result) {
    if (req_id == 0 || param == nullptr || result == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (param->size != sizeof(OrbisNpCheckPlusParameter)) {
        return ORBIS_NP_ERROR_INVALID_SIZE;
    }
    if (param->user_id == -1) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (param->features < 1 || param->features > 3) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (param->features != 1 && GetFirmwareVersion() >= Common::ElfInfo::FW_350) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    for (u8 b : param->reserved) {
        if (b != 0) {
            return ORBIS_NP_ERROR_INVALID_ARGUMENT;
        }
    }

    std::scoped_lock lk{g_request_mutex};
    s32 err;
    NpRequest* req = GetRequest(req_id, &err);
    if (!req) {
        return err;
    }
    if (!NpHandler::GetInstance().IsPsnSignedIn(param->user_id)) {
        return CompleteRequest(*req, ORBIS_NP_ERROR_SIGNED_OUT);
    }

    LOG_DEBUG(Lib_NpManager, "req_id = {:#x}, features = {:#x}", req_id, param->features);
    result->authorized = true;
    return CompleteRequest(*req, ORBIS_OK);
}

s32 sceNpGetAccountLanguage(s32 req_id, OrbisNpOnlineId* online_id, OrbisNpLanguageCode* language) {
    if (online_id == nullptr || language == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    const s32 user_id = NpHandler::GetInstance().GetUserIdByOnlineId(*online_id);
    if (user_id == -1) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    return sceNpGetAccountLanguageA(req_id, user_id, language);
}

s32 sceNpGetAccountLanguageA(s32 req_id, UserService::OrbisUserServiceUserId user_id,
                             OrbisNpLanguageCode* language) {
    if (language == nullptr || user_id == -1) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{g_request_mutex};
    s32 err;
    NpRequest* req = GetRequest(req_id, &err);
    if (!req) {
        return err;
    }
    if (!NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return CompleteRequest(*req, ORBIS_NP_ERROR_SIGNED_OUT);
    }
    LOG_DEBUG(Lib_NpManager, "req_id = {:#x}, user_id = {}", req_id, user_id);
    // TODO: Retrieve from console?
    std::memcpy(language->code, "en", 2);
    return CompleteRequest(*req, ORBIS_OK);
}

s32 sceNpGetParentalControlInfo(s32 req_id, OrbisNpOnlineId* online_id, s8* age,
                                OrbisNpParentalControlInfo* info) {
    if (online_id == nullptr || age == nullptr || info == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    const s32 user_id = NpHandler::GetInstance().GetUserIdByOnlineId(*online_id);
    if (user_id == -1) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    return sceNpGetParentalControlInfoA(req_id, user_id, age, info);
}

s32 sceNpGetParentalControlInfoA(s32 req_id, UserService::OrbisUserServiceUserId user_id, s8* age,
                                 OrbisNpParentalControlInfo* info) {
    if (age == nullptr || info == nullptr || user_id == -1) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{g_request_mutex};
    s32 err;
    NpRequest* req = GetRequest(req_id, &err);
    if (!req) {
        return err;
    }
    if (!NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return CompleteRequest(*req, ORBIS_NP_ERROR_SIGNED_OUT);
    }
    LOG_DEBUG(Lib_NpManager, "req_id = {:#x}, user_id = {}", req_id, user_id);
    // TODO: Retrieve from console?
    *age = 18;
    std::memset(info, 0, sizeof(OrbisNpParentalControlInfo));
    return CompleteRequest(*req, ORBIS_OK);
}

s32 sceNpAbortRequest(s32 req_id) {
    LOG_DEBUG(Lib_NpManager, "called req_id = {:#x}", req_id);

    std::scoped_lock lk{g_request_mutex};

    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || req_index < 0 ||
        req_index >= static_cast<s32>(g_requests.size()) ||
        g_requests[req_index].state == NpRequestState::None) {
        return ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
    }

    if (g_requests[req_index].state == NpRequestState::Complete) {
        // If the request is already complete, abort is ignored.
        return ORBIS_OK;
    }

    g_requests[req_index].state = NpRequestState::Aborted;
    return ORBIS_OK;
}

s32 sceNpSetTimeout(s32 req_id, s32 resolve_retry, u32 resolve_timeout, u32 conn_timeout,
                    u32 send_timeout, u32 recv_timeout) {
    LOG_DEBUG(Lib_NpManager, "called req_id = {:#x}", req_id);

    if (req_id <= 0) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    const auto unset_or_at_least = [](u32 v, u32 min_us) { return v == 0 || v >= min_us; };
    const bool all_zero = resolve_retry == 0 && resolve_timeout == 0 && conn_timeout == 0 &&
                          send_timeout == 0 && recv_timeout == 0;
    if (all_zero || resolve_retry < 0 || !unset_or_at_least(resolve_timeout, 1'000'000) ||
        !unset_or_at_least(conn_timeout, 10'000'000) ||
        !unset_or_at_least(send_timeout, 10'000'000) ||
        !unset_or_at_least(recv_timeout, 10'000'000)) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{g_request_mutex};

    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || req_index < 0 ||
        req_index >= static_cast<s32>(g_requests.size()) ||
        g_requests[req_index].state == NpRequestState::None) {
        return ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
    }

    // Since we're not actually performing any sort of network request here,
    // we can just return after validating our parameters.
    return ORBIS_OK;
}

s32 sceNpWaitAsync(s32 req_id, s32* result) {
    if (result == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{g_request_mutex};

    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || req_index < 0 ||
        req_index >= static_cast<s32>(g_requests.size()) ||
        g_requests[req_index].state == NpRequestState::None) {
        return ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
    }

    if (!g_requests[req_index].async || g_requests[req_index].state == NpRequestState::Ready) {
        return ORBIS_NP_ERROR_INVALID_ID;
    }

    // Since we're not actually performing any sort of network request here,
    // we can just set result based on the request and return.
    *result = g_requests[req_index].result;
    LOG_WARNING(Lib_NpManager, "called req_id = {:#x}, returning result = {:#x}", req_id,
                static_cast<u32>(*result));
    return ORBIS_OK;
}

s32 sceNpPollAsync(s32 req_id, s32* result) {
    if (result == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{g_request_mutex};

    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || req_index < 0 ||
        req_index >= static_cast<s32>(g_requests.size()) ||
        g_requests[req_index].state == NpRequestState::None) {
        return ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
    }

    if (!g_requests[req_index].async || g_requests[req_index].state == NpRequestState::Ready) {
        return ORBIS_NP_ERROR_INVALID_ID;
    }

    // Since we're not actually performing any sort of network request here,
    // we can just set result based on the request and return.
    *result = g_requests[req_index].result;
    LOG_WARNING(Lib_NpManager, "called req_id = {:#x}, returning result = {:#x}", req_id,
                static_cast<u32>(*result));
    return ORBIS_OK;
}

s32 sceNpDeleteRequest(s32 req_id) {
    LOG_DEBUG(Lib_NpManager, "called req_id = {:#x}", req_id);

    std::scoped_lock lk{g_request_mutex};

    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || req_index < 0 ||
        req_index >= static_cast<s32>(g_requests.size()) ||
        g_requests[req_index].state == NpRequestState::None) {
        return ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
    }

    g_active_requests--;
    g_requests[req_index].state = NpRequestState::None;
    return ORBIS_OK;
}

} // namespace Libraries::Np::NpManager