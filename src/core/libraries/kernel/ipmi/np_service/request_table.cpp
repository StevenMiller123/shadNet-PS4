// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/ipmi/np_service/request_table.h"
#include "core/libraries/np/np_error.h"

namespace Libraries::Kernel::Ipmi::NpService {

namespace {

// Each IPMI event flag holds 64 slots.
constexpr u32 SlotsPerEventFlag = 64;

} // namespace

RequestTable::Request* RequestTable::Find(s32 request_id) {
    for (Request& request : m_requests) {
        if (request.state != State::Dormant && request.id == request_id) {
            return &request;
        }
    }
    return nullptr;
}

s32 RequestTable::Create(s32 request_id) {
    if (request_id == InvalidId) {
        return ORBIS_NP_IPC_ERROR_INTERNAL_INVALID_REQUEST_ID_ASSIGNED;
    }

    std::scoped_lock lk{m_mutex};
    if (Find(request_id) != nullptr) {
        return ORBIS_NP_IPC_ERROR_INTERNAL_INVALID_REQUEST_ID_ASSIGNED;
    }
    for (Request& request : m_requests) {
        if (request.state == State::Dormant) {
            request = {State::Created, request_id, InvalidId};
            return 0;
        }
    }
    return ORBIS_NP_IPC_ERROR_TOO_MANY_REQUEST;
}

s32 RequestTable::Start(s32 request_id, s32 ipc_slot) {
    std::scoped_lock lk{m_mutex};
    Request* request = Find(request_id);
    if (request == nullptr) {
        return ORBIS_NP_IPC_ERROR_INVALID_REQUEST_ID;
    }
    if (request->state != State::Created) {
        return ORBIS_NP_IPC_ERROR_INTERNAL_INVALID_REQUEST_CONTEXT_STATE_START;
    }
    request->state = State::Started;
    request->ipc_slot = ipc_slot;
    return 0;
}

s32 RequestTable::Abort(s32 request_id) {
    std::scoped_lock lk{m_mutex};
    Request* request = Find(request_id);
    if (request == nullptr) {
        return ORBIS_NP_IPC_ERROR_INVALID_REQUEST_ID;
    }
    switch (request->state) {
    case State::Aborting:
    case State::Finished:
        return 0;
    case State::Created:
        return ORBIS_NP_IPC_ERROR_INTERNAL_INVALID_REQUEST_CONTEXT_STATE_ABORT;
    default:
        request->state = State::Aborting;
        return 0;
    }
}

s32 RequestTable::Destroy(s32 request_id) {
    std::scoped_lock lk{m_mutex};
    Request* request = Find(request_id);
    if (request == nullptr) {
        return ORBIS_NP_IPC_ERROR_INVALID_REQUEST_ID;
    }
    *request = {};
    return 0;
}

s32 RequestTable::Finish(s32 request_id, Signal* signal) {
    std::scoped_lock lk{m_mutex};
    Request* request = Find(request_id);
    if (request == nullptr) {
        return ORBIS_NP_IPC_ERROR_INVALID_REQUEST_ID;
    }
    if (request->state != State::Started && request->state != State::Aborting) {
        return ORBIS_NP_IPC_ERROR_INTERNAL_INVALID_REQUEST_CONTEXT_STATE_FINISH;
    }
    request->state = State::Finished;
    if (signal != nullptr) {
        signal->index = static_cast<u32>(request->ipc_slot) / SlotsPerEventFlag;
        signal->pattern = u64{1} << (static_cast<u32>(request->ipc_slot) % SlotsPerEventFlag);
    }
    return 0;
}

std::optional<RequestTable::State> RequestTable::GetState(s32 request_id) {
    std::scoped_lock lk{m_mutex};
    const Request* request = Find(request_id);
    if (request == nullptr) {
        return std::nullopt;
    }
    return request->state;
}

} // namespace Libraries::Kernel::Ipmi::NpService
