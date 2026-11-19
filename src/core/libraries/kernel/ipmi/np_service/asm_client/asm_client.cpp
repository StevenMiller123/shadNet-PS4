// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/kernel/ipmi/np_service/asm_client/asm_client.h"
#include "core/libraries/kernel/ipmi/np_service/asm_client/service_base_url_cache.h"
#include "core/libraries/kernel/ipmi/np_service/asm_client/title_token_cache.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/system_service/system_service.h"

namespace Libraries::Kernel::Ipmi::NpService {

using Np::OrbisNpAccountId;
using Np::OrbisNpClientId;
using Np::OrbisNpCommunicationId;
using Np::OrbisNpCommunicationPassphrase;
using Np::OrbisNpTitleId;
using Np::OrbisNpTitleSecret;
using Np::OrbisNpTitleToken;
using SystemService::OrbisLncAppType;

namespace {

enum class Method : u32 {
    DecryptNpCommPassphrase = 0xa0000,
    IsTitleBanned = 0xa0004,
    SetNpTitleId = 0xa0005,
    GetNpTitleId = 0xa0006,
    GetServiceBaseUrlCache = 0xa0007,
    SetServiceBaseUrlCache = 0xa0008,
    GetTitleTokenCache = 0xa0009,
    GetTitleTokenCacheWithClientId = 0xa000a,
    SetTitleTokenCache = 0xa000b,
    DeleteTitleTokenCache = 0xa000c,
};

// A list of firmware title ids which bypass title id checks
constexpr std::array<const char*, 4> SystemTitleIds = {"NPXS29996", "NPXS29997", "NPXS29998",
                                                       "NPXS29999"};
constexpr u64 TitleIdCompareLength = 9;

std::optional<std::string_view> StringInput(std::span<const IpmiDataInfo> in, u64 index) {
    if (index >= in.size() || in[index].data == nullptr) {
        return std::nullopt;
    }
    const char* comm_id = static_cast<const char*>(in[index].data);
    return std::string_view{comm_id, strnlen(comm_id, in[index].size)};
}

struct NpCommInfo {
    OrbisNpCommunicationId comm_id;
    u8 encrypted_passphrase[0x100];
    u64 encrypted_passphrase_size;
    u8 passphrase_iv[0x20];
    u64 passphrase_iv_size;
    s32 service_label;
    u8 padding[4];
};
static_assert(sizeof(NpCommInfo) == 0x148);

struct NpCommPassphraseInfo {
    OrbisNpCommunicationId comm_id;
    OrbisNpCommunicationPassphrase passphrase;
    s32 service_label;
};
static_assert(sizeof(NpCommPassphraseInfo) == 0x90);

struct SetNpTitleIdInput {
    OrbisLncAppType app_type;
};

} // namespace

AsmClientService::AsmClientService() : Service("SceNpAsmClient", Id, 0xa0000, 0xaffff) {}

s32 AsmClientService::InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                                       std::span<IpmiBufferInfo> out) {
    switch (static_cast<Method>(method)) {
    case Method::DecryptNpCommPassphrase:
        return DecryptNpCommPassphrase(in, out);
    case Method::IsTitleBanned:
        return IsTitleBanned(in, out);
    case Method::SetNpTitleId:
        return SetNpTitleId(in);
    case Method::GetNpTitleId:
        return GetNpTitleId(out);
    case Method::GetServiceBaseUrlCache:
        return GetServiceBaseUrlCache(in, out);
    case Method::SetServiceBaseUrlCache:
        return SetServiceBaseUrlCache(in);
    case Method::GetTitleTokenCache:
        return GetTitleTokenCache(in, out, false);
    case Method::GetTitleTokenCacheWithClientId:
        return GetTitleTokenCache(in, out, true);
    case Method::SetTitleTokenCache:
        return SetTitleTokenCache(in);
    case Method::DeleteTitleTokenCache:
        return DeleteTitleTokenCache(in);
    default:
        return Service::InvokeSyncMethod(method, in, out);
    }
}

