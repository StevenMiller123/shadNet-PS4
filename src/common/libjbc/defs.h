// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Extracted from
// https://github.com/sleirsgoevy/ps4-libjbc/blob/master/defs.h

#pragma once
#include <stdint.h>

#define AF_UNIX 1
#define SOCK_STREAM 1

typedef uint64_t size_t;
typedef int64_t ssize_t;
typedef int64_t off_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef int32_t pid_t;

ssize_t read(int fd, void* dst, size_t sz);
ssize_t write(int fd, const void* dst, size_t sz);
int close(int fd);
