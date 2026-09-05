// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <cstring>
#include <string_view>

#include "common/types.h"
#include "core/libraries/error_codes.h"

#include "core/libraries/userservice/userservice.h"
#include "orbis/_types/Np.h"

namespace Libraries::Np {

constexpr s32 ORBIS_NP_ONLINEID_MAX_LENGTH = 16;
constexpr s32 ORBIS_NP_INVALID_SERVICE_LABEL = 0xFFFFFFFF;

using OrbisNpAccountId = u64;
using OrbisNpServiceLabel = u32;
using OrbisNpAccountId = u64;

}; // namespace Libraries::Np
