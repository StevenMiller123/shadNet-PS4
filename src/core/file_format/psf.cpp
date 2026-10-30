// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include <fstream>
#include <ranges>

#include "common/logging/log.h"
#include "core/file_format/psf.h"

bool PSF::Open(const std::filesystem::path& filepath) {
    std::ifstream f(filepath, std::ios::binary | std::ios::ate);
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
    return Open(buf);
}

bool PSF::Open(const std::vector<u8>& psf_buffer) {
    const u8* psf_data = psf_buffer.data();

    entry_list.clear();
    map_binaries.clear();
    map_strings.clear();
    map_integers.clear();

    // Parse file contents
    PSFHeader header{};
    std::memcpy(&header, psf_data, sizeof(header));

    if (header.magic != PSF_MAGIC) {
        LOG_ERROR(Core, "Invalid PSF magic number");
        return false;
    }
    if (header.version != PSF_VERSION_1_1 && header.version != PSF_VERSION_1_0) {
        LOG_ERROR(Core, "Unsupported PSF version: 0x{:08x}", header.version);
        return false;
    }

    for (u32 i = 0; i < header.index_table_entries; i++) {
        PSFRawEntry raw_entry{};
        std::memcpy(&raw_entry, psf_data + sizeof(PSFHeader) + i * sizeof(PSFRawEntry),
                    sizeof(raw_entry));

        Entry& entry = entry_list.emplace_back();
        entry.key = std::string{(char*)(psf_data + header.key_table_offset + raw_entry.key_offset)};
        entry.param_fmt = static_cast<PSFEntryFmt>(raw_entry.param_fmt.Raw());
        entry.max_len = raw_entry.param_max_len;

        const u8* data = psf_data + header.data_table_offset + raw_entry.data_offset;

        switch (entry.param_fmt) {
        case PSFEntryFmt::Binary: {
            std::vector<u8> value(raw_entry.param_len);
            std::memcpy(value.data(), data, raw_entry.param_len);
            map_binaries.emplace(i, std::move(value));
        } break;
        case PSFEntryFmt::Text: {
            std::string c_str{reinterpret_cast<const char*>(data)};
            map_strings.emplace(i, std::move(c_str));
        } break;
        case PSFEntryFmt::Integer: {
            s32 integer = *(s32*)data;
            map_integers.emplace(i, integer);
        } break;
        default:
            LOG_WARNING(Core, "Unknown PSF entry type {}", u32(entry.param_fmt));
        }
    }
    return true;
}

std::optional<std::span<const u8>> PSF::GetBinary(std::string_view key) const {
    const auto& [it, index] = FindEntry(key);
    if (it == entry_list.end()) {
        return {};
    }
    return std::span{map_binaries.at(index)};
}

std::optional<std::string_view> PSF::GetString(std::string_view key) const {
    const auto& [it, index] = FindEntry(key);
    if (it == entry_list.end()) {
        return {};
    }
    return std::string_view{map_strings.at(index)};
}

std::optional<s32> PSF::GetInteger(std::string_view key) const {
    const auto& [it, index] = FindEntry(key);
    if (it == entry_list.end()) {
        return {};
    }
    return map_integers.at(index);
}

std::pair<std::vector<PSF::Entry>::iterator, u64> PSF::FindEntry(std::string_view key) {
    auto entry =
        std::ranges::find_if(entry_list, [&](const auto& entry) { return entry.key == key; });
    return {entry, std::distance(entry_list.begin(), entry)};
}

std::pair<std::vector<PSF::Entry>::const_iterator, u64> PSF::FindEntry(std::string_view key) const {
    auto entry =
        std::ranges::find_if(entry_list, [&](const auto& entry) { return entry.key == key; });
    return {entry, std::distance(entry_list.begin(), entry)};
}
