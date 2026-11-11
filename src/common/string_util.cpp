// SPDX-FileCopyrightText: 2013 Dolphin Emulator Project
// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cctype>
#include <string>

#include "common/string_util.h"
#include "common/types.h"

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

std::u16string UTF8ToUTF16(std::string_view str) {
    std::u16string out;
    out.reserve(str.size());
    for (u64 i = 0; i < str.size();) {
        const u8 lead = static_cast<u8>(str[i]);
        const u64 length = lead < 0x80           ? 1
                           : (lead >> 5) == 0x6  ? 2
                           : (lead >> 4) == 0xe  ? 3
                           : (lead >> 3) == 0x1e ? 4
                                                 : 0;
        if (length == 0 || i + length > str.size()) {
            out.push_back(u'\xfffd');
            i++;
            continue;
        }
        u32 code_point = length == 1 ? lead : lead & (0xff >> (length + 1));
        bool valid = true;
        for (u64 j = 1; j < length; j++) {
            const u8 next = static_cast<u8>(str[i + j]);
            valid &= (next & 0xc0) == 0x80;
            code_point = (code_point << 6) | (next & 0x3f);
        }
        i += valid ? length : 1;
        if (!valid || code_point > 0x10ffff) {
            out.push_back(u'\xfffd');
        } else if (code_point >= 0x10000) {
            code_point -= 0x10000;
            out.push_back(static_cast<char16_t>(0xd800 + (code_point >> 10)));
            out.push_back(static_cast<char16_t>(0xdc00 + (code_point & 0x3ff)));
        } else {
            out.push_back(static_cast<char16_t>(code_point));
        }
    }
    return out;
}

std::string UTF16ToUTF8(std::u16string_view str) {
    std::string out;
    out.reserve(str.size());
    for (u64 i = 0; i < str.size(); i++) {
        u32 code_point = str[i];
        if (code_point >= 0xd800 && code_point < 0xdc00 && i + 1 < str.size() &&
            str[i + 1] >= 0xdc00 && str[i + 1] < 0xe000) {
            code_point = 0x10000 + ((code_point - 0xd800) << 10) + (str[++i] - 0xdc00);
        }
        if (code_point < 0x80) {
            out.push_back(static_cast<char>(code_point));
        } else if (code_point < 0x800) {
            out.push_back(static_cast<char>(0xc0 | (code_point >> 6)));
            out.push_back(static_cast<char>(0x80 | (code_point & 0x3f)));
        } else if (code_point < 0x10000) {
            out.push_back(static_cast<char>(0xe0 | (code_point >> 12)));
            out.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3f)));
            out.push_back(static_cast<char>(0x80 | (code_point & 0x3f)));
        } else {
            out.push_back(static_cast<char>(0xf0 | (code_point >> 18)));
            out.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3f)));
            out.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3f)));
            out.push_back(static_cast<char>(0x80 | (code_point & 0x3f)));
        }
    }
    return out;
}

} // namespace Common
