// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#include "common/types.h"

namespace Libraries::Kernel::Ipmi::NpService {

// A cache of NP service urls which libSceNpManager interacts with through SceNpAsmClient
class ServiceBaseUrlCache {
public:
    static constexpr u64 MaxEntries = 0x100;
    static constexpr std::chrono::hours Lifetime{12};
    static constexpr u64 MaxTitleIdLength = 0xc;
    static constexpr u64 MaxApiGroupLength = 0x20;
    static constexpr u64 MaxUrlLength = 0x80;

    static ServiceBaseUrlCache& Instance();

    // Returns the cached base URL, nullopt if there's no valid one.
    std::optional<std::string> Find(std::string_view title_id, std::string_view api_group);
    void Store(std::string_view title_id, std::string_view api_group, std::string_view url);

private:
    struct Entry {
        std::string title_id;
        std::string api_group;
        std::string url;
        std::chrono::steady_clock::time_point stored;
    };

    std::mutex m_mutex;
    std::list<Entry> m_entries;
};

} // namespace Libraries::Kernel::Ipmi::NpService
