// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include "core/libraries/kernel/ipmi/ipmi_server.h"
#include "core/libraries/kernel/ipmi/np_service/np_service.h"

namespace Libraries::Kernel::Ipmi {

// Emulated SceNpService IPMI server
// This encompasses many "Service" objects for library-specific requests.
class NpServiceServer : public Server {
public:
    static constexpr std::string_view ServiceName = "SceNpService";

    explicit NpServiceServer(void* user_data);
    ~NpServiceServer() override;

    s32 InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                         std::span<IpmiBufferInfo> out, s32* method_result) override;
    s32 InvokeAsyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                          s32* method_result) override;

private:
    static constexpr u64 MaxOpenServices = 32;

    s32 OpenService(std::span<const IpmiDataInfo> in);
    s32 CloseService(std::span<const IpmiDataInfo> in);
    s32 CreateRequest(std::span<const IpmiDataInfo> in);
    s32 StartRequest(std::span<const IpmiDataInfo> in);
    s32 DestroyRequest(std::span<const IpmiDataInfo> in);
    s32 AbortRequest(std::span<const IpmiDataInfo> in);

    NpService::Service* FindService(u32 id);
    NpService::Service* FindOpenService(u32 method);
    NpService::Service* FindOpenServiceById(u32 id);

    std::vector<std::unique_ptr<NpService::Service>> m_services;
    std::mutex m_mutex;
    std::set<u32> m_open_services;
};

} // namespace Libraries::Kernel::Ipmi
