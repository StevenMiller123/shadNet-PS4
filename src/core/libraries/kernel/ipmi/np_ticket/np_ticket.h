// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <string_view>

#include "common/endian.h"
#include "common/types.h"
#include "core/libraries/np/np_types.h"

namespace Libraries::Kernel::Ipmi {

// An NP ticket, which PSN issues to prove a user's identity to other servers.
#pragma pack(push, 1)
struct NpTicketFieldHeader {
    u16_be type;
    u16_be size;
};
static_assert(sizeof(NpTicketFieldHeader) == 4);

// Empty values, the body section, and the 0x3010 section use a 32-bit size.
struct NpTicketLargeSectionHeader {
    u16_be type;
    u32_be size;
};
static_assert(sizeof(NpTicketLargeSectionHeader) == 6);

template <typename T>
struct NpTicketField {
    NpTicketFieldHeader header;
    T value;
};

struct NpTicketDate {
    u16_be year;
    u8 month;
    u8 day;
};

struct NpTicketBody {
    NpTicketLargeSectionHeader header;
    NpTicketField<std::array<char, 0x14>> serial_id;
    NpTicketField<u32_be> issuer_id;
    NpTicketField<u64_be> issued_date;
    NpTicketField<u64_be> expire_date;
    NpTicketField<u64_be> account_id;
    NpTicketField<std::array<char, 0x20>> online_id;
    NpTicketField<std::array<char, 4>> region;
    NpTicketField<std::array<char, 4>> domain;
    NpTicketField<std::array<char, 0x18>> service_id;
    NpTicketField<NpTicketDate> date_of_birth;
    NpTicketField<u32_be> status;
    NpTicketField<u64_be> unknown_date;
    NpTicketLargeSectionHeader age;
    NpTicketLargeSectionHeader reserved[2];
    NpTicketField<std::array<char, 8>> console_id;
    NpTicketField<std::array<u8, 0x40>> unknown;
};

struct NpTicketSignature {
    NpTicketFieldHeader header;
    NpTicketField<std::array<u8, 4>> signer;
    NpTicketField<std::array<u8, 0x20>> signature;
};

struct NpTicket {
    u32_be version;
    u16_be reserved;
    u32_be size;
    NpTicketBody body;
    NpTicketSignature signature;
};
static_assert(sizeof(NpTicket) == 0x152);
#pragma pack(pop)

NpTicket CreateNpTicket(u64 account_id, const Np::OrbisNpOnlineId& online_id,
                        std::string_view service_id);

} // namespace Libraries::Kernel::Ipmi
