// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <cstring>
#include <span>
#include <string>
#include <string_view>

#include "common/endian.h"
#include "common/types.h"

#define NPTITLE_MAGIC 0x4E505444u // "NPTD"

#pragma pack(push, 1)
struct NpTitleHeader {
    u32_be magic;
    u32_be secret_size;
    u8 padding[8];
};
#pragma pack(pop)
static_assert(sizeof(NpTitleHeader) == 0x10);

// sce_sys/nptitle.dat, the NP title id and secret a title authenticates with.
class NpTitleFile {
public:
    using TitleId = std::array<char, 0x10>;
    using Secret = std::array<u8, 0x80>;

    // Load from file
    bool Load(const std::string& path);
    bool Load(std::span<const u8> data);

    const TitleId& GetTitleId() const {
        return m_title_id;
    }
    std::string_view GetTitleIdString() const {
        return {m_title_id.data(), strnlen(m_title_id.data(), m_title_id.size())};
    }
    const Secret& GetSecret() const {
        return m_secret;
    }

    // Check if file was loaded successfully
    bool IsValid() const {
        return m_header.magic == NPTITLE_MAGIC;
    }

    // Clear all data
    void Clear() {
        m_header = NpTitleHeader{};
        m_title_id = {};
        m_secret = {};
    }

private:
    NpTitleHeader m_header{};
    TitleId m_title_id{};
    Secret m_secret{};
};
