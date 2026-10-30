// SPDX-FileCopyrightText: 2013 Dolphin Emulator Project
// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <string_view>

namespace Common {

[[nodiscard]] std::string ToLower(std::string_view str);

void ToLowerInPlace(std::string& str);

} // namespace Common
