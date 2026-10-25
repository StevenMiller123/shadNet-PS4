// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Extracted from
// https://github.com/sleirsgoevy/ps4-libjbc/blob/master/utils.h

#pragma once
#include <stdint.h>
#include "defs.h"

enum { CWD_KEEP, CWD_ROOT, CWD_RESET };

#ifdef __cplusplus
extern "C" {
#endif

void jbc_run_as_root(void (*fn)(void* arg), void* arg, int cwd_mode);
int jbc_mount_in_sandbox(const char* system_path, const char* mnt_name);
int jbc_unmount_in_sandbox(const char* mnt_name);

#ifdef __cplusplus
}
#endif