// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <optional>

#include "core/file_format/nptitle.h"
#include "core/libraries/kernel/ipmi/np_service/np_service.h"

namespace Libraries::Kernel::Ipmi::NpService {

// SceNpAsmClient service, handles methods 0xa0000-0xaffff
class AsmClientService : public Service {
public:
    static constexpr u32 Id = 9;

    AsmClientService();

    s32 InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                         std::span<IpmiBufferInfo> out) override;

private:
    s32 DecryptNpCommPassphrase(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 IsTitleBanned(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 GetServiceBaseUrlCache(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 SetServiceBaseUrlCache(std::span<const IpmiDataInfo> in);
    s32 GetTitleTokenCache(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out,
                           bool with_client_id);
    s32 SetTitleTokenCache(std::span<const IpmiDataInfo> in);
    s32 DeleteTitleTokenCache(std::span<const IpmiDataInfo> in);
    s32 SetNpTitleId(std::span<const IpmiDataInfo> in);
    s32 GetNpTitleId(std::span<IpmiBufferInfo> out);

    struct NpTitle {
        NpTitleFile::TitleId id;
        NpTitleFile::Secret secret;
    };
    std::mutex m_mutex;
    std::optional<NpTitle> m_mini_app_title;
    std::optional<NpTitle> m_big_app_title;
};

} // namespace Libraries::Kernel::Ipmi::NpService
