// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <mutex>

#include "core/libraries/kernel/ipmi/np_service/np_service.h"

namespace Libraries::Kernel::Ipmi::NpService {

// SceNpTpip service, handles methods 0xd0000-0xdffff
class TpipService : public Service {
public:
    static constexpr u32 Id = 12;

    TpipService();

    s32 InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                         std::span<IpmiBufferInfo> out) override;

protected:
    s32 OnDestroyRequest(s32 request_id) override;

private:
    s32 SendCheckPlusApiErrorLog(std::span<const IpmiDataInfo> in);
    s32 BeginCheckPlus(std::span<const IpmiDataInfo> in);
    s32 GetCheckPlusResult(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);

    std::mutex m_mutex;
    std::map<s32, bool> m_check_plus_results;
};

} // namespace Libraries::Kernel::Ipmi::NpService
