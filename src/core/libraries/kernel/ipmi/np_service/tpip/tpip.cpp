// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/ipmi/np_service/tpip/tpip.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_types.h"

namespace Libraries::Kernel::Ipmi::NpService {

namespace {

enum class Method : u32 {
    SendCheckPlusApiErrorLog = 0xd0002,
    BeginCheckPlus = 0xd0003,
    GetCheckPlusResult = 0xd0004,
};

struct BeginCheckPlusInput {
    s32 request_id;
    s32 user_id;
    u64 features;
};

struct RequestIdInput {
    s32 request_id;
};

struct CheckPlusApiErrorLogInput {
    s32 user_id;
    u32 padding;
    u64 unk;
    s32 error;
};

} // namespace

TpipService::TpipService() : Service("SceNpTpip", Id, 0xd0000, 0xdffff) {}

s32 TpipService::InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                                  std::span<IpmiBufferInfo> out) {
    switch (static_cast<Method>(method)) {
    case Method::SendCheckPlusApiErrorLog:
        return SendCheckPlusApiErrorLog(in);
    case Method::BeginCheckPlus:
        return BeginCheckPlus(in);
    case Method::GetCheckPlusResult:
        return GetCheckPlusResult(in, out);
    default:
        return Service::InvokeSyncMethod(method, in, out);
    }
}

// sceNpCheckPlus
s32 TpipService::SendCheckPlusApiErrorLog(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<CheckPlusApiErrorLogInput>(in, 0);
    if (input == nullptr || input->user_id == -1 || in.size() < 2 || in[1].data == nullptr ||
        input->error >= 0) {
        return ORBIS_NP_PLUS_LOGGER_ERROR_INVALID_ARGUMENT;
    }
    return 0;
}

// sceNpCheckPlus
s32 TpipService::BeginCheckPlus(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<BeginCheckPlusInput>(in, 0);
    const auto* title_id = InputAs<Np::OrbisNpTitleId>(in, 1);
    const auto* title_token = InputAs<Np::OrbisNpTitleToken>(in, 2);
    if (input == nullptr || input->request_id <= 0 || input->user_id == -1 || title_id == nullptr ||
        title_token == nullptr) {
        return ORBIS_NP_PLUS_LOGGER_ERROR_INVALID_ARGUMENT;
    }
    {
        std::scoped_lock lk{m_mutex};
        m_check_plus_results[input->request_id] = true;
    }
    return FinishRequest(input->request_id);
}

// sceNpCheckPlus
s32 TpipService::GetCheckPlusResult(std::span<const IpmiDataInfo> in,
                                    std::span<IpmiBufferInfo> out) {
    const auto* input = InputAs<RequestIdInput>(in, 0);
    if (input == nullptr || input->request_id <= 0 || out.empty()) {
        return ORBIS_NP_PLUS_LOGGER_ERROR_INVALID_ARGUMENT;
    }
    bool authorized;
    {
        std::scoped_lock lk{m_mutex};
        const auto it = m_check_plus_results.find(input->request_id);
        if (it == m_check_plus_results.end()) {
            return ORBIS_NP_IPC_ERROR_INVALID_REQUEST_ID;
        }
        authorized = it->second;
    }
    return WriteOutput(out[0], &authorized, sizeof(authorized));
}

s32 TpipService::OnDestroyRequest(s32 request_id) {
    std::scoped_lock lk{m_mutex};
    m_check_plus_results.erase(request_id);
    return 0;
}

} // namespace Libraries::Kernel::Ipmi::NpService
