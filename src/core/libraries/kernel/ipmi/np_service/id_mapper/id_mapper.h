// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "core/libraries/kernel/ipmi/np_service/np_service.h"
#include "core/libraries/np/np_types.h"

namespace Libraries::Kernel::Ipmi::NpService {

// SceNpIdMapper service, handles methods 0xc0000-0xcffff
class IdMapperService : public Service {
public:
    static constexpr u32 Id = 11;

    IdMapperService();

    s32 InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                         std::span<IpmiBufferInfo> out) override;

protected:
    s32 OnAbortRequest(s32 request_id) override;
    s32 OnDestroyRequest(s32 request_id) override;

private:
    struct Lookup {
        std::vector<Np::OrbisNpOnlineId> online_ids;
        std::vector<u64> account_ids;
        u64 remaining = 0;
        s32 result = 0;
    };

    s32 AddOnlineIds(std::span<const IpmiDataInfo> in);
    s32 StartOnlineIdToAccountId(std::span<const IpmiDataInfo> in);
    s32 GetAccountIdResult(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);

    // Called through NpHandler when lookup requests complete
    void OnAccountIdResolved(s32 request_id, u64 index, s32 result, u64 account_id);

    std::mutex m_mutex;
    std::map<s32, Lookup> m_lookups;
    std::shared_ptr<IdMapperService*> m_self = std::make_shared<IdMapperService*>(this);
};

} // namespace Libraries::Kernel::Ipmi::NpService
