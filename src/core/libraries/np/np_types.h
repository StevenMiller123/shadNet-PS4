// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstring>
#include <string_view>

#include "common/types.h"

// For structs and constants shared between multiple Np libraries.
namespace Libraries::Np {

using OrbisNpAccountId = u64;

struct OrbisNpTitleId {
    char id[13];
    u8 padding[3];
};

struct OrbisNpTitleSecret {
    u8 data[128];
};

constexpr s32 ORBIS_NP_ONLINEID_MAX_LENGTH = 16;

struct OrbisNpOnlineId {
    char data[ORBIS_NP_ONLINEID_MAX_LENGTH];
    s8 term;
    s8 dummy[3];
};

struct OrbisNpId {
    OrbisNpOnlineId handle;
    u8 opt[8];
    u8 reserved[8];
};

inline void SetNpOnlineId(OrbisNpOnlineId& dst, std::string_view src) {
    dst = {};
    const size_t len = std::min(src.size(), static_cast<size_t>(ORBIS_NP_ONLINEID_MAX_LENGTH));
    if (len > 0) {
        std::memcpy(dst.data, src.data(), len);
    }
    dst.term = 0;
}

inline void SetNpId(OrbisNpId& dst, std::string_view online_id) {
    dst = {};
    SetNpOnlineId(dst.handle, online_id);
}

struct OrbisNpClientId {
    char id[129];
    u8 padding[7];
};

struct OrbisNpAuthorizationCode {
    char code[129];
    u8 padding[7];
};

struct OrbisNpClientSecret {
    char secret[257];
    u8 padding[7];
};

struct OrbisNpIdToken {
    char token[4097];
    u8 padding[7];
};

using OrbisNpServiceLabel = u32;
constexpr s32 ORBIS_NP_INVALID_SERVICE_LABEL = 0xFFFFFFFF;

enum class OrbisNpPlatformType : s32 {
    None = 0,
    PS3 = 1,
    Vita = 2,
    PS4 = 3,
};

enum class OrbisNpState : s32 {
    NotSignedUp = 0,
    SignedOut = 1,
    SignedIn = 2,
};

enum class OrbisNpReachabilityState : s32 {
    Unavailable = 0,
    Available = 1,
    Reachable = 2,
};

enum class OrbisNpGamePresenseStatus {
    Offline = 0,
    Online = 1,
};

struct OrbisNpPeerAddress {
    OrbisNpOnlineId onlineId;
    OrbisNpPlatformType platform;
    char padding[4];
};

struct OrbisNpPeerAddressA {
    OrbisNpAccountId accountId;
    OrbisNpPlatformType platform;
    char padding[4];
};

}; // namespace Libraries::Np