// Used by sceNpAsmClientGetNpComInfo variants to decrypt what PSN servers return.
// We aren't encrypting anything, so this can just copy the input to the output.
s32 AsmClientService::DecryptNpCommPassphrase(std::span<const IpmiDataInfo> in,
                                              std::span<IpmiBufferInfo> out) {
    const auto* info = InputAs<NpCommInfo>(in, 0);
    const auto* secret = InputAs<OrbisNpTitleSecret>(in, 1);
    if (info == nullptr || secret == nullptr || out.empty() ||
        info->encrypted_passphrase_size > sizeof(info->encrypted_passphrase)) {
        return ORBIS_NP_ASM_CLIENT_ERROR_INVALID_ARGUMENT;
    }

    NpCommPassphraseInfo result{};
    std::memcpy(result.passphrase.data, info->encrypted_passphrase,
                std::min<u64>(info->encrypted_passphrase_size, sizeof(result.passphrase.data)));
    return WriteOutput(out[0], &result, sizeof(result));
}

// Checked during various ASM client requests
s32 AsmClientService::IsTitleBanned(std::span<const IpmiDataInfo> in,
                                    std::span<IpmiBufferInfo> out) {
    if (InputAs<OrbisNpTitleId>(in, 0) == nullptr || out.empty()) {
        return ORBIS_NP_ASM_CLIENT_ERROR_INVALID_ARGUMENT;
    }
    const bool banned = false;
    return WriteOutput(out[0], &banned, sizeof(banned));
}

// sceNpAsmClientSetNpTitleId
s32 AsmClientService::SetNpTitleId(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<SetNpTitleIdInput>(in, 0);
    const auto* id = InputAs<NpTitleFile::TitleId>(in, 1);
    const auto* secret = InputAs<NpTitleFile::Secret>(in, 2);
    if (input == nullptr || id == nullptr || secret == nullptr) {
        return ORBIS_NP_ASM_CLIENT_ERROR_INVALID_ARGUMENT;
    }
    // libSceNpManager only sets title ids for games and mini apps.
    if (input->app_type != OrbisLncAppType::MiniApp && input->app_type != OrbisLncAppType::BigApp) {
        return ORBIS_NP_ASM_CLIENT_ERROR_INTERNAL_ERROR;
    }

    std::scoped_lock lk{m_mutex};
    auto& stored = input->app_type == OrbisLncAppType::MiniApp ? m_mini_app_title : m_big_app_title;
    stored = NpTitle{*id, *secret};
    return 0;
}

// sceNpAsmClientGetNpTitleId
s32 AsmClientService::GetNpTitleId(std::span<IpmiBufferInfo> out) {
    if (out.size() < 2) {
        return ORBIS_NP_ASM_CLIENT_ERROR_INVALID_ARGUMENT;
    }

    // SceShellCore reads the title's nptitle.dat from its appmeta, loaded at startup.
    const std::optional<NpTitleFile>& title = Common::ElfInfo::Instance().GetNpTitle();
    if (!title) {
        LOG_ERROR(Kernel_Ipmi, "{} found no nptitle.dat", GetName());
        return ORBIS_NP_ERROR_NP_TITLE_DAT_NOT_FOUND;
    }
    if (!title->IsValid()) {
        LOG_ERROR(Kernel_Ipmi, "{} found an invalid nptitle.dat", GetName());
        return ORBIS_NP_ASM_CLIENT_ERROR_INVALID_NPTITLE_DAT;
    }

    const std::string_view serial = Common::ElfInfo::Instance().GameSerial();
    if (serial.empty()) {
        return ORBIS_NP_ERROR_TITLE_ID_IN_PARAM_SFO_NOT_EXIST;
    }
    const bool system_title = std::any_of(SystemTitleIds.begin(), SystemTitleIds.end(),
                                          [&](const char* id) { return serial.starts_with(id); });
    if (!system_title && std::strncmp(serial.data(), title->GetTitleId().data(),
                                      std::min<u64>(serial.size(), TitleIdCompareLength)) != 0) {
        return ORBIS_NP_ERROR_TITLE_ID_IN_PARAM_SFO_NOT_MATCHED_TO_NP_TITLE_ID;
    }

    const NpTitleFile::TitleId& id = title->GetTitleId();
    const NpTitleFile::Secret& secret = title->GetSecret();
    WriteOutput(out[0], id.data(), id.size());
    return WriteOutput(out[1], secret.data(), secret.size());
}

