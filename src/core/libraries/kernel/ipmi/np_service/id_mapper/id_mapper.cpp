// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>

#include "common/logging/log.h"
#include "core/libraries/kernel/ipmi/np_service/id_mapper/id_mapper.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"

namespace Libraries::Kernel::Ipmi::NpService {

using Np::NpHandler;
using Np::OrbisNpOnlineId;

namespace {

enum class Method : u32 {
    AddOnlineIds = 0xc0001,
    StartOnlineIdToAccountId = 0xc0005,
    GetAccountIdResult = 0xc0006,
};

struct AddOnlineIdsInput {
    s32 request_id;
    s32 count;
};

struct StartInput {
    s32 request_id;
    s32 user_id;
    u32 unknown;
};

struct GetResultInput {
    s32 request_id;
    s32 count;
    u8 unknown;
};

constexpr u64 MaxOnlineIds = 1000;

} // namespace

IdMapperService::IdMapperService() : Service("SceNpIdMapper", Id, 0xc0000, 0xcffff) {}

s32 IdMapperService::InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                                      std::span<IpmiBufferInfo> out) {
    switch (static_cast<Method>(method)) {
    case Method::AddOnlineIds:
        return AddOnlineIds(in);
    case Method::StartOnlineIdToAccountId:
        return StartOnlineIdToAccountId(in);
    case Method::GetAccountIdResult:
        return GetAccountIdResult(in, out);
    default:
        return Service::InvokeSyncMethod(method, in, out);
    }
}

// sceNpIdMapperOnlineIdToAccountId
s32 IdMapperService::AddOnlineIds(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<AddOnlineIdsInput>(in, 0);
    if (input == nullptr || input->count <= 0 || in.size() < 2 || in[1].data == nullptr ||
        in[1].size < input->count * sizeof(OrbisNpOnlineId)) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    const auto* online_ids = static_cast<const OrbisNpOnlineId*>(in[1].data);
    std::scoped_lock lk{m_mutex};
    Lookup& lookup = m_lookups[input->request_id];
    if (lookup.online_ids.size() + input->count > MaxOnlineIds) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    lookup.online_ids.insert(lookup.online_ids.end(), online_ids, online_ids + input->count);
    return 0;
}

// sceNpIdMapperOnlineIdToAccountId
s32 IdMapperService::StartOnlineIdToAccountId(std::span<const IpmiDataInfo> in) {
    // const auto* np_title_id = in[1], 0x10 bytes
    // const auto* np_title_secret = in[2], 0x410 bytes
    const auto* input = InputAs<StartInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::vector<std::string> online_ids;
    {
        std::scoped_lock lk{m_mutex};
        Lookup& lookup = m_lookups[input->request_id];
        lookup.account_ids.assign(lookup.online_ids.size(), 0);
        lookup.remaining = lookup.online_ids.size();
        lookup.result = 0;
        for (const OrbisNpOnlineId& online_id : lookup.online_ids) {
            online_ids.emplace_back(Np::ViewNpOnlineId(online_id));
        }
    }
    if (online_ids.empty()) {
        return FinishRequest(input->request_id);
    }

    for (u64 i = 0; i < online_ids.size(); i++) {
        // Handle the request asynchronously, with a callback to mark the request as complete.
        NpHandler::Instance().ResolveOnlineId(
            input->user_id, online_ids[i],
            [self = std::weak_ptr{m_self}, request_id = input->request_id,
             i](s32 result, u64 account_id, const std::string&) {
                if (const auto service = self.lock()) {
                    (*service)->OnAccountIdResolved(request_id, i, result, account_id);
                }
            });
    }
    return 0;
}

void IdMapperService::OnAccountIdResolved(s32 request_id, u64 index, s32 result, u64 account_id) {
    {
        std::scoped_lock lk{m_mutex};
        const auto it = m_lookups.find(request_id);
        if (it == m_lookups.end() || index >= it->second.account_ids.size() ||
            it->second.remaining == 0) {
            return;
        }
        Lookup& lookup = it->second;
        if (result < 0) {
            LOG_WARNING(Kernel_Ipmi, "{} can't find account id for {} ({:#x})", GetName(),
                        Np::ViewNpOnlineId(lookup.online_ids[index]), result);
            lookup.result = ORBIS_NP_ID_MAPPER_ERROR_ONLINE_ID_NOT_FOUND;
        } else {
            lookup.account_ids[index] = account_id;
        }
        if (--lookup.remaining != 0) {
            return;
        }
    }
    FinishRequest(request_id);
}

// sceNpIdMapperOnlineIdToAccountId
s32 IdMapperService::GetAccountIdResult(std::span<const IpmiDataInfo> in,
                                        std::span<IpmiBufferInfo> out) {
    const auto* input = InputAs<GetResultInput>(in, 0);
    if (input == nullptr || out.size() < 2) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{m_mutex};
    const auto it = m_lookups.find(input->request_id);
    if (it == m_lookups.end()) {
        return ORBIS_NP_IPC_ERROR_INVALID_REQUEST_ID;
    }
    const Lookup& lookup = it->second;
    const u64 count = std::min<u64>(std::max(input->count, 0), lookup.account_ids.size());
    WriteOutput(out[0], lookup.account_ids.data(), count * sizeof(u64));
    return WriteOutput(out[1], &lookup.result, sizeof(lookup.result));
}

s32 IdMapperService::OnAbortRequest(s32 request_id) {
    {
        std::scoped_lock lk{m_mutex};
        const auto it = m_lookups.find(request_id);
        if (it == m_lookups.end() || it->second.remaining == 0) {
            return 0;
        }
        // Later shadNet replies are ignored.
        it->second.remaining = 0;
        it->second.result = ORBIS_NP_ID_MAPPER_ERROR_ABORTED;
    }
    return FinishRequest(request_id);
}

s32 IdMapperService::OnDestroyRequest(s32 request_id) {
    std::scoped_lock lk{m_mutex};
    m_lookups.erase(request_id);
    return 0;
}

} // namespace Libraries::Kernel::Ipmi::NpService
