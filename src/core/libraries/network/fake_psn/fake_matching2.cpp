// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <deque>
#include <memory>
#include <string>
#include <orbis/Net.h>

#include "common/endian.h"
#include "common/logging/log.h"
#include "core/libraries/network/fake_psn/fake_matching2.h"
#include "core/libraries/network/fake_psn/fake_signaling.h"
#include "core/libraries/network/net/net_error.h"
#include "core/libraries/network/ssl/ssl_error.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "shadnet.pb.h"
#include "shadnet/client.h"
#include "shadnet/config.h"

namespace Libraries::Network {

namespace {

constexpr std::string_view SocketName = "SceNpMatching2";
constexpr u8 MessageVersion = 0x14;
constexpr u8 MessageVersionMask = 0xf0;

enum class MessageType : u8 {
    Request = 1,
    Reply = 2,
    Event = 4,
};

enum class Opcode : u16 {
    Connect = 0x1001,
    SignIn = 0x1002,
    RenewSession = 0x1003,
    GetWorldInfoList = 0x1008,
    SearchRoom = 0x1200,
    PublishRoom = 0x1202,
    JoinedRoom = 0x1203,
    SetRoomDataExternal = 0x1207,
    GetRoomDataExternalList = 0x1208,
    GetRoomServer = 0x1209,
    ReleaseRoomServer = 0x120b,
    // SearchRoom with account ids.
    SearchRoomA = 0x120c,
    RoomServerSignIn = 0x2001,
    CreateJoinRoom = 0x3200,
    JoinRoom = 0x3201,
    LeaveRoom = 0x3202,
    SetRoomDataInternal = 0x3204,
    SendRoomMessage = 0x320a,
};

enum class RoomEventOpcode : u16 {
    RoomMessage = 0x320b,
    MemberJoined = 0x320c,
    MemberDataInternalUpdated = 0x320d,
    MemberLeft = 0x320f,
    OwnerChanged = 0x3212,
    RoomClosed = 0x3213,
};

constexpr u16 AttributeIdMask = 0xfff;

// The room message kind libSceNpMatching2 uses for signaling.
constexpr u32 InternalMessageKind = 3;

enum class AttributeId : u16 {
    BinAttrData = 0x001,
    List = 0x002,
    RangeList = 0x003,
    RangeFilter = 0x004,
    RequestedAttributeId = 0x005,
    FlagFilter = 0x006,
    IntSearchFilter = 0x007,
    BinSearchFilter = 0x008,
    ServerHost = 0x009,
    ServerAddress = 0x00a,
    NpId = 0x00b,
    MemberString1 = 0x00c,
    MemberString2 = 0x00d,
    Environment = 0x00f,
    CommunicationId = 0x010,
    Ticket = 0x011,
    NatType = 0x013,
    SignInId = 0x014,
    CommunicationPassphrase = 0x015,
    MemberId = 0x016,
    RoomMessage = 0x01b,
    TeamId = 0x01c,
    ConnectResult = 0x01d,
    RoomServerKey = 0x020,
    RoomData2 = 0x021,
    SignInData1 = 0x022,
    SignInData2 = 0x023,
    RoomKey = 0x024,
    PresenceOptData = 0x028,
    RoomDataExternal = 0x045,
    WorldId = 0x029,
    World = 0x02a,
    LobbyId = 0x02e,
    RoomData1 = 0x03a,
    RoomNumber = 0x03d,
    MaxSlot = 0x03f,
    RoomFlags = 0x040,
    SlotGroups = 0x041,
    BlockedUser = 0x042,
    AllowedUser = 0x043,
    RoomPassword = 0x046,
    RoomGroup = 0x047,
    RoomMessageTargets = 0x044,
    JoinGroupLabel = 0x048,
    RoomMemberDataInternal = 0x04b,
    RoomSearchableIntAttrExternal1 = 0x04c,
    RoomSearchableIntAttrExternal2 = 0x04d,
    RoomSearchableIntAttrExternal3 = 0x04e,
    RoomSearchableIntAttrExternal4 = 0x04f,
    RoomSearchableIntAttrExternal5 = 0x050,
    RoomSearchableIntAttrExternal6 = 0x051,
    RoomSearchableIntAttrExternal7 = 0x052,
    RoomSearchableIntAttrExternal8 = 0x053,
    RoomSearchableBinAttrExternal1 = 0x054,
    RoomBinAttrExternal1 = 0x055,
    RoomBinAttrExternal2 = 0x056,
    RoomBinAttrInternal1 = 0x057,
    RoomBinAttrInternal2 = 0x058,
    RoomMemberBinAttrInternal = 0x059,
    SessionToken = 0x05d,
    RoomMessageOption = 0x064,
    EventCause = 0x062,
    PasswordSlotMask = 0x06a,
    SignalingParam = 0x06d,
    NatAddress = 0x06e,
    UpdateDate = 0x070,
    RoomDataInternal = 0x071,
    ServerId = 0x073,
    KeepAliveInterval = 0x074,
};

constexpr u8 ConnectStatusReady = 1;
constexpr u16 KeepAliveInterval = 60;

#pragma pack(push, 1)
struct MessageHeader {
    u8 version;
    u8 type;
    u16_be size;
};
static_assert(sizeof(MessageHeader) == 4);

struct PayloadHeader {
    u16_be id;
    u16_be size;
};
static_assert(sizeof(PayloadHeader) == 4);

struct ReplyHeader {
    MessageHeader message;
    u16_be opcode;
    u8 error;
    u8 reserved;
    u32_be request_id;
    std::array<u8, 0x14> reserved2;
};
static_assert(sizeof(ReplyHeader) == 0x20);

struct EventHeader {
    MessageHeader message;
    u16_be opcode;
    u16 reserved;
    // Signaling is kind 3, messages with kind bit 0 or 2 unset are passed to games
    u32_be message_kind;
    std::array<u8, 0x14> reserved2;
};
static_assert(sizeof(EventHeader) == 0x20);

struct ConnectResult {
    u16_be server_id;
    u8 status;
};

struct ConnectReply {
    PayloadHeader result_header;
    ConnectResult result;
};

struct SignInReply {
    PayloadHeader sign_in_id_header;
    u32_be sign_in_id;
    PayloadHeader data1_header;
    std::array<u8, 0x10> data1;
    PayloadHeader data2_header;
    std::array<u8, 0x10> data2;
    PayloadHeader session_token_header;
    std::array<u8, 0x10> session_token;
    PayloadHeader keep_alive_interval_header;
    u16_be keep_alive_interval;
};

struct RenewSessionReply {
    PayloadHeader session_token_header;
    std::array<u8, 0x10> session_token;
};

struct ListHeader {
    u16_be item_id;
    u16_be count;
};

struct RangeListHeader {
    u16_be item_id;
    u16_be count;
    u16_be range_start;
    u16_be range_total;
};

struct RangeFilter {
    u16_be start;
    u16_be max;
};

struct RoomDataExternalHeader {
    u16_be world_id;
    u16_be lobby_id;
    u16_be room_number;
    u32_be flags;
    u16_be max_slot;
    std::array<u8, 0x10> slot_states;
    u64_be password_slot_mask;
};
static_assert(sizeof(RoomDataExternalHeader) == 0x24);

struct World {
    u16_be world_id;
    u16_be lobbies;
    u32_be max_lobby_members;
    u32_be lobby_members;
    u16_be rooms;
    u32_be room_members;
};
static_assert(sizeof(World) == 0x12);

struct WorldItem {
    PayloadHeader header;
    World world;
};

struct RoomServerKeyData {
    u16_be room_number;
    u16 reserved;
    u32_be sign_in_id;
    std::array<u8, 8> reserved2;
};
static_assert(sizeof(RoomServerKeyData) == 0x10);

struct RoomServerKey {
    PayloadHeader header;
    RoomServerKeyData key;
};
static_assert(sizeof(RoomServerKey) == 0x14);

struct RoomGroup {
    u8 group_id;
    u8 has_label;
    Np::NpMatching2::OrbisNpMatching2GroupLabel label;
};
static_assert(sizeof(RoomGroup) == 10);

struct RoomBinAttrHeader {
    u16_be update_member_id;
    u64_be update_date;
};

struct RoomMemberBinAttrHeader {
    u64_be update_date;
};
#pragma pack(pop)

// Finds a value in a payload.
std::optional<std::span<const u8>> FindPayload(std::span<const u8> payload, AttributeId id) {
    while (payload.size() >= sizeof(PayloadHeader)) {
        PayloadHeader header{};
        std::memcpy(&header, payload.data(), sizeof(header));
        const u64 size = header.size;
        if (sizeof(header) + size > payload.size()) {
            break;
        }
        if ((header.id & AttributeIdMask) == static_cast<u16>(id)) {
            return payload.subspan(sizeof(header), size);
        }
        payload = payload.subspan(sizeof(header) + size);
    }
    return std::nullopt;
}

template <typename T>
std::optional<T> ReadPayload(std::span<const u8> payload, AttributeId id) {
    const auto value = FindPayload(payload, id);
    if (!value || value->size() < sizeof(T)) {
        return std::nullopt;
    }
    T out{};
    std::memcpy(&out, value->data(), sizeof(out));
    return out;
}

std::string HexDump(std::span<const u8> data) {
    std::string out;
    for (u64 i = 0; i < std::min<u64>(data.size(), 64); i++) {
        out += fmt::format("{:02x} ", data[i]);
    }
    if (data.size() > 64) {
        out += "...";
    }
    return out;
}

template <typename T>
void AppendBytes(std::vector<u8>& out, const T& value) {
    const auto* bytes = reinterpret_cast<const u8*>(&value);
    out.insert(out.end(), bytes, bytes + sizeof(value));
}

PayloadHeader MakePayloadHeader(AttributeId id, u64 size) {
    PayloadHeader header{};
    header.id = static_cast<u16>(id);
    header.size = static_cast<u16>(size);
    return header;
}

#pragma pack(push, 1)
struct RoomDataHeader {
    u16 server_id;
    u32_be flags;
    u16_be max_slot;
    std::array<u8, 0x10> slot_states;
    u64_be password_slot_mask;
};
static_assert(sizeof(RoomDataHeader) == 0x20);

struct RoomMemberDataHeader {
    u16_be member_id;
    u64_be join_date;
    u32_be flag_attr;
    u8 team_id;
    u8 nat_type;
};
static_assert(sizeof(RoomMemberDataHeader) == 0x10);

struct RoomMemberNpId {
    std::array<char, 0x10> online_id;
    std::array<u8, 8> opt;
};
static_assert(sizeof(RoomMemberNpId) == 0x18);
#pragma pack(pop)

constexpr u32 MemberSlotShift = 4;
constexpr u64 MaxSlots = 64;

u64 ToMilliseconds(u64 microseconds) {
    return microseconds / 1000;
}

u64 BeginPayload(std::vector<u8>& out, AttributeId id) {
    const u64 offset = out.size();
    AppendBytes(out, MakePayloadHeader(id, 0));
    return offset;
}

void EndPayload(std::vector<u8>& out, u64 offset) {
    PayloadHeader header{};
    std::memcpy(&header, out.data() + offset, sizeof(header));
    header.size = static_cast<u16>(out.size() - offset - sizeof(header));
    std::memcpy(out.data() + offset, &header, sizeof(header));
}

void AppendBlob(std::vector<u8>& out, AttributeId id, std::span<const u8> data) {
    AppendBytes(out, MakePayloadHeader(id, data.size()));
    out.insert(out.end(), data.begin(), data.end());
}

void AppendBinAttrData(std::vector<u8>& out, const std::string& data) {
    AppendBlob(out, AttributeId::BinAttrData,
               {reinterpret_cast<const u8*>(data.data()), data.size()});
}

// Groups take their slots in order, each slot holds its group id in a nibble.
std::array<u8, 0x20> SlotGroups(
    const google::protobuf::RepeatedPtrField<shadnet::MatchingRoomGroup>& groups) {
    std::array<u8, 0x20> slot_groups{};
    u64 slot = 0;
    for (const auto& group : groups) {
        for (u32 i = 0; i < group.slot_count() && slot < MaxSlots; i++, slot++) {
            slot_groups[slot / 2] |= (group.group_id() & 0xf) << ((slot % 2) == 0 ? 4 : 0);
        }
    }
    return slot_groups;
}

// Marks a joined slot in a room's slot states, slots start at 1.
void SetSlotJoined(std::array<u8, 0x10>& slot_states, u64 slot) {
    slot_states[(slot - 1) / 4] |= 1 << (6 - ((slot - 1) % 4) * 2);
}

void AppendRoomData(std::vector<u8>& out, const shadnet::CreateJoinRoomResponse& details,
                    const Np::NpMatching2::OrbisNpMatching2SignalingOptParam* signaling_param) {
    const auto& room = details.room_data();
    const u64 room_offset = BeginPayload(out, AttributeId::RoomDataInternal);

    RoomDataHeader header{};
    header.server_id = static_cast<u16>(room.server_id());
    header.flags = room.flags();
    header.max_slot = static_cast<u16>(room.max_slot());
    for (const auto& member : details.members()) {
        const u64 slot = member.member_id() >> MemberSlotShift;
        if (slot == 0 || slot > MaxSlots) {
            continue;
        }
        SetSlotJoined(header.slot_states, slot);
    }
    header.password_slot_mask = room.passwd_slot_mask();
    AppendBytes(out, header);

    AppendBlob(out, AttributeId::SlotGroups, SlotGroups(room.groups()));

    AppendBytes(out,
                MakePayloadHeader(AttributeId::List,
                                  sizeof(ListHeader) + room.groups_size() * (sizeof(PayloadHeader) +
                                                                             sizeof(RoomGroup))));
    ListHeader list{};
    list.item_id = static_cast<u16>(AttributeId::RoomGroup);
    list.count = static_cast<u16>(room.groups_size());
    AppendBytes(out, list);
    for (const auto& group : room.groups()) {
        RoomGroup item{};
        item.group_id = static_cast<u8>(group.group_id());
        item.has_label = group.has_label() ? 1 : 0;
        std::memcpy(item.label.data, group.label().data(),
                    std::min(group.label().size(), sizeof(item.label.data)));
        AppendBytes(out, MakePayloadHeader(AttributeId::RoomGroup, sizeof(item)));
        AppendBytes(out, item);
    }

    // Both room bin attributes are required, even when unset.
    for (const AttributeId id :
         {AttributeId::RoomBinAttrInternal1, AttributeId::RoomBinAttrInternal2}) {
        const shadnet::MatchingBinAttr* attr = nullptr;
        for (const auto& candidate : room.bin_attrs_internal()) {
            if (candidate.attr_id() == static_cast<u16>(id)) {
                attr = &candidate;
            }
        }
        const u64 attr_offset = BeginPayload(out, id);
        RoomBinAttrHeader attr_header{};
        if (attr != nullptr) {
            attr_header.update_member_id = static_cast<u16>(attr->update_member_id());
            attr_header.update_date = ToMilliseconds(attr->update_date());
        }
        AppendBytes(out, attr_header);
        AppendBinAttrData(out, attr != nullptr ? attr->data() : std::string{});
        EndPayload(out, attr_offset);
    }

    std::span<const u8> signaling{};
    if (signaling_param != nullptr) {
        signaling = {reinterpret_cast<const u8*>(signaling_param), sizeof(*signaling_param)};
    }
    AppendBlob(out, AttributeId::SignalingParam, signaling);
    EndPayload(out, room_offset);
}

void AppendMemberNpId(std::vector<u8>& out, const std::string& online_id);

void AppendRoomDataExternal(std::vector<u8>& out, const shadnet::MatchingRoomDataExternal& room) {
    const u64 room_offset = BeginPayload(out, AttributeId::RoomDataExternal);

    RoomDataExternalHeader header{};
    // shadNet's ids are the world, lobby, and room numbers games see.
    header.world_id = static_cast<u16>(room.world_id());
    header.lobby_id = static_cast<u16>(room.lobby_id());
    header.room_number = static_cast<u16>(room.room_id());
    header.flags = room.flags();
    header.max_slot = static_cast<u16>(room.max_slot());
    for (u64 slot = 1; slot <= MaxSlots; slot++) {
        if ((room.joined_slot_mask() & (1ULL << (slot - 1))) != 0) {
            SetSlotJoined(header.slot_states, slot);
        }
    }
    header.password_slot_mask = room.passwd_slot_mask();
    AppendBytes(out, header);

    AppendBlob(out, AttributeId::SlotGroups, SlotGroups(room.groups()));
    for (const auto& attr : room.external_search_int_attrs()) {
        u32_be value{};
        value = attr.attr_value();
        AppendBytes(out,
                    MakePayloadHeader(static_cast<AttributeId>(attr.attr_id()), sizeof(value)));
        AppendBytes(out, value);
    }
    for (const auto* attrs : {&room.external_search_bin_attrs(), &room.external_bin_attrs()}) {
        for (const auto& attr : *attrs) {
            AppendBlob(out, static_cast<AttributeId>(attr.attr_id()),
                       {reinterpret_cast<const u8*>(attr.data().data()), attr.data().size()});
        }
    }
    if (!room.owner_npid().empty()) {
        AppendMemberNpId(out, room.owner_npid());
    }
    EndPayload(out, room_offset);
}

// Members are identified in room messages by their slot,
// this is stored in the upper bits of their member id.
u16 MemberSlot(u32 member_id) {
    return static_cast<u16>(member_id >> MemberSlotShift);
}

std::array<u8, 0x10> MemberSlotBitmap(u32 member_id) {
    std::array<u8, 0x10> bitmap{};
    const u16 slot = MemberSlot(member_id);
    if (slot < bitmap.size() * 8) {
        bitmap[slot / 8] |= 0x80 >> (slot % 8);
    }
    return bitmap;
}

enum class RoomMessageCastType : u8 {
    Broadcast = 1,
    Unicast = 2,
    Multicast = 3,
    MulticastTeam = 4,
};

// libSceNpMatching2 rejects messages without a set cast type.
void AppendRoomMessageCastType(std::vector<u8>& out, RoomMessageCastType cast_type) {
    constexpr u16 CastTypeId = 0x001;
    const u64 option_offset = BeginPayload(out, AttributeId::RoomMessageOption);
    PayloadHeader header{};
    header.id = CastTypeId;
    header.size = sizeof(cast_type);
    AppendBytes(out, header);
    AppendBytes(out, cast_type);
    EndPayload(out, option_offset);
}

void AppendMemberNpId(std::vector<u8>& out, const std::string& online_id) {
    RoomMemberNpId np_id{};
    online_id.copy(np_id.online_id.data(), np_id.online_id.size());
    // Matches the options of our own NpId, so the library can compare them.
    np_id.opt = Np::ORBIS_NP_ID_OPT;
    AppendBytes(out, MakePayloadHeader(AttributeId::NpId, sizeof(np_id)));
    AppendBytes(out, np_id);
}

void AppendMemberId(std::vector<u8>& out, u32 member_id) {
    u16_be value{};
    value = static_cast<u16>(member_id);
    AppendBytes(out, MakePayloadHeader(AttributeId::MemberId, sizeof(value)));
    AppendBytes(out, value);
}

void AppendEventCause(std::vector<u8>& out, u32 cause) {
    const u8 value = static_cast<u8>(cause);
    AppendBlob(out, AttributeId::EventCause, {&value, sizeof(value)});
}

// A room member's internal data, shared by join replies and member joined events.
struct RoomMemberData {
    u32 member_id;
    u64 join_date;
    u32 flag_attr;
    u32 team_id;
    u32 nat_type;
    std::string_view online_id;
    // Internal bin attributes
    u64 bin_attr_update_date;
    std::span<const u8> bin_attr_data;
};

void AppendMemberBinAttrInternal(std::vector<u8>& out, u64 update_date, std::span<const u8> data) {
    const u64 attr_offset = BeginPayload(out, AttributeId::RoomMemberBinAttrInternal);
    RoomMemberBinAttrHeader attr_header{};
    // shadNet reports microseconds, Matching2 expects milliseconds.
    attr_header.update_date = ToMilliseconds(update_date);
    AppendBytes(out, attr_header);
    AppendBlob(out, AttributeId::BinAttrData, data);
    EndPayload(out, attr_offset);
}

void AppendRoomMemberData(std::vector<u8>& out, const RoomMemberData& member) {
    const u64 member_offset = BeginPayload(out, AttributeId::RoomMemberDataInternal);

    RoomMemberDataHeader header{};
    header.member_id = static_cast<u16>(member.member_id);
    header.join_date = ToMilliseconds(member.join_date);
    header.flag_attr = member.flag_attr;
    header.team_id = static_cast<u8>(member.team_id);
    header.nat_type = static_cast<u8>(member.nat_type);
    AppendBytes(out, header);

    AppendMemberNpId(out, std::string(member.online_id));

    // Every value is required, shadNet doesn't have these strings.
    AppendBlob(out, AttributeId::MemberString1, {});
    AppendBlob(out, AttributeId::MemberString2, {});
    AppendMemberBinAttrInternal(out, member.bin_attr_update_date, member.bin_attr_data);

    // The member's address for signaling, which is still unknown.
    AppendBlob(out, AttributeId::NatAddress, {});
    EndPayload(out, member_offset);
}

void AppendRoomMemberData(std::vector<u8>& out, const shadnet::MatchingRoomMemberData& member) {
    RoomMemberData data{};
    data.member_id = member.member_id();
    data.join_date = member.join_date();
    data.flag_attr = member.flag_attr();
    data.team_id = member.team_id();
    data.nat_type = member.nat_type();
    data.online_id = member.npid();
    if (member.bin_attrs_internal_size() > 0) {
        const auto& attr = member.bin_attrs_internal(0);
        data.bin_attr_update_date = attr.update_date();
        data.bin_attr_data = {reinterpret_cast<const u8*>(attr.data().data()), attr.data().size()};
    }
    AppendRoomMemberData(out, data);
}

void AppendRoomMemberData(std::vector<u8>& out, const ShadNet::NotifyRoomEvent& event) {
    RoomMemberData data{};
    data.member_id = event.member_id;
    data.join_date = event.member_join_date;
    data.flag_attr = event.member_flag_attr;
    data.team_id = event.member_team_id;
    data.nat_type = event.member_nat_type;
    data.online_id = event.member_npid;
    if (!event.member_bin_attrs.empty()) {
        data.bin_attr_update_date = event.member_bin_attrs[0].update_date;
        data.bin_attr_data = event.member_bin_attrs[0].data;
    }
    AppendRoomMemberData(out, data);
}

std::vector<u8> BuildCreateJoinRoomReply(const shadnet::CreateRoomReply& reply,
                                         const Np::NpMatching2::CreateJoinRoomParams& params) {
    const auto& details = reply.details();
    const auto* signaling_param = params.UsesAccountIds() ? params.GetRequestA().signaling_param
                                                          : params.GetRequest().signaling_param;

    std::vector<u8> out;
    // Keys for the room, we keep these blank since we're not using encryption.
    static constexpr std::array<u8, 0x10> RoomKey{};
    AppendBlob(out, AttributeId::RoomKey, RoomKey);
    AppendBlob(out, AttributeId::RoomData1, RoomKey);
    AppendBlob(out, AttributeId::RoomData2, RoomKey);
    AppendRoomData(out, details, signaling_param);
    for (const auto& member : details.members()) {
        if (member.member_id() == details.me_member_id()) {
            AppendRoomMemberData(out, member);
        }
    }
    return out;
}

std::vector<u8> BuildJoinRoomReply(const shadnet::JoinRoomReply& reply) {
    const auto& details = reply.details();

    // shadNet doesn't keep the room's signaling parameters, members signal each other directly.
    Np::NpMatching2::OrbisNpMatching2SignalingOptParam signaling_param{};
    signaling_param.type = Np::NpMatching2::ORBIS_NP_MATCHING2_SIGNALING_TYPE_MESH;

    std::vector<u8> out;
    AppendMemberId(out, details.me_member_id());
    // Keys for the room, we keep these blank since we're not using encryption.
    static constexpr std::array<u8, 0x10> RoomKey{};
    AppendBlob(out, AttributeId::RoomKey, RoomKey);
    AppendBlob(out, AttributeId::RoomData1, RoomKey);
    AppendBlob(out, AttributeId::RoomData2, RoomKey);
    AppendRoomData(out, details, &signaling_param);

    // Joining lists every member.
    const u64 list_offset = BeginPayload(out, AttributeId::List);
    ListHeader list{};
    list.item_id = static_cast<u16>(AttributeId::RoomMemberDataInternal);
    list.count = static_cast<u16>(details.members_size());
    AppendBytes(out, list);
    for (const auto& member : details.members()) {
        AppendRoomMemberData(out, member);
    }
    EndPayload(out, list_offset);
    return out;
}

void AppendServerList(std::vector<u8>& out) {
    const std::string server_url = ShadNet::Settings::GetInstance().GetServerUrl();
    const u64 colon = server_url.rfind(':');
    const std::string host = server_url.substr(0, colon);
    u16_be port{};
    if (colon != std::string::npos) {
        port = static_cast<u16>(std::strtoul(server_url.c_str() + colon + 1, nullptr, 10));
    }

    // Each address is the port, then the host.
    const u64 address_size = sizeof(port) + sizeof(PayloadHeader) + host.size();
    AppendBytes(out, MakePayloadHeader(AttributeId::List,
                                       sizeof(ListHeader) + sizeof(PayloadHeader) + address_size));
    ListHeader list{};
    list.item_id = static_cast<u16>(AttributeId::ServerAddress);
    list.count = 1;
    AppendBytes(out, list);
    AppendBytes(out, MakePayloadHeader(AttributeId::ServerAddress, address_size));
    AppendBytes(out, port);
    AppendBytes(out, MakePayloadHeader(AttributeId::ServerHost, host.size()));
    out.insert(out.end(), host.begin(), host.end());
}

struct RoomAttributes {
    std::vector<Np::NpMatching2::OrbisNpMatching2IntAttr> search_int_attrs;
    std::vector<Np::NpMatching2::OrbisNpMatching2BinAttr> search_bin_attrs;
    std::vector<Np::NpMatching2::OrbisNpMatching2BinAttr> external_bin_attrs;
    std::vector<Np::NpMatching2::OrbisNpMatching2BinAttr> internal_bin_attrs;
    std::optional<u32> flag_filter;
    u32 flag_attr = 0;
    std::optional<Np::NpMatching2::OrbisNpMatching2RoomPasswordSlotMask> password_slot_mask;
    std::deque<std::vector<u8>> bin_attr_data;
};

RoomAttributes ParseRoomAttributes(std::span<const u8> payload) {
    RoomAttributes attrs;
    const auto add_bin_attr = [&](std::vector<Np::NpMatching2::OrbisNpMatching2BinAttr>& out,
                                  u16 id, std::span<const u8> data) {
        auto& storage = attrs.bin_attr_data.emplace_back(data.begin(), data.end());
        Np::NpMatching2::OrbisNpMatching2BinAttr attr{};
        attr.id = id;
        attr.data = storage.data();
        attr.data_size = storage.size();
        out.push_back(attr);
    };

    while (payload.size() >= sizeof(PayloadHeader)) {
        PayloadHeader header{};
        std::memcpy(&header, payload.data(), sizeof(header));
        const u64 size = header.size;
        if (sizeof(header) + size > payload.size()) {
            break;
        }
        const u16 id = header.id & AttributeIdMask;
        const std::span<const u8> data = payload.subspan(sizeof(header), size);
        payload = payload.subspan(sizeof(header) + size);

        if (id >= static_cast<u16>(AttributeId::RoomSearchableIntAttrExternal1) &&
            id <= static_cast<u16>(AttributeId::RoomSearchableIntAttrExternal8)) {
            u32_be value{};
            if (data.size() >= sizeof(value)) {
                std::memcpy(&value, data.data(), sizeof(value));
                Np::NpMatching2::OrbisNpMatching2IntAttr attr{};
                attr.id = id;
                attr.num = value;
                attrs.search_int_attrs.push_back(attr);
            }
            continue;
        }
        switch (static_cast<AttributeId>(id)) {
        case AttributeId::RoomSearchableBinAttrExternal1:
            add_bin_attr(attrs.search_bin_attrs, id, data);
            break;
        case AttributeId::RoomBinAttrExternal1:
        case AttributeId::RoomBinAttrExternal2:
            add_bin_attr(attrs.external_bin_attrs, id, data);
            break;
        case AttributeId::RoomBinAttrInternal1:
        case AttributeId::RoomBinAttrInternal2:
            // Internal attributes start with the updating member and date, which are unset.
            if (data.size() >= sizeof(RoomBinAttrHeader)) {
                const auto value =
                    FindPayload(data.subspan(sizeof(RoomBinAttrHeader)), AttributeId::BinAttrData);
                add_bin_attr(attrs.internal_bin_attrs, id, value.value_or(std::span<const u8>{}));
            }
            break;
        case AttributeId::FlagFilter: {
            u32_be filter{};
            if (data.size() >= sizeof(filter)) {
                std::memcpy(&filter, data.data(), sizeof(filter));
                attrs.flag_filter = filter;
                attrs.flag_attr =
                    ReadPayload<u32_be>(data.subspan(sizeof(filter)), AttributeId::RoomFlags)
                        .value_or(u32_be{});
            }
            break;
        }
        case AttributeId::PasswordSlotMask:
            if (data.size() >= sizeof(u64_be)) {
                u64_be mask{};
                std::memcpy(&mask, data.data(), sizeof(mask));
                attrs.password_slot_mask = mask;
            }
            break;
        default:
            break;
        }
    }
    return attrs;
}

} // namespace

#pragma pack(push, 1)
struct FakeMatching2::RequestHeader {
    MessageHeader message;
    u16_be opcode;
    // Set for internal room messages, used for signaling.
    u8 internal;
    u8 unknown7;
    u32_be request_id;
    u32_be sign_in_id;
    std::array<u8, 0x10> digest;
};
#pragma pack(pop)

FakeMatching2& FakeMatching2::Instance() {
    static FakeMatching2 instance;
    return instance;
}

const FakeMatching2::OpcodeHandler* FakeMatching2::FindOpcode(u16 opcode) {
    static constexpr std::array<OpcodeHandler, 18> Opcodes = {{
        {static_cast<u16>(Opcode::Connect), "connect", &FakeMatching2::Connect},
        {static_cast<u16>(Opcode::SignIn), "sign in", &FakeMatching2::SignIn},
        {static_cast<u16>(Opcode::GetRoomServer), "get room server", &FakeMatching2::GetRoomServer},
        {static_cast<u16>(Opcode::RenewSession), "renew session", &FakeMatching2::RenewSession},
        {static_cast<u16>(Opcode::RoomServerSignIn), "room server sign in",
         &FakeMatching2::RoomServerSignIn},
        {static_cast<u16>(Opcode::SearchRoom), "search room", &FakeMatching2::SearchRoom},
        {static_cast<u16>(Opcode::SearchRoomA), "search room", &FakeMatching2::SearchRoom},
        {static_cast<u16>(Opcode::GetWorldInfoList), "get world info list",
         &FakeMatching2::GetWorldInfoList},
        {static_cast<u16>(Opcode::CreateJoinRoom), "create join room",
         &FakeMatching2::CreateJoinRoom},
        {static_cast<u16>(Opcode::JoinRoom), "join room", &FakeMatching2::JoinRoom},
        {static_cast<u16>(Opcode::PublishRoom), "publish room", &FakeMatching2::PublishRoom},
        {static_cast<u16>(Opcode::JoinedRoom), "joined room", &FakeMatching2::JoinedRoom},
        {static_cast<u16>(Opcode::SetRoomDataExternal), "set room data external",
         &FakeMatching2::SetRoomDataExternal},
        {static_cast<u16>(Opcode::GetRoomDataExternalList), "get room data external list",
         &FakeMatching2::GetRoomDataExternalList},
        {static_cast<u16>(Opcode::SetRoomDataInternal), "set room data internal",
         &FakeMatching2::SetRoomDataInternal},
        {static_cast<u16>(Opcode::LeaveRoom), "leave room", &FakeMatching2::LeaveRoom},
        {static_cast<u16>(Opcode::SendRoomMessage), "send room message",
         &FakeMatching2::SendRoomMessage},
        {static_cast<u16>(Opcode::ReleaseRoomServer), "release room server",
         &FakeMatching2::ReleaseRoomServer},
    }};
    for (const OpcodeHandler& entry : Opcodes) {
        if (entry.opcode == opcode) {
            return &entry;
        }
    }
    return nullptr;
}

void FakeMatching2::OnSocketCreate(s32 socket, const char* name, s32 type) {
    if (name == nullptr || name != SocketName || type != ORBIS_NET_SOCK_STREAM) {
        return;
    }
    std::scoped_lock lk{m_mutex};
    m_sockets.insert(socket);
}

void FakeMatching2::OnSocketClose(s32 socket) {
    std::scoped_lock lk{m_mutex};
    m_sockets.erase(socket);
    if (const auto it = m_connections.find(socket); it != m_connections.end()) {
        LOG_INFO(Lib_NpMatching2, "matching2 connection {} closed", socket);
        // Room servers remove members whose connection closes, leave the room on shadNet too.
        if (it->second.room_id != 0 && it->second.user_id != -1) {
            Np::NpMatching2::OrbisNpMatching2LeaveRoomRequest request{};
            request.room_id = it->second.room_id;
            LOG_INFO(Lib_NpMatching2, "matching2 leaving room {} after its connection closed",
                     request.room_id);
            Np::NpHandler::Instance().LeaveRoom(it->second.user_id, 0, request, [](s32) {});
            FakeSignaling::Instance().OnRoomLeft(request.room_id);
        }
        m_connections.erase(it);
    }
    for (auto& [epoll, registrations] : m_epolls) {
        registrations.erase(socket);
    }
    std::erase_if(m_ssl_sockets, [&](const auto& entry) { return entry.second == socket; });
    m_cv.notify_all();
}

std::optional<s32> FakeMatching2::OnConnect(s32 socket, const OrbisNetSockaddr* addr,
                                            u32 addr_size) {
    std::scoped_lock lk{m_mutex};
    if (!m_sockets.contains(socket)) {
        return std::nullopt;
    }
    m_sockets.erase(socket);
    m_connections[socket] = {.socket = socket};

    std::string destination = "(unknown)";
    if (addr != nullptr && addr_size >= sizeof(OrbisNetSockaddrIn) &&
        addr->sa_family == ORBIS_NET_AF_INET) {
        const auto* addr_in = reinterpret_cast<const OrbisNetSockaddrIn*>(addr);
        const u32 ip = addr_in->sin_addr;
        destination = fmt::format("{}.{}.{}.{}:{}", ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff,
                                  ip >> 24, sceNetNtohs(addr_in->sin_port));
    }

    LOG_INFO(Lib_NpMatching2, "faking matching2 connection {} to {}", socket, destination);
    return 0;
}

std::optional<s32> FakeMatching2::OnSend(s32 socket, const void* data, u64 size, s32 flags) {
    std::scoped_lock lk{m_mutex};
    const auto it = m_connections.find(socket);
    if (it == m_connections.end()) {
        return std::nullopt;
    }
    if (data == nullptr) {
        return ORBIS_NET_ERROR_EINVAL;
    }
    Send(it->second, data, size);
    return static_cast<s32>(size);
}

std::optional<s32> FakeMatching2::OnRecv(s32 socket, void* buf, u64 size, s32 flags) {
    std::unique_lock lk{m_mutex};
    auto it = m_connections.find(socket);
    if (it == m_connections.end()) {
        return std::nullopt;
    }
    if (buf == nullptr) {
        return ORBIS_NET_ERROR_EINVAL;
    }
    if (it->second.received.empty()) {
        if ((flags & Net::ORBIS_NET_MSG_DONTWAIT) != 0) {
            return ORBIS_NET_ERROR_EWOULDBLOCK;
        }
        m_cv.wait(lk, [&] {
            it = m_connections.find(socket);
            return it == m_connections.end() || !it->second.received.empty();
        });
        if (it == m_connections.end()) {
            return 0;
        }
    }
    return static_cast<s32>(Receive(it->second, buf, size));
}

void FakeMatching2::OnSslCreate(s32 ssl_id, s32 socket) {
    std::scoped_lock lk{m_mutex};
    if (!m_connections.contains(socket)) {
        return;
    }

    LOG_INFO(Lib_NpMatching2, "created ssl connection {:#x} for matching2 connection {}",
             static_cast<u32>(ssl_id), socket);
    if (ssl_id >= 0) {
        m_ssl_sockets[ssl_id] = socket;
    }
}

void FakeMatching2::OnSslDelete(s32 ssl_id) {
    std::scoped_lock lk{m_mutex};
    m_ssl_sockets.erase(ssl_id);
}

std::optional<s32> FakeMatching2::OnSslConnect(s32 ssl_id) {
    std::scoped_lock lk{m_mutex};
    const auto it = m_ssl_sockets.find(ssl_id);
    if (it == m_ssl_sockets.end()) {
        return std::nullopt;
    }

    LOG_INFO(Lib_NpMatching2, "faking ssl handshake for matching2 connection {}", it->second);
    return 0;
}

std::optional<s32> FakeMatching2::OnSslClose(s32 ssl_id) {
    std::scoped_lock lk{m_mutex};
    if (!m_ssl_sockets.contains(ssl_id)) {
        return std::nullopt;
    }

    return 0;
}

std::optional<s32> FakeMatching2::OnSslSend(s32 ssl_id, const void* data, u64 size) {
    std::scoped_lock lk{m_mutex};
    Connection* connection = FindSslConnection(ssl_id);
    if (connection == nullptr) {
        return std::nullopt;
    }
    if (data == nullptr) {
        return ORBIS_SSL_ERROR_INVALID_VALUE;
    }
    Send(*connection, data, size);
    return static_cast<s32>(size);
}

std::optional<s32> FakeMatching2::OnSslRecv(s32 ssl_id, void* buf, u64 size) {
    std::scoped_lock lk{m_mutex};
    Connection* connection = FindSslConnection(ssl_id);
    if (connection == nullptr) {
        return std::nullopt;
    }
    if (buf == nullptr) {
        return ORBIS_SSL_ERROR_INVALID_VALUE;
    }
    if (connection->received.empty()) {
        return ORBIS_SSL_ERROR_EAGAIN;
    }
    return static_cast<s32>(Receive(*connection, buf, size));
}

std::optional<s32> FakeMatching2::OnSslCheckRecvPending(s32 ssl_id, u8* pending) {
    std::scoped_lock lk{m_mutex};
    Connection* connection = FindSslConnection(ssl_id);
    if (connection == nullptr) {
        return std::nullopt;
    }
    if (pending == nullptr) {
        return ORBIS_SSL_ERROR_INVALID_VALUE;
    }
    *pending = connection->received.empty() ? 0 : 1;
    return 0;
}

FakeMatching2::Connection* FakeMatching2::FindSslConnection(s32 ssl_id) {
    const auto socket = m_ssl_sockets.find(ssl_id);
    if (socket == m_ssl_sockets.end()) {
        return nullptr;
    }
    const auto it = m_connections.find(socket->second);
    return it == m_connections.end() ? nullptr : &it->second;
}

void FakeMatching2::Send(Connection& connection, const void* data, u64 size) {
    const auto* bytes = static_cast<const u8*>(data);
    LOG_DEBUG(Lib_NpMatching2, "matching2 connection {} sent {} bytes: {}", connection.socket, size,
              HexDump({bytes, size}));
    connection.sent.insert(connection.sent.end(), bytes, bytes + size);

    // Idle connections send a zero byte between messages to keep alive
    const auto keep_alive_end =
        std::find_if(connection.sent.begin(), connection.sent.end(), [](u8 b) { return b != 0; });
    const u64 keep_alives = std::distance(connection.sent.begin(), keep_alive_end);
    if (keep_alives != 0) {
        connection.sent.erase(connection.sent.begin(), keep_alive_end);
        connection.received.insert(connection.received.end(), keep_alives, u8{0});
    }

    while (connection.sent.size() >= sizeof(MessageHeader)) {
        MessageHeader header{};
        std::memcpy(&header, connection.sent.data(), sizeof(header));
        const u64 message_size = header.size;
        if ((header.version & MessageVersionMask) != (MessageVersion & MessageVersionMask) ||
            message_size < sizeof(MessageHeader)) {
            LOG_ERROR(
                Lib_NpMatching2,
                "invalid matching2 message header {:#x} {:#x}, size {}, buffered {} bytes: {}",
                header.version, header.type, message_size, connection.sent.size(),
                HexDump(connection.sent));
            connection.sent.clear();
            break;
        }
        if (connection.sent.size() < message_size) {
            break;
        }
        HandleMessage(connection, std::span<const u8>{connection.sent.data(), message_size});
        connection.sent.erase(connection.sent.begin(), connection.sent.begin() + message_size);
    }
    m_cv.notify_all();
}

u64 FakeMatching2::Receive(Connection& connection, void* buf, u64 size) {
    std::vector<u8>& received = connection.received;
    const u64 count = std::min(size, received.size());
    std::memcpy(buf, received.data(), count);
    received.erase(received.begin(), received.begin() + count);
    return count;
}

std::optional<s32> FakeMatching2::OnEpollControl(s32 epoll, s32 op, s32 socket,
                                                 const Net::OrbisNetEpollEvent* event) {
    std::scoped_lock lk{m_mutex};
    if (!m_connections.contains(socket)) {
        if (m_epolls.contains(epoll)) {
            LOG_WARNING(Lib_NpMatching2, "socket {} added to matching2 epoll {} is ignored", socket,
                        epoll);
        }
        return std::nullopt;
    }
    switch (op) {
    case Net::ORBIS_NET_EPOLL_CTL_ADD:
    case Net::ORBIS_NET_EPOLL_CTL_MOD:
        if (event == nullptr) {
            return ORBIS_NET_ERROR_EINVAL;
        }
        m_epolls[epoll][socket] = *event;
        break;
    case Net::ORBIS_NET_EPOLL_CTL_DEL:
        if (const auto it = m_epolls.find(epoll); it != m_epolls.end()) {
            it->second.erase(socket);
        }
        break;
    default:
        return ORBIS_NET_ERROR_EINVAL;
    }
    m_cv.notify_all();
    return 0;
}

std::optional<s32> FakeMatching2::OnEpollWait(s32 epoll, Net::OrbisNetEpollEvent* events,
                                              s32 max_events, s32 timeout) {
    std::unique_lock lk{m_mutex};
    if (!m_epolls.contains(epoll)) {
        return std::nullopt;
    }
    if (events == nullptr || max_events <= 0) {
        return ORBIS_NET_ERROR_EINVAL;
    }

    s32 count = 0;
    const auto ready = [&] {
        count = CollectEvents(epoll, events, max_events);
        return count != 0 || !m_epolls.contains(epoll);
    };
    if (timeout < 0) {
        m_cv.wait(lk, ready);
    } else if (timeout > 0) {
        m_cv.wait_for(lk, std::chrono::microseconds{timeout}, ready);
    } else {
        ready();
    }
    return count;
}

void FakeMatching2::OnEpollDestroy(s32 epoll) {
    std::scoped_lock lk{m_mutex};
    if (m_epolls.erase(epoll) != 0) {
        m_cv.notify_all();
    }
}

u32 FakeMatching2::ReadyEvents(s32 socket) const {
    const auto it = m_connections.find(socket);
    if (it == m_connections.end()) {
        return Net::ORBIS_NET_EPOLLHUP;
    }

    // Sends never block, so we're always writable.
    u32 ready = Net::ORBIS_NET_EPOLLOUT;
    if (!it->second.received.empty()) {
        ready |= Net::ORBIS_NET_EPOLLIN;
    }

    return ready;
}

s32 FakeMatching2::CollectEvents(s32 epoll, Net::OrbisNetEpollEvent* events, s32 max_events) const {
    const auto it = m_epolls.find(epoll);
    if (it == m_epolls.end()) {
        return 0;
    }
    s32 count = 0;
    for (const auto& [socket, registration] : it->second) {
        if (count == max_events) {
            break;
        }
        const u32 ready = ReadyEvents(socket) & (registration.events | Net::ORBIS_NET_EPOLLHUP);
        if (ready == 0) {
            continue;
        }
        events[count] = registration;
        events[count].events = ready;
        events[count].ident = static_cast<u64>(socket);
        count++;
    }
    return count;
}

void FakeMatching2::HandleMessage(Connection& connection, std::span<const u8> message) {
    MessageHeader header{};
    std::memcpy(&header, message.data(), sizeof(header));
    switch (static_cast<MessageType>(header.type)) {
    case MessageType::Request:
        HandleRequest(connection, message);
        break;
    default:
        LOG_WARNING(Lib_NpMatching2, "unhandled matching2 message type {}, size {}", header.type,
                    message.size());
        break;
    }
}

void FakeMatching2::HandleRequest(Connection& connection, std::span<const u8> message) {
    if (message.size() < sizeof(RequestHeader)) {
        LOG_ERROR(Lib_NpMatching2, "matching2 request is too small, size {}", message.size());
        return;
    }
    static_assert(sizeof(RequestHeader) == 0x20);
    RequestHeader header{};
    std::memcpy(&header, message.data(), sizeof(header));
    const std::span<const u8> payload = message.subspan(sizeof(header));

    const OpcodeHandler* entry = FindOpcode(header.opcode);
    if (entry == nullptr) {
        LOG_ERROR(Lib_NpMatching2, "unhandled matching2 opcode {:#x}, request {}, payload size {}",
                  static_cast<u16>(header.opcode), static_cast<u32>(header.request_id),
                  payload.size());
        return;
    }
    LOG_INFO(Lib_NpMatching2, "matching2 {} request {}", entry->name,
             static_cast<u32>(header.request_id));
    (this->*entry->handler)(connection, header, payload);
}

void FakeMatching2::QueueReply(Connection& connection, const RequestHeader& header, s32 error,
                               std::span<const u8> payload) {
    ReplyHeader reply{};
    reply.message.version = MessageVersion;
    reply.message.type = static_cast<u8>(MessageType::Reply);
    reply.opcode = header.opcode;
    reply.error = static_cast<u8>(error < 0 ? error & 0xff : 0);
    reply.request_id = header.request_id;
    if (error < 0) {
        payload = {};
    }
    reply.message.size = static_cast<u16>(sizeof(reply) + payload.size());

    const auto* reply_bytes = reinterpret_cast<const u8*>(&reply);
    connection.received.insert(connection.received.end(), reply_bytes, reply_bytes + sizeof(reply));
    connection.received.insert(connection.received.end(), payload.begin(), payload.end());
    m_cv.notify_all();
}

void FakeMatching2::QueueLateReply(s32 socket, const RequestHeader& header, s32 error,
                                   std::span<const u8> payload) {
    std::scoped_lock lk{m_mutex};
    const auto it = m_connections.find(socket);
    if (it == m_connections.end()) {
        LOG_WARNING(Lib_NpMatching2, "matching2 connection {} closed before request {} finished",
                    socket, static_cast<u32>(header.request_id));
        return;
    }
    QueueReply(it->second, header, error, payload);
}

void FakeMatching2::QueueEvent(Connection& connection, u16 opcode, std::span<const u8> payload,
                               bool internal) {
    EventHeader event{};
    if (internal) {
        event.message_kind = InternalMessageKind << 28;
    }
    event.message.version = MessageVersion;
    event.message.type = static_cast<u8>(MessageType::Event);
    event.message.size = static_cast<u16>(sizeof(event) + payload.size());
    event.opcode = opcode;

    const auto* event_bytes = reinterpret_cast<const u8*>(&event);
    connection.received.insert(connection.received.end(), event_bytes, event_bytes + sizeof(event));
    connection.received.insert(connection.received.end(), payload.begin(), payload.end());
    m_cv.notify_all();
}

void FakeMatching2::OnRoomEvent(s32 user_id, const ShadNet::NotifyRoomEvent& event) {
    using namespace Np::NpMatching2;
    std::scoped_lock lk{m_mutex};
    const auto it =
        std::find_if(m_connections.begin(), m_connections.end(), [&](const auto& entry) {
            return entry.second.user_id == user_id && entry.second.room_id == event.room_id;
        });
    if (it == m_connections.end()) {
        LOG_WARNING(Lib_NpMatching2, "matching2 room event {:#x} for room {} has no connection",
                    event.event, event.room_id);
        return;
    }
    Connection& connection = it->second;

    std::vector<u8> payload;
    RoomEventOpcode opcode{};
    switch (event.event) {
    case ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED:
        opcode = RoomEventOpcode::MemberJoined;
        connection.members[MemberSlot(event.member_id)] = static_cast<u16>(event.member_id);
        FakeSignaling::Instance().OnMemberJoined(event.room_id, static_cast<u16>(event.member_id),
                                                 event.member_addr,
                                                 static_cast<u16>(event.member_port));
        AppendRoomMemberData(payload, event);
        break;
    case ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_LEFT:
        opcode = RoomEventOpcode::MemberLeft;
        connection.members.erase(MemberSlot(event.member_id));
        FakeSignaling::Instance().OnMemberLeft(event.room_id, static_cast<u16>(event.member_id));
        AppendMemberId(payload, event.member_id);
        AppendMemberNpId(payload, event.member_npid);
        AppendEventCause(payload, event.event_cause);
        break;
    case ORBIS_NP_MATCHING2_ROOM_EVENT_ROOM_OWNER_CHANGED:
        opcode = RoomEventOpcode::OwnerChanged;
        AppendMemberId(payload, event.member_id);
        AppendMemberNpId(payload, event.member_npid);
        AppendEventCause(payload, event.event_cause);
        break;
    case ORBIS_NP_MATCHING2_ROOM_EVENT_UPDATED_ROOM_MEMBER_DATA_INTERNAL:
        opcode = RoomEventOpcode::MemberDataInternalUpdated;
        AppendMemberId(payload, event.member_id);
        if (!event.member_bin_attrs.empty()) {
            AppendMemberBinAttrInternal(payload, event.member_bin_attrs[0].update_date,
                                        event.member_bin_attrs[0].data);
        }
        break;
    case ORBIS_NP_MATCHING2_ROOM_EVENT_KICKEDOUT:
    case ORBIS_NP_MATCHING2_ROOM_EVENT_ROOM_DESTROYED:
        opcode = RoomEventOpcode::RoomClosed;
        AppendEventCause(payload, event.event_cause);
        FakeSignaling::Instance().OnRoomLeft(connection.room_id);
        connection.room_id = 0;
        break;
    default:
        LOG_WARNING(Lib_NpMatching2, "unhandled matching2 room event {:#x} for room {}",
                    event.event, event.room_id);
        return;
    }

    LOG_INFO(Lib_NpMatching2, "matching2 room event {:#x} for room {}, member {}", event.event,
             event.room_id, event.member_id);
    QueueEvent(connection, static_cast<u16>(opcode), payload);
}

void FakeMatching2::Connect(Connection& connection, const RequestHeader& header,
                            std::span<const u8> payload) {
    const auto comm_id = FindPayload(payload, AttributeId::CommunicationId);
    const auto server_id = ReadPayload<u16_be>(payload, AttributeId::ServerId);
    LOG_INFO(Lib_NpMatching2, "matching2 connect for {} on server {}",
             comm_id ? std::string(comm_id->begin(), comm_id->end()) : "(none)",
             server_id ? static_cast<u16>(*server_id) : 0);

    connection.server_id = server_id ? static_cast<u16>(*server_id) : 0;

    ConnectReply reply{};
    reply.result_header = MakePayloadHeader(AttributeId::ConnectResult, sizeof(ConnectResult));
    reply.result.server_id = server_id.value_or(u16_be{});
    reply.result.status = ConnectStatusReady;
    QueueReply(connection, header, 0, reply);
}

void FakeMatching2::SignIn(Connection& connection, const RequestHeader& header,
                           std::span<const u8> payload) {
    const auto environment = FindPayload(payload, AttributeId::Environment);
    const auto np_id = ReadPayload<Np::OrbisNpOnlineId>(payload, AttributeId::NpId);
    const auto comm_id = FindPayload(payload, AttributeId::CommunicationId);
    const auto ticket = FindPayload(payload, AttributeId::Ticket);
    if (!np_id) {
        LOG_ERROR(Lib_NpMatching2, "matching2 sign in has no np id");
        QueueReply(connection, header, ORBIS_NP_MATCHING2_SERVER_ERROR_BAD_REQUEST,
                   std::span<const u8>{});
        return;
    }
    const s32 user_id = Np::NpHandler::Instance().GetUserIdByOnlineId(*np_id);
    LOG_INFO(Lib_NpMatching2, "matching2 sign in for {} (user {}) on {} with {}, ticket size {}",
             Np::ViewNpOnlineId(*np_id), user_id,
             environment ? std::string(environment->begin(), environment->end()) : "(none)",
             comm_id ? std::string(comm_id->begin(), comm_id->end()) : "(none)",
             ticket ? ticket->size() : 0);
    if (user_id == -1) {
        QueueReply(connection, header, ORBIS_NP_MATCHING2_SERVER_ERROR_NO_SUCH_USER,
                   std::span<const u8>{});
        return;
    }

    // Lets shadNet clean up this user's rooms if the game closes without leaving them.
    Np::NpHandler::Instance().StartMatching2Context(user_id);

    SignInReply reply{};
    reply.sign_in_id_header = MakePayloadHeader(AttributeId::SignInId, sizeof(reply.sign_in_id));
    reply.sign_in_id = static_cast<u32>(user_id);
    reply.data1_header = MakePayloadHeader(AttributeId::SignInData1, sizeof(reply.data1));
    reply.data2_header = MakePayloadHeader(AttributeId::SignInData2, sizeof(reply.data2));
    reply.session_token_header =
        MakePayloadHeader(AttributeId::SessionToken, sizeof(reply.session_token));
    reply.keep_alive_interval_header =
        MakePayloadHeader(AttributeId::KeepAliveInterval, sizeof(reply.keep_alive_interval));
    reply.keep_alive_interval = KeepAliveInterval;
    QueueReply(connection, header, 0, reply);
}

// These opcodes are effectively a keep-alive the library does.
void FakeMatching2::RenewSession(Connection& connection, const RequestHeader& header,
                                 std::span<const u8> payload) {
    // shadNet doesn't emulate sessions, so reply with a blank token.
    RenewSessionReply reply{};
    reply.session_token_header =
        MakePayloadHeader(AttributeId::SessionToken, sizeof(reply.session_token));
    QueueReply(connection, header, 0, reply);
}

void FakeMatching2::GetWorldInfoList(Connection& connection, const RequestHeader& header,
                                     std::span<const u8> payload) {
    const s32 user_id = static_cast<s32>(static_cast<u32>(header.sign_in_id));
    Np::NpMatching2::OrbisNpMatching2GetWorldInfoListRequest request{};
    request.server_id = connection.server_id;
    Np::NpHandler::Instance().GetWorldInfoList(
        user_id, request,
        [this, socket = connection.socket,
         header](s32 result, std::span<const Np::NpMatching2::OrbisNpMatching2World> worlds) {
            std::vector<u8> reply;
            if (result >= 0) {
                const PayloadHeader list_header = MakePayloadHeader(
                    AttributeId::List, sizeof(ListHeader) + worlds.size() * sizeof(WorldItem));
                ListHeader list{};
                list.item_id = static_cast<u16>(AttributeId::World);
                list.count = static_cast<u16>(worlds.size());
                AppendBytes(reply, list_header);
                AppendBytes(reply, list);
                for (const auto& world : worlds) {
                    WorldItem item{};
                    item.header = MakePayloadHeader(AttributeId::World, sizeof(World));
                    item.world.world_id = static_cast<u16>(world.world_id);
                    item.world.lobbies = static_cast<u16>(world.lobbies_num);
                    item.world.max_lobby_members = world.max_lobby_members_num;
                    item.world.lobby_members = world.lobby_members_num;
                    item.world.rooms = static_cast<u16>(world.rooms_num);
                    item.world.room_members = world.room_members_num;
                    AppendBytes(reply, item);
                }
                LOG_INFO(Lib_NpMatching2, "matching2 world info list has {} worlds", worlds.size());
            }
            QueueLateReply(socket, header, result, reply);
        });
}

void FakeMatching2::GetRoomServer(Connection& connection, const RequestHeader& header,
                                  std::span<const u8> payload) {
    const auto world_id = ReadPayload<u16_be>(payload, AttributeId::WorldId);
    const auto room_number = ReadPayload<u16_be>(payload, AttributeId::RoomNumber);
    LOG_INFO(Lib_NpMatching2, "matching2 room server for world {}, room {}",
             world_id ? static_cast<u16>(*world_id) : 0,
             room_number ? static_cast<u16>(*room_number) : 0);

    std::vector<u8> reply;
    RoomServerKey key{};
    key.header = MakePayloadHeader(AttributeId::RoomServerKey, sizeof(key.key));
    key.key.room_number = room_number ? static_cast<u16>(*room_number) : 0;
    key.key.sign_in_id = header.sign_in_id;
    AppendBytes(reply, key);
    AppendServerList(reply);

    // Return the room number when joining
    if (room_number) {
        AppendBytes(reply, MakePayloadHeader(AttributeId::RoomNumber, sizeof(*room_number)));
        AppendBytes(reply, *room_number);
        QueueReply(connection, header, 0, std::span<const u8>{reply});
        return;
    }

    // We need to create a room for libSceNpMatching2, use the stored parameters for this.
    if (m_pending_creates.empty()) {
        LOG_ERROR(Lib_NpMatching2, "matching2 room server request with no room to create");
        QueueReply(connection, header, ORBIS_NP_MATCHING2_SERVER_ERROR_BAD_REQUEST,
                   std::span<const u8>{});
        return;
    }
    const auto params = m_pending_creates.front();
    m_pending_creates.pop_front();

    const u32 sign_in_id = header.sign_in_id;
    Np::NpHandler::Instance().CreateJoinRoom(
        static_cast<s32>(sign_in_id), header.request_id, *params,
        [this, socket = connection.socket, header, params, sign_in_id,
         reply](s32 result, const shadnet::CreateRoomReply& room) mutable {
            if (result < 0) {
                QueueLateReply(socket, header, result, {});
                return;
            }
            // shadNet's room ids are used as room numbers.
            if (room.room_id() > 0xffff) {
                LOG_ERROR(Lib_NpMatching2, "shadnet room id {} doesn't fit a room number",
                          room.room_id());
            }
            LOG_INFO(Lib_NpMatching2, "matching2 created room {}", room.room_id());

            u16_be room_number{};
            room_number = static_cast<u16>(room.room_id());
            AppendBytes(reply, MakePayloadHeader(AttributeId::RoomNumber, sizeof(room_number)));
            AppendBytes(reply, room_number);

            CreatedRoom created{room.room_id(), BuildCreateJoinRoomReply(room, *params)};
            created.member_id = static_cast<u16>(room.details().me_member_id());
            for (const auto& member : room.details().members()) {
                created.members[MemberSlot(member.member_id())] =
                    static_cast<u16>(member.member_id());
            }
            std::scoped_lock lk{m_mutex};
            m_created_rooms[sign_in_id] = std::move(created);
            QueueLateReply(socket, header, 0, reply);
        });
}

void FakeMatching2::SearchRoom(Connection& connection, const RequestHeader& header,
                               std::span<const u8> payload) {
    Np::NpMatching2::OrbisNpMatching2SearchRoomRequest request{};
    std::vector<Np::NpMatching2::OrbisNpMatching2IntSearchFilter> int_filters;
    std::vector<Np::NpMatching2::OrbisNpMatching2BinSearchFilter> bin_filters;
    std::deque<std::vector<u8>> bin_filter_data;
    std::vector<Np::NpMatching2::OrbisNpMatching2AttributeId> attr_ids;

    while (payload.size() >= sizeof(PayloadHeader)) {
        PayloadHeader attr_header{};
        std::memcpy(&attr_header, payload.data(), sizeof(attr_header));
        const u64 size = attr_header.size;
        if (sizeof(attr_header) + size > payload.size()) {
            break;
        }
        const auto data = payload.subspan(sizeof(attr_header), size);
        payload = payload.subspan(sizeof(attr_header) + size);

        switch (static_cast<AttributeId>(attr_header.id & AttributeIdMask)) {
        case AttributeId::WorldId:
            if (data.size() >= sizeof(u16_be)) {
                request.world_id = static_cast<u16>(data[0] << 8 | data[1]);
            }
            break;
        case AttributeId::LobbyId:
            if (data.size() >= sizeof(u16_be)) {
                request.lobby_id = static_cast<u16>(data[0] << 8 | data[1]);
            }
            break;
        case AttributeId::RangeFilter: {
            RangeFilter range{};
            if (data.size() >= sizeof(range)) {
                std::memcpy(&range, data.data(), sizeof(range));
                request.range_filter.start = range.start;
                request.range_filter.max = range.max;
            }
            break;
        }
        case AttributeId::FlagFilter: {
            u32_be filter{};
            if (data.size() >= sizeof(filter)) {
                std::memcpy(&filter, data.data(), sizeof(filter));
                request.flags1 = filter;
                request.flags2 =
                    ReadPayload<u32_be>(data.subspan(sizeof(filter)), AttributeId::RoomFlags)
                        .value_or(u32_be{});
            }
            break;
        }
        case AttributeId::IntSearchFilter: {
            // An operator, then the attribute to compare with.
            if (data.size() < 1 + sizeof(PayloadHeader)) {
                break;
            }
            PayloadHeader value_header{};
            std::memcpy(&value_header, data.data() + 1, sizeof(value_header));
            const auto value = data.subspan(1 + sizeof(value_header));
            auto& filter = int_filters.emplace_back();
            filter.search_operator = data[0];
            filter.attr.id = value_header.id & AttributeIdMask;
            for (u64 i = 0; i < std::min<u64>(value_header.size, value.size()); i++) {
                filter.attr.num = filter.attr.num << 8 | value[i];
            }
            break;
        }
        case AttributeId::BinSearchFilter: {
            if (data.size() < 1 + sizeof(PayloadHeader)) {
                break;
            }
            PayloadHeader value_header{};
            std::memcpy(&value_header, data.data() + 1, sizeof(value_header));
            const auto value = data.subspan(1 + sizeof(value_header));
            auto& storage = bin_filter_data.emplace_back(
                value.begin(), value.begin() + std::min<u64>(value_header.size, value.size()));
            auto& filter = bin_filters.emplace_back();
            filter.search_operator = data[0];
            filter.attr.id = value_header.id & AttributeIdMask;
            filter.attr.data = storage.data();
            filter.attr.data_size = storage.size();
            break;
        }
        case AttributeId::RequestedAttributeId:
            if (data.size() >= sizeof(u16_be)) {
                attr_ids.push_back(static_cast<u16>((data[0] << 8 | data[1]) & AttributeIdMask));
            }
            break;
        default:
            break;
        }
    }
    request.int_filter = int_filters.data();
    request.int_filters = int_filters.size();
    request.bin_filter = bin_filters.data();
    request.bin_filters = bin_filters.size();
    request.attr = attr_ids.data();
    request.attrs = attr_ids.size();

    const s32 user_id = static_cast<s32>(static_cast<u32>(header.sign_in_id));
    Np::NpHandler::Instance().SearchRoom(
        user_id, request,
        [this, socket = connection.socket, header](s32 result,
                                                   const shadnet::SearchRoomReply& rooms) {
            if (result < 0) {
                QueueLateReply(socket, header, result, {});
                return;
            }
            std::vector<u8> reply;
            const u64 list_offset = BeginPayload(reply, AttributeId::RangeList);
            RangeListHeader list{};
            list.item_id = static_cast<u16>(AttributeId::RoomDataExternal);
            list.count = static_cast<u16>(rooms.rooms_size());
            list.range_start = static_cast<u16>(rooms.range_start());
            list.range_total = static_cast<u16>(rooms.range_total());
            AppendBytes(reply, list);
            for (const auto& room : rooms.rooms()) {
                AppendRoomDataExternal(reply, room);
            }
            EndPayload(reply, list_offset);
            LOG_INFO(Lib_NpMatching2, "matching2 search room found {} of {} rooms",
                     rooms.rooms_size(), rooms.range_total());
            QueueLateReply(socket, header, 0, reply);
        });
}

void FakeMatching2::GetRoomDataExternalList(Connection& connection, const RequestHeader& header,
                                            std::span<const u8> payload) {
    // Rooms come as room numbers in the request's world, which are shadNet's room ids.
    std::vector<Np::NpMatching2::OrbisNpMatching2RoomId> room_ids;
    std::vector<Np::NpMatching2::OrbisNpMatching2AttributeId> attr_ids;
    while (payload.size() >= sizeof(PayloadHeader)) {
        PayloadHeader attr_header{};
        std::memcpy(&attr_header, payload.data(), sizeof(attr_header));
        const u64 size = attr_header.size;
        if (sizeof(attr_header) + size > payload.size()) {
            break;
        }
        const auto data = payload.subspan(sizeof(attr_header), size);
        payload = payload.subspan(sizeof(attr_header) + size);
        if (data.size() < sizeof(u16_be)) {
            continue;
        }
        const u16 value = static_cast<u16>(data[0] << 8 | data[1]);
        switch (static_cast<AttributeId>(attr_header.id & AttributeIdMask)) {
        case AttributeId::RoomNumber:
            room_ids.push_back(value);
            break;
        case AttributeId::RequestedAttributeId:
            attr_ids.push_back(static_cast<u16>(value & AttributeIdMask));
            break;
        default:
            break;
        }
    }

    Np::NpMatching2::OrbisNpMatching2GetRoomDataExternalListRequest request{};
    request.room_id = room_ids.data();
    request.room_id_num = room_ids.size();
    request.attr_id = attr_ids.data();
    request.attr_id_num = attr_ids.size();

    const s32 user_id = static_cast<s32>(static_cast<u32>(header.sign_in_id));
    Np::NpHandler::Instance().GetRoomDataExternalList(
        user_id, request,
        [this, socket = connection.socket,
         header](s32 result, const shadnet::GetRoomDataExternalListReply& rooms) {
            if (result < 0) {
                QueueLateReply(socket, header, result, {});
                return;
            }
            std::vector<u8> reply;
            const u64 list_offset = BeginPayload(reply, AttributeId::List);
            ListHeader list{};
            list.item_id = static_cast<u16>(AttributeId::RoomDataExternal);
            list.count = static_cast<u16>(rooms.rooms_size());
            AppendBytes(reply, list);
            for (const auto& room : rooms.rooms()) {
                AppendRoomDataExternal(reply, room);
            }
            EndPayload(reply, list_offset);
            LOG_INFO(Lib_NpMatching2, "matching2 got data for {} rooms", rooms.rooms_size());
            QueueLateReply(socket, header, 0, reply);
        });
}

void FakeMatching2::RoomServerSignIn(Connection& connection, const RequestHeader& header,
                                     std::span<const u8> payload) {
    std::vector<u8> reply;
    AppendServerList(reply);
    QueueReply(connection, header, 0, std::span<const u8>{reply});
}

void FakeMatching2::CreateJoinRoom(Connection& connection, const RequestHeader& header,
                                   std::span<const u8> payload) {
    const auto it = m_created_rooms.find(header.sign_in_id);
    if (it == m_created_rooms.end()) {
        LOG_ERROR(Lib_NpMatching2, "matching2 create join room has no created room");
        QueueReply(connection, header, ORBIS_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM,
                   std::span<const u8>{});
        return;
    }
    connection.user_id = static_cast<s32>(static_cast<u32>(header.sign_in_id));
    connection.room_id = it->second.room_id;
    connection.member_id = it->second.member_id;
    connection.members = std::move(it->second.members);
    QueueReply(connection, header, 0, std::span<const u8>{it->second.reply});
    m_created_rooms.erase(it);
}

void FakeMatching2::JoinRoom(Connection& connection, const RequestHeader& header,
                             std::span<const u8> payload) {
    // The room and user are in the room server key we gave out for this room.
    const auto key = ReadPayload<RoomServerKeyData>(payload, AttributeId::RoomServerKey);
    if (!key || key->room_number == 0) {
        LOG_ERROR(Lib_NpMatching2, "matching2 join room has no room server key for a room");
        QueueReply(connection, header, ORBIS_NP_MATCHING2_SERVER_ERROR_BAD_REQUEST,
                   std::span<const u8>{});
        return;
    }
    const u16 room_number = key->room_number;

    // shadNet only needs to know whether a password or group label was given.
    static Np::NpMatching2::OrbisNpMatching2SessionPassword password{};
    static Np::NpMatching2::OrbisNpMatching2GroupLabel group_label{};
    Np::NpMatching2::OrbisNpMatching2JoinRoomRequest request{};

    // shadNet's room ids are used as room numbers.
    request.room_id = room_number;
    request.team_id = ReadPayload<u8>(payload, AttributeId::TeamId).value_or(0);
    if (FindPayload(payload, AttributeId::RoomPassword)) {
        request.room_passwd = &password;
    }
    if (FindPayload(payload, AttributeId::JoinGroupLabel)) {
        request.join_group_label = &group_label;
    }
    LOG_INFO(Lib_NpMatching2, "matching2 join room {}", request.room_id);

    const s32 user_id = static_cast<s32>(static_cast<u32>(key->sign_in_id));
    Np::NpHandler::Instance().JoinRoom(
        user_id, header.request_id, request,
        [this, socket = connection.socket, header, user_id](s32 result,
                                                            const shadnet::JoinRoomReply& room) {
            if (result < 0) {
                QueueLateReply(socket, header, result, {});
                return;
            }
            const auto& details = room.details();
            const u64 room_id = details.room_data().room_id();
            {
                std::scoped_lock lk{m_mutex};
                const auto it = m_connections.find(socket);
                if (it == m_connections.end()) {
                    return;
                }
                Connection& joined = it->second;
                joined.user_id = user_id;
                joined.room_id = room_id;
                joined.member_id = static_cast<u16>(details.me_member_id());
                joined.members.clear();
                for (const auto& member : details.members()) {
                    joined.members[MemberSlot(member.member_id())] =
                        static_cast<u16>(member.member_id());
                }
            }
            // Members already in the room signal with us
            for (const auto& member : details.members()) {
                if (member.member_id() != details.me_member_id()) {
                    FakeSignaling::Instance().OnMemberJoined(
                        room_id, static_cast<u16>(member.member_id()), member.addr(),
                        static_cast<u16>(member.port()));
                }
            }
            LOG_INFO(Lib_NpMatching2, "matching2 joined room {} as member {} with {} members",
                     room_id, details.me_member_id(), details.members_size());
            QueueLateReply(socket, header, 0, BuildJoinRoomReply(room));
        });
}

void FakeMatching2::PublishRoom(Connection& connection, const RequestHeader& header,
                                std::span<const u8> payload) {
    // Reply values are ignored
    QueueReply(connection, header, 0, std::span<const u8>{});
}

void FakeMatching2::JoinedRoom(Connection& connection, const RequestHeader& header,
                               std::span<const u8> payload) {
    // Reply values are ignored
    QueueReply(connection, header, 0, std::span<const u8>{});
}

void FakeMatching2::SetRoomDataExternal(Connection& connection, const RequestHeader& header,
                                        std::span<const u8> payload) {
    const auto room_number = ReadPayload<u16_be>(payload, AttributeId::RoomNumber);
    if (!room_number) {
        LOG_ERROR(Lib_NpMatching2, "matching2 set room data external has no room number");
        QueueReply(connection, header, ORBIS_NP_MATCHING2_SERVER_ERROR_BAD_REQUEST,
                   std::span<const u8>{});
        return;
    }
    const RoomAttributes attrs = ParseRoomAttributes(payload);

    // shadNet's room ids are used as room numbers.
    Np::NpMatching2::OrbisNpMatching2SetRoomDataExternalRequest request{};
    request.room_id = static_cast<u16>(*room_number);
    request.room_searchable_int_attr_external = attrs.search_int_attrs.data();
    request.room_searchable_int_attr_external_num = attrs.search_int_attrs.size();
    request.room_searchable_bin_attr_external = attrs.search_bin_attrs.data();
    request.room_searchable_bin_attr_external_num = attrs.search_bin_attrs.size();
    request.room_bin_attr_external = attrs.external_bin_attrs.data();
    request.room_bin_attr_external_num = attrs.external_bin_attrs.size();
    LOG_INFO(Lib_NpMatching2,
             "matching2 set room data external for room {}, {} int attrs, {} bin attrs",
             request.room_id, request.room_searchable_int_attr_external_num,
             request.room_searchable_bin_attr_external_num + request.room_bin_attr_external_num);

    // The reply has no values.
    Np::NpHandler::Instance().SetRoomDataExternal(
        static_cast<s32>(static_cast<u32>(header.sign_in_id)), header.request_id, request,
        [this, socket = connection.socket, header](s32 result) {
            QueueLateReply(socket, header, result, {});
        });
}

void FakeMatching2::SetRoomDataInternal(Connection& connection, const RequestHeader& header,
                                        std::span<const u8> payload) {
    if (connection.room_id == 0) {
        LOG_ERROR(Lib_NpMatching2, "matching2 set room data internal outside a room");
        QueueReply(connection, header, ORBIS_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM,
                   std::span<const u8>{});
        return;
    }
    const RoomAttributes attrs = ParseRoomAttributes(payload);

    // Group passwords, owner privilege ranks, and signaling parameters aren't supported by shadNet.
    Np::NpMatching2::OrbisNpMatching2SetRoomDataInternalRequest request{};
    request.room_id = connection.room_id;
    request.flag_filter = attrs.flag_filter.value_or(0);
    request.flag_attr = attrs.flag_attr;
    request.room_bin_attr_internal = attrs.internal_bin_attrs.data();
    request.room_bin_attr_internal_num = attrs.internal_bin_attrs.size();
    if (attrs.password_slot_mask) {
        request.password_slot_mask = &*attrs.password_slot_mask;
    }
    LOG_INFO(Lib_NpMatching2,
             "matching2 set room data internal for room {}, flags {:#x}/{:#x}, {} bin attrs",
             request.room_id, request.flag_attr, request.flag_filter,
             request.room_bin_attr_internal_num);

    Np::NpHandler::Instance().SetRoomDataInternal(
        connection.user_id, header.request_id, request,
        [this, socket = connection.socket, header](s32 result) {
            std::vector<u8> reply;
            if (result >= 0) {
                using namespace std::chrono;
                u64_be update_date{};
                update_date = static_cast<u64>(
                    duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
                AppendBytes(reply, MakePayloadHeader(AttributeId::UpdateDate, sizeof(update_date)));
                AppendBytes(reply, update_date);
            }
            QueueLateReply(socket, header, result, reply);
        });
}

void FakeMatching2::LeaveRoom(Connection& connection, const RequestHeader& header,
                              std::span<const u8> payload) {
    if (connection.room_id == 0) {
        LOG_ERROR(Lib_NpMatching2, "matching2 leave room outside a room");
        QueueReply(connection, header, ORBIS_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM,
                   std::span<const u8>{});
        return;
    }

    // shadNet doesn't take the presence option data.
    Np::NpMatching2::OrbisNpMatching2LeaveRoomRequest request{};
    request.room_id = connection.room_id;
    LOG_INFO(Lib_NpMatching2, "matching2 leave room {}", request.room_id);
    FakeSignaling::Instance().OnRoomLeft(connection.room_id);
    connection.room_id = 0;

    // The reply's optional RoomData1 makes the library update the lobby server, which isn't needed.
    Np::NpHandler::Instance().LeaveRoom(connection.user_id, header.request_id, request,
                                        [this, socket = connection.socket, header](s32 result) {
                                            QueueLateReply(socket, header, result, {});
                                        });
}

void FakeMatching2::SendRoomMessage(Connection& connection, const RequestHeader& header,
                                    std::span<const u8> payload) {
    const auto targets = FindPayload(payload, AttributeId::RoomMessageTargets);
    const auto data = FindPayload(payload, AttributeId::RoomMessage);
    if (connection.room_id == 0 || !targets || !data) {
        LOG_ERROR(Lib_NpMatching2, "matching2 send room message is missing its room or values");
        QueueReply(connection, header, ORBIS_NP_MATCHING2_SERVER_ERROR_BAD_REQUEST,
                   std::span<const u8>{});
        return;
    }
    // The reply lists the members the message went to.
    std::vector<u8> reply;
    AppendBlob(reply, AttributeId::RoomMessageTargets, *targets);
    QueueReply(connection, header, 0, std::span<const u8>{reply});

    if (header.internal == 0) {
        // Room messages aren't handled yet
        LOG_WARNING(Lib_NpMatching2, "matching2 room messages from games aren't forwarded yet");
        return;
    }

    // Internal messages are for signaling, we need to intercept these for cross-compatibility.
    for (u16 slot = 0; slot < targets->size() * 8; slot++) {
        if (((*targets)[slot / 8] & (0x80 >> (slot % 8))) == 0) {
            continue;
        }
        const auto member = connection.members.find(slot);
        if (member == connection.members.end()) {
            LOG_WARNING(Lib_NpMatching2, "matching2 room message for unknown member slot {}", slot);
            continue;
        }
        for (const auto& message : FakeSignaling::Instance().OnMessage(
                 connection.room_id, connection.member_id, member->second, *data)) {
            std::vector<u8> event;
            AppendMemberId(event, message.src_member_id);
            AppendBlob(event, AttributeId::RoomMessageTargets,
                       MemberSlotBitmap(message.dst_member_id));
            AppendBlob(event, AttributeId::RoomMessage, message.data);
            AppendRoomMessageCastType(event, RoomMessageCastType::Unicast);
            QueueEvent(connection, static_cast<u16>(RoomEventOpcode::RoomMessage), event, true);
        }
    }
}

void FakeMatching2::ReleaseRoomServer(Connection& connection, const RequestHeader& header,
                                      std::span<const u8> payload) {
    // Sent after leaving a room
    const auto room_number = ReadPayload<u16_be>(payload, AttributeId::RoomNumber);
    LOG_INFO(Lib_NpMatching2, "matching2 release room server for room {}",
             room_number ? static_cast<u16>(*room_number) : 0);
    QueueReply(connection, header, 0, std::span<const u8>{});
}

void FakeMatching2::OnCreateJoinRoom(
    std::shared_ptr<Np::NpMatching2::CreateJoinRoomParams> params) {
    std::scoped_lock lk{m_mutex};
    m_pending_creates.push_back(std::move(params));
}

void FakeMatching2::CancelCreateJoinRoom(const Np::NpMatching2::CreateJoinRoomParams* params) {
    std::scoped_lock lk{m_mutex};
    std::erase_if(m_pending_creates, [&](const auto& pending) { return pending.get() == params; });
}

} // namespace Libraries::Network
