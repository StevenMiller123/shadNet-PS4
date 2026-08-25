// SPDX-FileCopyrightText: 2013 Dolphin Emulator Project
// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cerrno>
#include <cstddef>
#include <cstring>

#include "error.h"

namespace Common {

std::string NativeErrorToString(int e) {
    return std::string(strerror(e));
}

std::string GetLastErrorMsg() {
    return NativeErrorToString(errno);
}

} // namespace Common
