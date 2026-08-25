// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

extern "C" {
int pthread_getname_np(pthread_t thread, char* name);
void pthread_set_name_np(pthread_t thread, const char* name);
}