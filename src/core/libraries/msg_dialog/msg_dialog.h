// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <orbis/MsgDialog.h>

#include "common/types.h"

namespace Libraries::MsgDialog {

// Marks the calling thread as one that runs plugin dialogs.
void SetPluginThread(bool plugin_thread);

s32 sceMsgDialogInitialize();
s32 sceMsgDialogOpen(const OrbisMsgDialogParam* param);
s32 sceMsgDialogClose();
s32 sceMsgDialogTerminate();
s32 sceMsgDialogGetResult(OrbisMsgDialogResult* result);
OrbisCommonDialogStatus sceMsgDialogUpdateStatus();
OrbisCommonDialogStatus sceMsgDialogGetStatus();
void RegisterHooks();

} // namespace Libraries::MsgDialog
