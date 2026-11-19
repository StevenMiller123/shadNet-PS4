// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>

#include "common/elf_info.h"
#include "core/file_format/nptitle.h"
#include "core/libraries/kernel/ipmi/np_service/basic_presence/basic_presence.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"

namespace Libraries::Kernel::Ipmi::NpService {

using Np::NpHandler;
using Np::OrbisNpOnlineId;

namespace {

enum class Method : u32 {
    GetGamePresenceStatus = 0x60002,
    SetNpTitleId = 0x60003,
};

enum class GamePresenceStatus : u32 {
    Offline = 0,
    Online = 1,
};

struct GamePresenceTitle {
    std::array<char, 0x10> title_id;
    std::array<char, 0x80> title_name;
};
static_assert(sizeof(GamePresenceTitle) == 0x90);

} // namespace

BasicPresenceService::BasicPresenceService()
    : Service("SceNpBasicPresence", Id, 0x60000, 0x6ffff) {}

s32 BasicPresenceService::InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                                           std::span<IpmiBufferInfo> out) {
    switch (static_cast<Method>(method)) {
    case Method::GetGamePresenceStatus:
        return GetGamePresenceStatus(in, out);
    case Method::SetNpTitleId:
        return SetNpTitleId(in);
    default:
        return Service::InvokeSyncMethod(method, in, out);
    }
}

// sceNpGetGamePresenceStatus, sceNpGetGamePresenceStatusA
s32 BasicPresenceService::GetGamePresenceStatus(std::span<const IpmiDataInfo> in,
                                                std::span<IpmiBufferInfo> out) {
    const auto* online_id = InputAs<OrbisNpOnlineId>(in, 0);
    if (online_id == nullptr || out.size() < 2) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    const s32 user_id = NpHandler::Instance().GetUserIdByOnlineId(*online_id);
    const bool online = user_id != -1 && NpHandler::Instance().IsSignedIn(user_id);

    GamePresenceTitle title{};
    if (online) {
        const std::optional<NpTitleFile>& np_title = Common::ElfInfo::Instance().GetNpTitle();
        if (np_title && np_title->IsValid()) {
            title.title_id = np_title->GetTitleId();
        } else {
            const std::string_view serial = Common::ElfInfo::Instance().GameSerial();
            std::copy_n(serial.begin(), std::min(serial.size(), title.title_id.size() - 1),
                        title.title_id.begin());
        }
        const std::string_view name = Common::ElfInfo::Instance().Title();
        std::copy_n(name.begin(), std::min(name.size(), title.title_name.size() - 1),
                    title.title_name.begin());
    }

    const GamePresenceStatus status =
        online ? GamePresenceStatus::Online : GamePresenceStatus::Offline;
    WriteOutput(out[0], &status, sizeof(status));
    return WriteOutput(out[1], &title, sizeof(title));
}

// sceNpSetNpTitleId, ignored in normal games.
s32 BasicPresenceService::SetNpTitleId(std::span<const IpmiDataInfo> in) {
    return 0;
}

} // namespace Libraries::Kernel::Ipmi::NpService
