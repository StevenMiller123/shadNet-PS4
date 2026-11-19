// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/ipmi/np_service/asm_client/service_base_url_cache.h"

namespace Libraries::Kernel::Ipmi::NpService {

ServiceBaseUrlCache& ServiceBaseUrlCache::Instance() {
    static ServiceBaseUrlCache instance;
    return instance;
}

std::optional<std::string> ServiceBaseUrlCache::Find(std::string_view title_id,
                                                     std::string_view api_group) {
    const auto now = std::chrono::steady_clock::now();
    std::scoped_lock lk{m_mutex};
    for (const Entry& entry : m_entries) {
        if (entry.title_id == title_id && entry.api_group == api_group &&
            now - entry.stored <= Lifetime) {
            return entry.url;
        }
    }
    return std::nullopt;
}

void ServiceBaseUrlCache::Store(std::string_view title_id, std::string_view api_group,
                                std::string_view url) {
    // Skip entries that are too large
    if (title_id.size() > MaxTitleIdLength || api_group.size() > MaxApiGroupLength ||
        url.size() > MaxUrlLength) {
        return;
    }

    std::scoped_lock lk{m_mutex};
    if (m_entries.size() >= MaxEntries) {
        m_entries.pop_front();
    }
    m_entries.push_back({std::string(title_id), std::string(api_group), std::string(url),
                         std::chrono::steady_clock::now()});
}

} // namespace Libraries::Kernel::Ipmi::NpService
