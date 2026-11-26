// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/user_service/user_service.h"

namespace Libraries::Np::NpSns {

struct OrbisNpSnsFacebookAccessTokenParam {
    u64 size;
    UserService::OrbisUserServiceUserId user_id;
    u64 fb_app_id;
    char permissions[1024];
    u8 reserved[32];
};
static_assert(sizeof(OrbisNpSnsFacebookAccessTokenParam) == 0x438);

struct OrbisNpSnsFacebookAccessTokenResult {
    u64 expiration;
    char access_token[4096];
    u8 reserved[39];
};
static_assert(sizeof(OrbisNpSnsFacebookAccessTokenResult) == 0x1030);

} // namespace Libraries::Np::NpSns