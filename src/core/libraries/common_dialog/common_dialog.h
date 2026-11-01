// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::CommonDialog {

enum OrbisCommonDialogResult : s32 {
    ORBIS_COMMON_DIALOG_RESULT_OK = 0,
    ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED = 1,
};

enum OrbisCommonDialogStatus : s32 {
    ORBIS_COMMON_DIALOG_STATUS_NONE = 0,
    ORBIS_COMMON_DIALOG_STATUS_INITIALIZED = 1,
    ORBIS_COMMON_DIALOG_STATUS_RUNNING = 2,
    ORBIS_COMMON_DIALOG_STATUS_FINISHED = 3,
};

struct OrbisCommonDialogBaseParam {
    u64 size;
    u8 reserved[36];
    u32 magic;
};

} // namespace Libraries::CommonDialog
