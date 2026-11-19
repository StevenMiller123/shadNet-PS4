// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <fstream>
#include <vector>

#include "core/file_format/nptitle.h"

bool NpTitleFile::Load(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        return false;
    }

    std::streamsize sz = f.tellg();
    if (sz <= 0) {
        return false;
    }

    f.seekg(0, std::ios::beg);
    std::vector<u8> buf(static_cast<u64>(sz));
    if (!f.read(reinterpret_cast<char*>(buf.data()), sz)) {
        return false;
    }

    return Load(std::span<const u8>{buf});
}

bool NpTitleFile::Load(std::span<const u8> data) {
    Clear(); // Clear any existing data

    // Header, title id, then secret
    constexpr u64 title_id_offset = sizeof(NpTitleHeader);
    constexpr u64 secret_offset = title_id_offset + sizeof(TitleId);
    if (data.size() < secret_offset + sizeof(Secret)) {
        return false;
    }

    std::memcpy(&m_header, data.data(), sizeof(NpTitleHeader));
    if (m_header.magic != NPTITLE_MAGIC || m_header.secret_size != sizeof(Secret)) {
        Clear();
        return false;
    }

    std::memcpy(m_title_id.data(), data.data() + title_id_offset, sizeof(TitleId));
    std::memcpy(m_secret.data(), data.data() + secret_offset, sizeof(Secret));
    return true;
}
