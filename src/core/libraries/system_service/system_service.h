// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::SystemService {

enum OrbisSystemServiceEventType : s32 {
    SessionInvitation = 0x10000002,
};

struct OrbisSystemServiceStatus {
    s32 event_num;
    bool is_system_ui_overlaid;
    bool is_in_background_execution;
    bool is_cpu_mode7_cpu_normal;
    bool is_game_live_streaming_on_air;
    bool is_out_of_vr_play_area;
    u8 reserved[];
};

struct OrbisSystemServiceEvent {
    OrbisSystemServiceEventType event_type;
    union {
        u8 param[8192];
    };
};

s32 sceSystemServiceGetStatus(OrbisSystemServiceStatus* status);
s32 sceSystemServiceReceiveEvent(OrbisSystemServiceEvent* event);

void PushSystemServiceEvent(const OrbisSystemServiceEvent& event);

void RegisterHooks();
} // namespace Libraries::SystemService
