// SPDX-FileCopyrightText: 2013 Dolphin Emulator Project
// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cctype>
#include <string>

#include "common/string_util.h"

namespace Common {

std::string ToLower(std::string_view input) {
    std::string str;
    str.resize(input.size());
    std::ranges::transform(input, str.begin(), tolower);
    return str;
}

void ToLowerInPlace(std::string& str) {
    std::ranges::transform(str, str.begin(), tolower);
}

} // namespace Common