s32 AsmClientService::GetTitleTokenCache(std::span<const IpmiDataInfo> in,
                                         std::span<IpmiBufferInfo> out, bool with_client_id) {
    const auto* account_id = InputAs<OrbisNpAccountId>(in, 0);
    const auto comm_id = StringInput(in, 1);
    if (account_id == nullptr || !comm_id || out.size() < (with_client_id ? 3u : 2u)) {
        return ORBIS_NP_ASM_CLIENT_ERROR_INVALID_ARGUMENT;
    }

    OrbisNpTitleToken token{};
    OrbisNpClientId client_id{};
    const bool found = TitleTokenCache::Instance().Find(*account_id, *comm_id, &token, &client_id);
    WriteOutput(out[0], &found, sizeof(found));
    WriteOutput(out[1], &token, sizeof(token));
    if (with_client_id) {
        WriteOutput(out[2], &client_id, sizeof(client_id));
    }
    return 0;
}

s32 AsmClientService::SetTitleTokenCache(std::span<const IpmiDataInfo> in) {
    const auto* account_id = InputAs<OrbisNpAccountId>(in, 0);
    const auto comm_id = StringInput(in, 1);
    const auto* token = InputAs<OrbisNpTitleToken>(in, 2);
    const auto* client_id = InputAs<OrbisNpClientId>(in, 3);
    if (account_id == nullptr || !comm_id || token == nullptr || client_id == nullptr) {
        return ORBIS_NP_ASM_CLIENT_ERROR_INVALID_ARGUMENT;
    }
    TitleTokenCache::Instance().Store(*account_id, *comm_id, *token, *client_id);
    return 0;
}

s32 AsmClientService::DeleteTitleTokenCache(std::span<const IpmiDataInfo> in) {
    const auto* account_id = InputAs<OrbisNpAccountId>(in, 0);
    const auto comm_id = StringInput(in, 1);
    if (account_id == nullptr || !comm_id) {
        return ORBIS_NP_ASM_CLIENT_ERROR_INVALID_ARGUMENT;
    }
    TitleTokenCache::Instance().Remove(*account_id, *comm_id);
    return 0;
}

// Used by sceNpAsmClientGetServiceBaseUrl variants, which are called by libSceNpWebApi.
// These return a cached URL for HTTP requests to use.
s32 AsmClientService::GetServiceBaseUrlCache(std::span<const IpmiDataInfo> in,
                                             std::span<IpmiBufferInfo> out) {
    const auto* url_size = InputAs<u64>(in, 0);
    const auto title_id = StringInput(in, 1);
    const auto api_group = StringInput(in, 2);
    if (url_size == nullptr || !title_id || !api_group || out.size() < 2) {
        return ORBIS_NP_ASM_CLIENT_ERROR_INVALID_ARGUMENT;
    }

    const auto url = ServiceBaseUrlCache::Instance().Find(*title_id, *api_group);
    const bool found = url.has_value();
    WriteOutput(out[0], &found, sizeof(found));

    // The URL is null terminated within the caller's buffer.
    std::string buffer(std::min<u64>(*url_size, ServiceBaseUrlCache::MaxUrlLength), '\0');
    if (found && !buffer.empty()) {
        url->copy(buffer.data(), buffer.size() - 1);
    }
    return WriteOutput(out[1], buffer.data(), buffer.size());
}

// libSceNpManager caches each base URL it receives from the NP ASM server.
s32 AsmClientService::SetServiceBaseUrlCache(std::span<const IpmiDataInfo> in) {
    const auto title_id = StringInput(in, 0);
    const auto api_group = StringInput(in, 1);
    const auto url = StringInput(in, 2);
    if (!title_id || !api_group || !url) {
        return ORBIS_NP_ASM_CLIENT_ERROR_INVALID_ARGUMENT;
    }
    ServiceBaseUrlCache::Instance().Store(*title_id, *api_group, *url);
    return 0;
}

} // namespace Libraries::Kernel::Ipmi::NpService
