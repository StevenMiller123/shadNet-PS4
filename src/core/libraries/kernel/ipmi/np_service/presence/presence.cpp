// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/ipmi/np_service/presence/presence.h"
#include "core/libraries/np/np_error.h"

namespace Libraries::Kernel::Ipmi::NpService {

namespace {

enum class Method : u32 {
    StartPresenceRequest = 0x50000,
    GetPresenceRequestResult = 0x50001,
};

struct RequestIdInput {
    s32 request_id;
};

} // namespace

PresenceService::PresenceService() : Service("SceNpPresence", Id, 0x50000, 0x5ffff) {}

s32 PresenceService::InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                                      std::span<IpmiBufferInfo> out) {
    switch (static_cast<Method>(method)) {
    case Method::StartPresenceRequest:
        return StartPresenceRequest(in);
    case Method::GetPresenceRequestResult:
        return GetPresenceRequestResult(in, out);
    default:
        return Service::InvokeSyncMethod(method, in, out);
    }
}

// Used by libSceNpWebApi
s32 PresenceService::StartPresenceRequest(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<RequestIdInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    {
        std::scoped_lock lk{m_mutex};
        m_results[input->request_id] = 0;
    }
    return FinishRequest(input->request_id);
}

// Read by libSceNpWebApi once the request finishes.
s32 PresenceService::GetPresenceRequestResult(std::span<const IpmiDataInfo> in,
                                              std::span<IpmiBufferInfo> out) {
    const auto* input = InputAs<RequestIdInput>(in, 0);
    if (input == nullptr || out.empty()) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    s32 result;
    {
        std::scoped_lock lk{m_mutex};
        const auto it = m_results.find(input->request_id);
        if (it == m_results.end()) {
            return ORBIS_NP_IPC_ERROR_INVALID_REQUEST_ID;
        }
        result = it->second;
    }
    return WriteOutput(out[0], &result, sizeof(result));
}

s32 PresenceService::OnDestroyRequest(s32 request_id) {
    std::scoped_lock lk{m_mutex};
    m_results.erase(request_id);
    return 0;
}

} // namespace Libraries::Kernel::Ipmi::NpService
