// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <list>
#include <mutex>
#include <string>
#include <string_view>

#include "common/types.h"
#include "core/libraries/np/np_types.h"

namespace Libraries::Kernel::Ipmi::NpService {

// A cache of NpTitleTokens which libSceNpManager interacts with through SceNpAsmClient
class TitleTokenCache {
public:
    static constexpr u64 MaxEntries = 0x100;
    // NP communication ids are compared up to their null terminator
    static constexpr u64 CommIdSize = 0xd;

    static TitleTokenCache& Instance();

    // Returns whether a valid token is cached, copying it and its client id when it is.
    bool Find(Np::OrbisNpAccountId account_id, std::string_view comm_id,
              Np::OrbisNpTitleToken* token, Np::OrbisNpClientId* client_id);
    void Store(Np::OrbisNpAccountId account_id, std::string_view comm_id,
               const Np::OrbisNpTitleToken& token, const Np::OrbisNpClientId& client_id);
    void Remove(Np::OrbisNpAccountId account_id, std::string_view comm_id);

private:
    struct Entry {
        Np::OrbisNpAccountId account_id;
        std::string comm_id;
        Np::OrbisNpTitleToken token;
        Np::OrbisNpClientId client_id;
        u64 valid_from;
        u64 valid_until;
    };

    std::mutex m_mutex;
    std::list<Entry> m_entries;
};

} // namespace Libraries::Kernel::Ipmi::NpService
