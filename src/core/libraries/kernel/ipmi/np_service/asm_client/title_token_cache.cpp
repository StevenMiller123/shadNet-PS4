// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <orbis/Rtc.h>

#include "core/libraries/kernel/ipmi/np_service/asm_client/title_token_cache.h"

namespace Libraries::Kernel::Ipmi::NpService {

using Np::OrbisNpAccountId;
using Np::OrbisNpClientId;
using Np::OrbisNpTitleToken;

TitleTokenCache& TitleTokenCache::Instance() {
    static TitleTokenCache instance;
    return instance;
}

bool TitleTokenCache::Find(OrbisNpAccountId account_id, std::string_view comm_id,
                           OrbisNpTitleToken* token, OrbisNpClientId* client_id) {
    comm_id = comm_id.substr(0, CommIdSize);
    u64 now = 0;
    if (sceRtcGetCurrentRawNetworkTick(&now) < 0) {
        return false;
    }

    std::scoped_lock lk{m_mutex};
    for (const Entry& entry : m_entries) {
        if (entry.account_id == account_id && entry.comm_id == comm_id && entry.valid_from <= now &&
            now <= entry.valid_until) {
            if (token != nullptr) {
                *token = entry.token;
            }
            if (client_id != nullptr) {
                *client_id = entry.client_id;
            }
            return true;
        }
    }
    return false;
}

void TitleTokenCache::Store(OrbisNpAccountId account_id, std::string_view comm_id,
                            const OrbisNpTitleToken& token, const OrbisNpClientId& client_id) {
    if (account_id == 0 || comm_id.size() >= CommIdSize) {
        return;
    }
    if (Find(account_id, comm_id, nullptr, nullptr)) {
        Remove(account_id, comm_id);
    }

    u64 now = 0;
    if (sceRtcGetCurrentRawNetworkTick(&now) < 0) {
        return;
    }

    std::scoped_lock lk{m_mutex};
    if (m_entries.size() >= MaxEntries) {
        m_entries.pop_front();
    }
    m_entries.push_back({account_id, std::string(comm_id), token, client_id, now,
                         now + ((token.expiration_time - now) >> 1)});
}

void TitleTokenCache::Remove(OrbisNpAccountId account_id, std::string_view comm_id) {
    comm_id = comm_id.substr(0, CommIdSize);
    std::scoped_lock lk{m_mutex};
    auto it = std::find_if(m_entries.begin(), m_entries.end(), [&](const Entry& entry) {
        return entry.account_id == account_id && entry.comm_id == comm_id;
    });
    if (it != m_entries.end()) {
        m_entries.erase(it);
    }
}

} // namespace Libraries::Kernel::Ipmi::NpService
