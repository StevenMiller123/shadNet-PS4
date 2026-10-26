// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::Rtc {

constexpr s32 ORBIS_RTC_DAYOFWEEK_SUNDAY = 0;
constexpr s32 ORBIS_RTC_DAYOFWEEK_MONDAY = 1;
constexpr s32 ORBIS_RTC_DAYOFWEEK_TUESDAY = 2;
constexpr s32 ORBIS_RTC_DAYOFWEEK_WEDNESDAY = 3;
constexpr s32 ORBIS_RTC_DAYOFWEEK_THURSDAY = 4;
constexpr s32 ORBIS_RTC_DAYOFWEEK_FRIDAY = 5;
constexpr s32 ORBIS_RTC_DAYOFWEEK_SATURDAY = 6;

constexpr s32 ORBIS_RTC_CLOCK_ID_NETWORK = 0x10;
constexpr s32 ORBIS_RTC_CLOCK_ID_DEBUG_NETWORK = 0x11;
constexpr s32 ORBIS_RTC_CLOCK_ID_AD_NETWORK = 0x12;

constexpr s64 UNIX_EPOCH_TICKS = 0xdcbffeff2bc000;
constexpr s64 WIN32_FILETIME_EPOCH_TICKS = 0xb36168b6a58000;

struct OrbisRtcTick {
    u64 tick;
};

struct OrbisRtcDateTime {
    u16 year;
    u16 month;
    u16 day;
    u16 hour;
    u16 minute;
    u16 second;
    u32 microsecond;
};

}; // namespace Libraries::Rtc