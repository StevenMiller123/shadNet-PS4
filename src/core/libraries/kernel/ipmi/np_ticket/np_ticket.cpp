// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <chrono>
#include <format>

#include "core/libraries/kernel/ipmi/np_ticket/np_ticket.h"

namespace Libraries::Kernel::Ipmi {

namespace {

constexpr u32 TicketVersion = 0x41000000;

enum class FieldType : u16 {
    Empty = 0,
    U32 = 1,
    U64 = 2,
    String = 4,
    Time = 7,
    Binary = 8,
    Section = 0x3000,
};

enum class SectionType : u16 {
    Body = 0x00,
    Footer = 0x02,
    Age = 0x10,
    DateOfBirth = 0x11,
};

constexpr u32 ReleaseIssuerId = 0x100;

constexpr std::chrono::milliseconds IssuedOffset = std::chrono::minutes{1};
constexpr std::chrono::milliseconds TicketLifetime = std::chrono::minutes{15};

std::atomic<u64> g_next_serial_id{1};

NpTicketFieldHeader MakeHeader(FieldType type, u64 size) {
    NpTicketFieldHeader header{};
    header.type = static_cast<u16>(type);
    header.size = static_cast<u16>(size);
    return header;
}

u16 SectionId(SectionType type) {
    return static_cast<u16>(FieldType::Section) | static_cast<u16>(type);
}

NpTicketFieldHeader MakeSectionHeader(SectionType type, u64 size) {
    NpTicketFieldHeader header{};
    header.type = SectionId(type);
    header.size = static_cast<u16>(size);
    return header;
}

NpTicketLargeSectionHeader MakeLargeHeader(u16 type, u64 size) {
    NpTicketLargeSectionHeader header{};
    header.type = type;
    header.size = static_cast<u32>(size);
    return header;
}

template <typename T, typename V>
void SetField(NpTicketField<T>& field, FieldType type, const V& value) {
    field.header = MakeHeader(type, sizeof(T));
    field.value = value;
}

template <u64 N>
std::array<char, N> MakeString(std::string_view str) {
    std::array<char, N> out{};
    str.copy(out.data(), std::min(str.size(), N));
    return out;
}

} // namespace

NpTicket CreateNpTicket(u64 account_id, const Np::OrbisNpOnlineId& online_id,
                        std::string_view service_id) {
    using namespace std::chrono;
    const auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    const auto issued_date = now - IssuedOffset;

    NpTicket ticket{};
    ticket.version = TicketVersion;
    ticket.size = sizeof(NpTicket) - offsetof(NpTicket, body);

    NpTicketBody& body = ticket.body;
    body.header =
        MakeLargeHeader(SectionId(SectionType::Body), sizeof(NpTicketBody) - sizeof(body.header));
    SetField(body.serial_id, FieldType::Binary,
             MakeString<0x14>(std::format("{}", g_next_serial_id++)));
    SetField(body.issuer_id, FieldType::U32, ReleaseIssuerId);
    SetField(body.issued_date, FieldType::Time, static_cast<u64>(issued_date.count()));
    SetField(body.expire_date, FieldType::Time,
             static_cast<u64>((issued_date + TicketLifetime).count()));
    SetField(body.account_id, FieldType::U64, account_id);
    SetField(body.online_id, FieldType::String, MakeString<0x20>(ViewNpOnlineId(online_id)));
    SetField(body.region, FieldType::Binary, MakeString<4>("br"));
    SetField(body.domain, FieldType::String, MakeString<4>("un"));
    SetField(body.service_id, FieldType::Binary, MakeString<0x18>(service_id));
    body.date_of_birth.header = MakeSectionHeader(SectionType::DateOfBirth, sizeof(NpTicketDate));
    SetField(body.status, FieldType::U32, u32{0});
    SetField(body.unknown_date, FieldType::Time, u64{0});
    body.age = MakeLargeHeader(SectionId(SectionType::Age), 0);
    body.reserved[0] = MakeLargeHeader(static_cast<u16>(FieldType::Empty), 0);
    body.reserved[1] = MakeLargeHeader(static_cast<u16>(FieldType::Empty), 0);
    body.console_id.header = MakeHeader(FieldType::Binary, sizeof(body.console_id.value));
    std::array<u8, 0x40> unknown{};
    unknown[0] = 1;
    SetField(body.unknown, FieldType::Binary, unknown);
    NpTicketSignature& signature = ticket.signature;
    signature.header = MakeSectionHeader(SectionType::Footer,
                                         sizeof(NpTicketSignature) - sizeof(NpTicketFieldHeader));
    signature.signer.header = MakeHeader(FieldType::Binary, sizeof(signature.signer.value));
    signature.signature.header = MakeHeader(FieldType::Binary, sizeof(signature.signature.value));
    return ticket;
}

} // namespace Libraries::Kernel::Ipmi
