// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/rtc/rtc.h"
#include "core/libraries/user_service/user_service.h"

namespace Libraries::Np::NpMatching2 {

// Constants for matching functions and structures
constexpr u32 ORBIS_NP_MATCHING2_SESSION_PASSWORD_SIZE = 8;
constexpr u32 ORBIS_NP_MATCHING2_GROUP_LABEL_SIZE = 8;

// Default initialization values
enum OrbisNpMatching2InitDefault : u64 {
    ORBIS_NP_MATCHING2_THREAD_STACK_SIZE_DEFAULT = 32768,
    ORBIS_NP_MATCHING2_POOLSIZE_DEFAULT = 131072,
    ORBIS_NP_MATCHING2_SSL_POOLSIZE_DEFAULT = 196608,
};

using OrbisNpMatching2AttributeId = u16;
using OrbisNpMatching2BlockKickFlag = u8;
using OrbisNpMatching2ContextId = u16;
using OrbisNpMatching2FlagAttr = u32;
using OrbisNpMatching2Flags = u32;
using OrbisNpMatching2LobbyId = u64;
using OrbisNpMatching2LobbyMemberId = u16;
using OrbisNpMatching2NatType = u8;
using OrbisNpMatching2Operator = u8;
using OrbisNpMatching2RequestId = u32;
using OrbisNpMatching2RoomGroupId = u8;
using OrbisNpMatching2RoomId = u64;
using OrbisNpMatching2RoomMemberId = u16;
using OrbisNpMatching2RoomPasswordSlotMask = u64;
using OrbisNpMatching2ServerId = u16;
using OrbisNpMatching2SessionType = u8;
using OrbisNpMatching2SignalingFlag = u8;
using OrbisNpMatching2SignalingRequestId = u32;
using OrbisNpMatching2TeamId = u8;
using OrbisNpMatching2WorldId = u32;

using OrbisNpMatching2CastType = u8;

enum : OrbisNpMatching2CastType {
    ORBIS_NP_MATCHING2_CASTTYPE_BROADCAST = 1,
    ORBIS_NP_MATCHING2_CASTTYPE_UNICAST = 2,
    ORBIS_NP_MATCHING2_CASTTYPE_MULTICAST = 3,
};

// Event of request/room/signaling/context functions
enum OrbisNpMatching2Event : u16 {
    ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_WORLD_INFO_LIST = 0x0002,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_MEMBER_DATA_EXTERNAL_LIST = 0x0003,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_ROOM_DATA_EXTERNAL = 0x0004,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_DATA_EXTERNAL_LIST = 0x0005,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_USER_INFO = 0x0007,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_USER_INFO_LIST = 0x0008,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM = 0x0101,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM = 0x0102,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_LEAVE_ROOM = 0x0103,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_KICKOUT_ROOM_MEMBER = 0x0104,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_GRANT_ROOM_OWNER = 0x0105,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_SEARCH_ROOM = 0x0106,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_SEND_ROOM_MESSAGE = 0x0108,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_ROOM_DATA_INTERNAL = 0x0109,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_DATA_INTERNAL = 0x010A,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_ROOM_MEMBER_DATA_INTERNAL = 0x010B,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_LOBBY = 0x0201,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_SIGNALING_GET_PING_INFO = 0x0E01,
    ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED = 0x1101,
    ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_LEFT = 0x1102,
    ORBIS_NP_MATCHING2_ROOM_EVENT_KICKEDOUT = 0x1103,
    ORBIS_NP_MATCHING2_ROOM_EVENT_ROOM_DESTROYED = 0x1104,
    ORBIS_NP_MATCHING2_ROOM_EVENT_ROOM_OWNER_CHANGED = 0x1105,
    ORBIS_NP_MATCHING2_ROOM_EVENT_UPDATED_ROOM_DATA_INTERNAL = 0x1106,
    ORBIS_NP_MATCHING2_ROOM_EVENT_UPDATED_ROOM_MEMBER_DATA_INTERNAL = 0x1107,
    ORBIS_NP_MATCHING2_ROOM_EVENT_UPDATED_SIGNALING_OPT_PARAM = 0x1108,
    ORBIS_NP_MATCHING2_SIGNALING_EVENT_DEAD = 0x5101,
    ORBIS_NP_MATCHING2_SIGNALING_EVENT_ESTABLISHED = 0x5102,
    ORBIS_NP_MATCHING2_SIGNALING_EVENT_NETINFO_ERROR = 0x5103,
    ORBIS_NP_MATCHING2_CONTEXT_EVENT_START_OVER = 0x6F01,
    ORBIS_NP_MATCHING2_CONTEXT_EVENT_STARTED = 0x6F02,
    ORBIS_NP_MATCHING2_CONTEXT_EVENT_STOPPED = 0x6F03,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM_A = 0x7101,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM_A = 0x7102,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_MEMBER_DATA_EXTERNAL_LIST_A = 0x7003,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_DATA_EXTERNAL_LIST_A = 0x7005,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_SEARCH_ROOM_A = 0x7106,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_MEMBER_DATA_INTERNAL_A = 0x710C,
    ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED_A = 0x8101,
    ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_LEFT_A = 0x8102,
    ORBIS_NP_MATCHING2_ROOM_EVENT_UPDATED_ROOM_MEMBER_DATA_INTERNAL_A = 0x8107,
    ORBIS_NP_MATCHING2_ROOM_MSG_EVENT_MESSAGE = 0x2102,
    ORBIS_NP_MATCHING2_ROOM_MSG_EVENT_MESSAGE_A = 0x9102,
};

// Event cause
enum OrbisNpMatching2EventCause : u8 {
    ORBIS_NP_MATCHING2_EVENT_CAUSE_LEAVE_ACTION = 1,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_KICKOUT_ACTION = 2,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_GRANT_OWNER_ACTION = 3,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_SERVER_OPERATION = 4,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_MEMBER_DISAPPEARED = 5,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_SERVER_INTERNAL = 6,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_CONNECTION_ERROR = 7,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_NP_SIGNED_OUT = 8,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_SYSTEM_ERROR = 9,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ERROR = 10,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION = 11,
};

// Signaling type
enum OrbisNpMatching2SignalingType : u8 {
    ORBIS_NP_MATCHING2_SIGNALING_TYPE_NONE = 0,
    ORBIS_NP_MATCHING2_SIGNALING_TYPE_MESH = 1,
    ORBIS_NP_MATCHING2_SIGNALING_TYPE_STAR = 2,
};

// Type of connection information to acquire
enum OrbisNpMatching2SignalingConnectionInfoType : s32 {
    ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_RTT = 1,
    ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_BANDWIDTH = 2,
    ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_NP_ID = 3,
    ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_ADDR = 4,
    ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_MAPPED_ADDR = 5,
    ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PACKET_LOSS = 6,
    ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_NPID = 7,
    ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_ADDRESS_A = 7,
};

// Attribute IDs of external/internal room and room member binary attributes
enum OrbisNpMatching2AttrId : OrbisNpMatching2AttributeId {
    ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_1_ID = 0x004C,
    ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_2_ID = 0x004D,
    ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_3_ID = 0x004E,
    ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_4_ID = 0x004F,
    ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_5_ID = 0x0050,
    ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_6_ID = 0x0051,
    ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_7_ID = 0x0052,
    ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_8_ID = 0x0053,
    ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_BIN_ATTR_EXTERNAL_1_ID = 0x0054,
    ORBIS_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_1_ID = 0x0055,
    ORBIS_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_2_ID = 0x0056,
    ORBIS_NP_MATCHING2_ROOM_BIN_ATTR_INTERNAL_1_ID = 0x0057,
    ORBIS_NP_MATCHING2_ROOM_BIN_ATTR_INTERNAL_2_ID = 0x0058,
    ORBIS_NP_MATCHING2_ROOMMEMBER_BIN_ATTR_INTERNAL_1_ID = 0x0059,
};

// Flag-type room attribute
enum OrbisNpMatching2RoomFlagAttr : OrbisNpMatching2Flags {
    ORBIS_NP_MATCHING2_ROOM_FLAG_ATTR_OWNER_AUTO_GRANT = 0x80000000,
    ORBIS_NP_MATCHING2_ROOM_FLAG_ATTR_CLOSED = 0x40000000,
    ORBIS_NP_MATCHING2_ROOM_FLAG_ATTR_FULL = 0x20000000,
    ORBIS_NP_MATCHING2_ROOM_FLAG_ATTR_HIDDEN = 0x10000000,
    ORBIS_NP_MATCHING2_ROOM_FLAG_ATTR_NAT_TYPE_RESTRICTION = 0x04000000,
    ORBIS_NP_MATCHING2_ROOM_FLAG_ATTR_PROHIBITIVE_MODE = 0x02000000,
};

// Flag-type room member attribute
enum OrbisNpMatching2RoomMemberFlagAttr : OrbisNpMatching2Flags {
    ORBIS_NP_MATCHING2_ROOMMEMBER_FLAG_ATTR_OWNER = 0x80000000,
};

// Member role
enum OrbisNpMatching2Role : u8 {
    ORBIS_NP_MATCHING2_ROLE_MEMBER = 1,
    ORBIS_NP_MATCHING2_ROLE_OWNER = 2,
};

// Context callback function
using OrbisNpMatching2ContextCallback = void (*)(OrbisNpMatching2ContextId context_id,
                                                 OrbisNpMatching2Event event,
                                                 OrbisNpMatching2EventCause cause, s32 error_code,
                                                 void* userdata);

// Lobby event callback function
using OrbisNpMatching2LobbyEventCallback = void (*)(OrbisNpMatching2ContextId context_id,
                                                    OrbisNpMatching2LobbyId lobby_id,
                                                    OrbisNpMatching2Event event, const void* data,
                                                    void* userdata);

// Lobby message callback function
using OrbisNpMatching2LobbyMessageCallback = void (*)(OrbisNpMatching2ContextId context_id,
                                                      OrbisNpMatching2LobbyId lobby_id,
                                                      OrbisNpMatching2LobbyMemberId src_member_id,
                                                      OrbisNpMatching2Event event, const void* data,
                                                      void* userdata);

// Request callback function
using OrbisNpMatching2RequestCallback = void (*)(OrbisNpMatching2ContextId,
                                                 OrbisNpMatching2RequestId, OrbisNpMatching2Event,
                                                 int, const void*, void*);

// Room event callback function
using OrbisNpMatching2RoomEventCallback = void (*)(OrbisNpMatching2ContextId context_id,
                                                   OrbisNpMatching2RoomId room_id,
                                                   OrbisNpMatching2Event event, const void* data,
                                                   void* userdata);

// Room message callback function
using OrbisNpMatching2RoomMessageCallback = void (*)(OrbisNpMatching2ContextId context_id,
                                                     OrbisNpMatching2RoomId room_id,
                                                     OrbisNpMatching2RoomMemberId src_member_id,
                                                     OrbisNpMatching2Event event, const void* data,
                                                     void* userdata);

// Signaling callback function
using OrbisNpMatching2SignalingCallback = void (*)(OrbisNpMatching2ContextId context_id,
                                                   OrbisNpMatching2RoomId room_id,
                                                   OrbisNpMatching2RoomMemberId room_member_id,
                                                   OrbisNpMatching2Event event, s32 error_code,
                                                   void* userdata);

struct OrbisNpMatching2RoomGroup;
struct OrbisNpMatching2RoomGroupInfo;
struct OrbisNpMatching2RoomBinAttrInternal;
struct OrbisNpMatching2RoomMemberBinAttrInternal;

// Session password
struct OrbisNpMatching2SessionPassword {
    u8 data[8];
};

// Group label
struct OrbisNpMatching2GroupLabel {
    u8 data[8];
};

// Optional presence data
struct OrbisNpMatching2PresenceOptionData {
    u8 data[16];
    u64 len;
};

// Integer-type attribute
struct OrbisNpMatching2IntAttr {
    OrbisNpMatching2AttributeId id;
    u8 padding[2];
    u32 num;
};

// Binary-type attribute
struct OrbisNpMatching2BinAttr {
    OrbisNpMatching2AttributeId id;
    u8 pad[6];
    u8* data;
    u64 data_size;
};

// Range filter
struct OrbisNpMatching2RangeFilter {
    u32 start;
    u32 max;
};

// Integer-type search condition
struct OrbisNpMatching2IntSearchFilter {
    OrbisNpMatching2Operator search_operator;
    u8 padding[7];
    OrbisNpMatching2IntAttr attr;
};

// Binary-type search condition
struct OrbisNpMatching2BinSearchFilter {
    OrbisNpMatching2Operator search_operator;
    u8 padding[7];
    OrbisNpMatching2BinAttr attr;
};

// Range of result
struct OrbisNpMatching2Range {
    u32 start;
    u32 total;
    u32 results;
    u8 pad[4];
};

// Signaling optional parameter
struct OrbisNpMatching2SignalingOptParam {
    OrbisNpMatching2SignalingType type;
    OrbisNpMatching2SignalingFlag flag;
    OrbisNpMatching2RoomMemberId member_id;
    u8 padding[4];
};

// Set groups in a room
struct OrbisNpMatching2RoomGroupConfig {
    u32 slots;
    bool has_label;
    OrbisNpMatching2GroupLabel label;
    bool has_password;
    u8 pad[2];
};

// Set group password
struct OrbisNpMatching2RoomGroupPasswordConfig {
    OrbisNpMatching2RoomGroupId group_id;
    bool has_password;
    u8 pad[1];
};

// Group (of slots in a room)
struct OrbisNpMatching2RoomGroup {
    OrbisNpMatching2RoomGroupId id;
    bool has_passwd;
    bool has_label;
    u8 pad;
    OrbisNpMatching2GroupLabel label;
    u32 slots;
    u32 group_members;
};

// Group information
struct OrbisNpMatching2RoomGroupInfo {
    OrbisNpMatching2RoomGroupId id;
    bool has_passwd;
    u8 pad[2];
    u32 slots;
    u32 group_members;
};

union OrbisNpMatching2RoomMessageDestination {
    OrbisNpMatching2RoomMemberId unicast_target;

    struct {
        OrbisNpMatching2RoomMemberId* member_id;
        u64 member_id_num;
    } multicast_target;
};
static_assert(sizeof(OrbisNpMatching2RoomMessageDestination) == 0x10);

struct OrbisNpMatching2SendRoomMessageRequest {
    OrbisNpMatching2RoomId room_id;
    OrbisNpMatching2CastType cast_type;
    u8 padding[3];
    OrbisNpMatching2RoomMessageDestination dst;
    const void* msg;
    u32 msg_len;
    s32 option;
};
static_assert(sizeof(OrbisNpMatching2SendRoomMessageRequest) == 0x30);

struct OrbisNpMatching2RoomMessageInfo {
    bool filtered;
    OrbisNpMatching2CastType cast_type;
    u8 padding[2];
    OrbisNpMatching2RoomMessageDestination* dst;
    OrbisNpId* src_member;
    void* msg;
    u32 msg_len;
};
static_assert(sizeof(OrbisNpMatching2RoomMessageInfo) == 0x28);

struct OrbisNpMatching2RoomMessageInfoA {
    bool filtered;
    OrbisNpMatching2CastType cast_type;
    u8 padding[6];
    OrbisNpPeerAddressA src_member;
    OrbisNpOnlineId src_member_online_id;
    OrbisNpMatching2RoomMessageDestination* dst;
    const void* msg;
    u64 msg_len;
};
static_assert(offsetof(OrbisNpMatching2RoomMessageInfoA, src_member) == 0x08);
static_assert(offsetof(OrbisNpMatching2RoomMessageInfoA, src_member_online_id) == 0x18);
static_assert(offsetof(OrbisNpMatching2RoomMessageInfoA, dst) == 0x30);
static_assert(offsetof(OrbisNpMatching2RoomMessageInfoA, msg) == 0x38);
static_assert(offsetof(OrbisNpMatching2RoomMessageInfoA, msg_len) == 0x40);
static_assert(sizeof(OrbisNpMatching2RoomMessageInfoA) == 0x48);

// Room-internal binary attribute
struct OrbisNpMatching2RoomBinAttrInternal {
    Rtc::OrbisRtcTick last_update;
    OrbisNpMatching2RoomMemberId member_id;
    u8 pad[6];
    OrbisNpMatching2BinAttr bin_attr;
};

// Room member internal binary attribute
struct OrbisNpMatching2RoomMemberBinAttrInternal {
    Rtc::OrbisRtcTick last_update;
    OrbisNpMatching2BinAttr bin_attr;
};

// Server
struct OrbisNpMatching2World {
    OrbisNpMatching2World* next;
    OrbisNpMatching2WorldId world_id;
    u32 lobbies_num;
    u32 max_lobby_members_num;
    u32 lobby_members_num;
    u32 rooms_num;
    u32 room_members_num;
    u8 pad[3];
};

// Room-internal room information
struct OrbisNpMatching2RoomDataInternal {
    u16 public_slots;
    u16 private_slots;
    u16 open_public_slots;
    u16 open_private_slots;
    u16 max_slot;
    OrbisNpMatching2ServerId server_id;
    OrbisNpMatching2WorldId world_id;
    OrbisNpMatching2LobbyId lobby_id;
    OrbisNpMatching2RoomId room_id;
    OrbisNpMatching2RoomPasswordSlotMask passwd_slot_mask;
    u64 joined_slot_mask;
    OrbisNpMatching2RoomGroup* room_group;
    u64 room_groups;
    OrbisNpMatching2Flags flags;
    u8 pad[4];
    OrbisNpMatching2RoomBinAttrInternal* internal_bin_attr;
    u64 internal_bin_attrs;
};

// Room-external room information
struct OrbisNpMatching2RoomDataExternal {
    OrbisNpMatching2RoomDataExternal* next;
    u16 max_slot;
    u16 cur_members;
    OrbisNpMatching2Flags flags;
    OrbisNpMatching2ServerId server_id;
    u8 pad[2];
    OrbisNpMatching2WorldId world_id;
    OrbisNpMatching2LobbyId lobby_id;
    OrbisNpMatching2RoomId room_id;
    u64 passwd_slot_mask;
    u64 joined_slot_mask;
    u16 public_slots;
    u16 private_slots;
    u16 open_public_slots;
    u16 open_private_slots;
    OrbisNpId* owner_np_id;
    OrbisNpMatching2RoomGroupInfo* room_group;
    u64 room_groups;
    OrbisNpMatching2IntAttr* external_search_int_attr;
    u64 external_search_int_attrs;
    OrbisNpMatching2BinAttr* external_search_bin_attr;
    u64 external_search_bin_attrs;
    OrbisNpMatching2BinAttr* external_bin_attr;
    u64 external_bin_attrs;
};

// Room-external room information (account-id variant)
struct OrbisNpMatching2RoomDataExternalA {
    OrbisNpMatching2RoomDataExternalA* next;
    u16 max_slot;
    u16 cur_members;
    OrbisNpMatching2Flags flags;
    OrbisNpMatching2ServerId server_id;
    u8 pad[2];
    OrbisNpMatching2WorldId world_id;
    OrbisNpMatching2LobbyId lobby_id;
    OrbisNpMatching2RoomId room_id;
    OrbisNpMatching2RoomPasswordSlotMask passwd_slot_mask;
    u64 joined_slot_mask;
    u16 public_slots;
    u16 private_slots;
    u16 open_public_slots;
    u16 open_private_slots;
    OrbisNpPeerAddressA owner;
    OrbisNpOnlineId owner_online_id;
    OrbisNpMatching2RoomGroupInfo* room_group;
    u64 room_groups;
    OrbisNpMatching2IntAttr* external_search_int_attr;
    u64 external_search_int_attrs;
    OrbisNpMatching2BinAttr* external_search_bin_attr;
    u64 external_search_bin_attrs;
    OrbisNpMatching2BinAttr* external_bin_attr;
    u64 external_bin_attrs;
};

// Room-internal room member information
struct OrbisNpMatching2RoomMemberDataInternal {
    OrbisNpMatching2RoomMemberDataInternal* next;
    u64 join_date;
    OrbisNpId np_id;
    u8 pad[4];
    OrbisNpMatching2RoomMemberId member_id;
    OrbisNpMatching2TeamId team_id;
    OrbisNpMatching2NatType nat_type;
    OrbisNpMatching2Flags flag_attr;
    OrbisNpMatching2RoomGroup* room_group;
    OrbisNpMatching2RoomMemberBinAttrInternal* room_member_bin_attr_internal;
    u64 room_member_bin_attr_internal_num;
};
static_assert(sizeof(OrbisNpMatching2RoomMemberDataInternal) == 0x58);

// Room-internal room member information (account-id variant)
struct OrbisNpMatching2RoomMemberDataInternalA {
    OrbisNpMatching2RoomMemberDataInternalA* next;
    Rtc::OrbisRtcTick join_date_ticks;
    OrbisNpPeerAddressA user;
    OrbisNpOnlineId online_id;
    u8 pad[4];
    OrbisNpMatching2RoomMemberId member_id;
    OrbisNpMatching2TeamId team_id;
    OrbisNpMatching2NatType nat_type;
    OrbisNpMatching2Flags flags;
    OrbisNpMatching2RoomGroup* room_group;
    OrbisNpMatching2RoomMemberBinAttrInternal* room_member_internal_bin_attr;
    u64 room_member_internal_bin_attrs;
};

// Room member ID list
struct OrbisNpMatching2RoomMemberDataInternalList {
    OrbisNpMatching2RoomMemberDataInternal* members;
    u64 members_num;
    OrbisNpMatching2RoomMemberDataInternal* me;
    OrbisNpMatching2RoomMemberDataInternal* owner;
};

// Room member ID list (account-id variant)
struct OrbisNpMatching2RoomMemberDataInternalListA {
    OrbisNpMatching2RoomMemberDataInternalA* members;
    u64 members_num;
    OrbisNpMatching2RoomMemberDataInternalA* me;
    OrbisNpMatching2RoomMemberDataInternalA* owner;
};

// Room-external room member information
struct OrbisNpMatching2RoomMemberDataExternal {
    OrbisNpMatching2RoomMemberDataExternal* next;
    OrbisNpId np_id;
    Rtc::OrbisRtcTick join_date;
    OrbisNpMatching2Role role;
    u8 padding[7];
};

// Room-external room member information (account-id variant)
struct OrbisNpMatching2RoomMemberDataExternalA {
    OrbisNpMatching2RoomMemberDataExternalA* next;
    OrbisNpPeerAddressA user;
    OrbisNpOnlineId online_id;
    Rtc::OrbisRtcTick join_date;
    OrbisNpMatching2Role role;
    u8 padding[7];
};

// Initialization parameter
struct OrbisNpMatching2InitializeParameter {
    u64 pool_size;
    u64 unknown;
    s32 priority;
    u8 padding[4];
    u64 stack_size;
    u64 size;
    u64 ssl_pool_size;
};
static_assert(sizeof(OrbisNpMatching2InitializeParameter) == 0x30);

// Create context parameter
struct OrbisNpMatching2CreateContextParameter {
    OrbisNpId* np_id;
    void* np_communication_id;
    void* np_passphrase;
    OrbisNpServiceLabel service_label;
    u64 size;
};
static_assert(sizeof(OrbisNpMatching2CreateContextParameter) == 0x28);

// Create context parameter (account-id variant)
struct OrbisNpMatching2CreateContextParameterA {
    UserService::OrbisUserServiceUserId user_id;
    OrbisNpServiceLabel service_label;
    u64 size;
};
static_assert(sizeof(OrbisNpMatching2CreateContextParameterA) == 16);

// Request optional parameter
struct OrbisNpMatching2RequestOptParam {
    OrbisNpMatching2RequestCallback callback;
    void* arg;
    u32 timeout;
    u16 app_id;
    u8 padding[2];
};

// GetWorldInfoList request
struct OrbisNpMatching2GetWorldInfoListRequest {
    OrbisNpMatching2ServerId server_id;
};

// GetWorldInfoList response
struct OrbisNpMatching2GetWorldInfoListResponse {
    OrbisNpMatching2World* world;
    u64 world_num;
};

// CreateJoinRoom request
struct OrbisNpMatching2CreateJoinRoomRequest {
    u16 max_slot;
    OrbisNpMatching2TeamId team_id;
    u8 pad[5];
    OrbisNpMatching2Flags flags;
    OrbisNpMatching2WorldId world_id;
    OrbisNpMatching2LobbyId lobby_id;
    void* room_passwd;
    OrbisNpMatching2RoomPasswordSlotMask* passwd_slot_mask;
    void* group_config;
    u64 group_configs;
    void* join_group_label;
    OrbisNpOnlineId* allowed_user;
    u64 allowed_users;
    OrbisNpOnlineId* blocked_user;
    u64 blocked_users;
    OrbisNpMatching2BinAttr* internal_bin_attr;
    u64 internal_bin_attrs;
    OrbisNpMatching2IntAttr* external_search_int_attr;
    u64 external_search_int_attrs;
    OrbisNpMatching2BinAttr* external_search_bin_attr;
    u64 external_search_bin_attrs;
    OrbisNpMatching2BinAttr* external_bin_attr;
    u64 external_bin_attrs;
    OrbisNpMatching2BinAttr* member_internal_bin_attr;
    u64 member_internal_bin_attrs;
    OrbisNpMatching2SignalingOptParam* signaling_param;
};
static_assert(sizeof(OrbisNpMatching2CreateJoinRoomRequest) == 184);

// CreateJoinRoom request (account-id variant)
struct OrbisNpMatching2CreateJoinRoomRequestA {
    u16 max_slot;
    OrbisNpMatching2TeamId team_id;
    u8 pad[5];
    OrbisNpMatching2Flags flags;
    OrbisNpMatching2WorldId world_id;
    OrbisNpMatching2LobbyId lobby_id;
    void* room_passwd;
    OrbisNpMatching2RoomPasswordSlotMask* passwd_slot_mask;
    void* group_config;
    u64 group_configs;
    void* join_group_label;
    OrbisNpAccountId* allowed_user;
    u64 allowed_users;
    OrbisNpAccountId* blocked_user;
    u64 blocked_users;
    OrbisNpMatching2BinAttr* internal_bin_attr;
    u64 internal_bin_attrs;
    OrbisNpMatching2IntAttr* external_search_int_attr;
    u64 external_search_int_attrs;
    OrbisNpMatching2BinAttr* external_search_bin_attr;
    u64 external_search_bin_attrs;
    OrbisNpMatching2BinAttr* external_bin_attr;
    u64 external_bin_attrs;
    OrbisNpMatching2BinAttr* member_internal_bin_attr;
    u64 member_internal_bin_attrs;
    OrbisNpMatching2SignalingOptParam* signaling_param;
};
static_assert(sizeof(OrbisNpMatching2CreateJoinRoomRequestA) == 184);

// CreateJoinRoom response
struct OrbisNpMatching2CreateJoinRoomResponse {
    const OrbisNpMatching2RoomDataInternal* room_data;
    OrbisNpMatching2RoomMemberDataInternalList members;
};

// CreateJoinRoom response (account-id variant)
struct OrbisNpMatching2CreateJoinRoomResponseA {
    OrbisNpMatching2RoomDataInternal* room_data;
    OrbisNpMatching2RoomMemberDataInternalListA members;
};

// JoinRoom request
struct OrbisNpMatching2JoinRoomRequest {
    OrbisNpMatching2RoomId room_id;
    OrbisNpMatching2SessionPassword* room_passwd;
    OrbisNpMatching2GroupLabel* join_group_label;
    OrbisNpMatching2BinAttr* room_member_bin_internal_attr;
    u64 room_member_bin_internal_attr_num;
    OrbisNpMatching2PresenceOptionData opt_data;
    OrbisNpMatching2TeamId team_id;
    u8 pad[3];
    OrbisNpMatching2Flags flags;
    OrbisNpOnlineId* blocked_user;
    u64 blocked_users;

    s32 Validate() {
        return 0;
    }
};
static_assert(sizeof(OrbisNpMatching2JoinRoomRequest) == 0x58);

// JoinRoom request (account-id variant)
struct OrbisNpMatching2JoinRoomRequestA {
    OrbisNpMatching2RoomId room_id;
    OrbisNpMatching2SessionPassword* room_passwd;
    OrbisNpMatching2GroupLabel* join_group_label;
    OrbisNpMatching2BinAttr* room_member_bin_internal_attr;
    u64 room_member_bin_internal_attr_num;
    OrbisNpMatching2PresenceOptionData opt_data;
    OrbisNpMatching2TeamId team_id;
    u8 pad[3];
    OrbisNpMatching2Flags flags;
    OrbisNpAccountId* blocked_user;
    u64 blocked_users;

    s32 Validate() {
        return 0;
    }
};
static_assert(sizeof(OrbisNpMatching2JoinRoomRequestA) == 0x58);

// LeaveRoom request
struct OrbisNpMatching2LeaveRoomRequest {
    OrbisNpMatching2RoomId room_id;
    OrbisNpMatching2PresenceOptionData opt_data;
};

// LeaveRoom response
struct OrbisNpMatching2LeaveRoomResponse {
    OrbisNpMatching2RoomId room_id;
};

// KickoutRoomMember request
struct OrbisNpMatching2KickoutRoomMemberRequest {
    OrbisNpMatching2RoomId room_id;
    OrbisNpMatching2RoomMemberId member_id;
    OrbisNpMatching2BlockKickFlag block_kick_flag;
    u8 padding[5];
    OrbisNpMatching2PresenceOptionData opt_data;
};
static_assert(sizeof(OrbisNpMatching2KickoutRoomMemberRequest) == 0x28);

// SearchRoom request
struct OrbisNpMatching2SearchRoomRequest {
    s32 option;
    OrbisNpMatching2WorldId world_id;
    OrbisNpMatching2LobbyId lobby_id;
    OrbisNpMatching2RangeFilter range_filter;
    OrbisNpMatching2Flags flags1;
    OrbisNpMatching2Flags flags2;
    OrbisNpMatching2IntSearchFilter* int_filter;
    u64 int_filters;
    OrbisNpMatching2BinSearchFilter* bin_filter;
    u64 bin_filters;
    OrbisNpMatching2AttributeId* attr;
    u64 attrs;
};

// SearchRoom response
struct OrbisNpMatching2SearchRoomResponse {
    OrbisNpMatching2Range range;
    OrbisNpMatching2RoomDataExternal* room_data_ext;
};

// SearchRoom response (account-id variant)
struct OrbisNpMatching2SearchRoomResponseA {
    OrbisNpMatching2Range range;
    OrbisNpMatching2RoomDataExternalA* room_data_ext;
};

// GetRoomDataExternalList request
struct OrbisNpMatching2GetRoomDataExternalListRequest {
    OrbisNpMatching2RoomId* room_id;
    u64 room_id_num;
    const OrbisNpMatching2AttributeId* attr_id;
    u64 attr_id_num;
};

// GetRoomDataExternalList response
struct OrbisNpMatching2GetRoomDataExternalListResponse {
    OrbisNpMatching2RoomDataExternal* room_data_external;
    u64 room_data_external_num;
};

// GetRoomDataExternalList response (account-id variant)
struct OrbisNpMatching2GetRoomDataExternalListResponseA {
    OrbisNpMatching2RoomDataExternalA* room_data_external;
    u64 room_data_external_num;
};

// GetRoomMemberDataExternalList request
struct OrbisNpMatching2GetRoomMemberDataExternalListRequest {
    OrbisNpMatching2RoomId room_id;
};

// GetRoomMemberDataExternalList response
struct OrbisNpMatching2GetRoomMemberDataExternalListResponse {
    OrbisNpMatching2RoomMemberDataExternal* room_member_data_external;
    u64 room_member_data_external_num;
};

// GetRoomMemberDataExternalList response (account-id variant)
struct OrbisNpMatching2GetRoomMemberDataExternalListResponseA {
    OrbisNpMatching2RoomMemberDataExternalA* room_member_data_external;
    u64 room_member_data_external_num;
};

// SetRoomDataExternal request
struct OrbisNpMatching2SetRoomDataExternalRequest {
    OrbisNpMatching2RoomId room_id;
    const OrbisNpMatching2IntAttr* room_searchable_int_attr_external;
    u64 room_searchable_int_attr_external_num;
    const OrbisNpMatching2BinAttr* room_searchable_bin_attr_external;
    u64 room_searchable_bin_attr_external_num;
    const OrbisNpMatching2BinAttr* room_bin_attr_external;
    u64 room_bin_attr_external_num;
};

// SetRoomDataInternal request
struct OrbisNpMatching2SetRoomDataInternalRequest {
    OrbisNpMatching2RoomId room_id;
    OrbisNpMatching2Flags flag_filter;
    OrbisNpMatching2Flags flag_attr;
    const OrbisNpMatching2BinAttr* room_bin_attr_internal;
    u64 room_bin_attr_internal_num;
    const OrbisNpMatching2RoomGroupPasswordConfig* password_config;
    u64 password_config_num;
    const OrbisNpMatching2RoomPasswordSlotMask* password_slot_mask;
    const OrbisNpMatching2RoomMemberId* owner_privilege_rank;
    u64 owner_privilege_rank_num;
};

// GetRoomDataInternal request
struct OrbisNpMatching2GetRoomDataInternalRequest {
    OrbisNpMatching2RoomId room_id;
    const OrbisNpMatching2AttributeId* attr_id;
    u64 attr_id_num;
};

// SetRoomMemberDataInternal request
struct OrbisNpMatching2SetRoomMemberDataInternalRequest {
    OrbisNpMatching2RoomId room_id;
    OrbisNpMatching2RoomMemberId member_id;
    OrbisNpMatching2TeamId team_id;
    u8 padding[5];
    OrbisNpMatching2FlagAttr flag_filter;
    OrbisNpMatching2FlagAttr flag_attr;
    const OrbisNpMatching2BinAttr* room_member_bin_attr_internal;
    u64 room_member_bin_attr_internal_num;
};
static_assert(sizeof(OrbisNpMatching2SetRoomMemberDataInternalRequest) == 0x28);

// GetRoomMemberDataInternal request
struct OrbisNpMatching2GetRoomMemberDataInternalRequest {
    OrbisNpMatching2RoomId room_id;
    OrbisNpMatching2RoomMemberId member_id;
    u8 padding[6];
    const OrbisNpMatching2AttributeId* attr_id;
    u64 attr_id_num;
};
static_assert(sizeof(OrbisNpMatching2GetRoomMemberDataInternalRequest) == 0x20);

// GetRoomMemberDataInternal response
struct OrbisNpMatching2GetRoomMemberDataInternalResponse {
    OrbisNpMatching2RoomMemberDataInternal* room_member_data_internal;
};

// GetRoomMemberDataInternal response (account-id variant)
struct OrbisNpMatching2GetRoomMemberDataInternalResponseA {
    OrbisNpMatching2RoomMemberDataInternalA* room_member_data_internal;
};

// SetUserInfo request
struct OrbisNpMatching2SetUserInfoRequest {
    OrbisNpMatching2ServerId server_id;
    u8 padding[6];
    OrbisNpMatching2BinAttr* user_bin_attr;
    u64 user_bin_attrs;
};

// Session a user is currently joined to
struct OrbisNpMatching2JoinedSessionInfo {
    OrbisNpMatching2SessionType session_type;
    u8 padding[1];
    OrbisNpMatching2ServerId server_id;
    OrbisNpMatching2WorldId world_id;
    OrbisNpMatching2LobbyId lobby_id;
    OrbisNpMatching2RoomId room_id;
    Rtc::OrbisRtcTick join_date;
};

// User information
// from PS3 this was NpUserInfo2; setting addresses like our other A and nonA types TODO: confirm
struct OrbisNpMatching2UserInfo {
    OrbisNpMatching2UserInfo* next;
    OrbisNpId np_id;
    OrbisNpMatching2BinAttr* user_bin_attr;
    u64 user_bin_attr_num;
    OrbisNpMatching2JoinedSessionInfo joined_session_info;
    u64 joined_session_info_num;
};

// User information (account-id variant)
// from PS3 this was NpUserInfo2; setting addresses like our other A and nonA types TODO: confirm
struct OrbisNpMatching2UserInfoA {
    OrbisNpMatching2UserInfoA* next;
    OrbisNpPeerAddressA user;
    OrbisNpOnlineId user_online_id;
    OrbisNpMatching2BinAttr* user_bin_attr;
    u64 user_bin_attr_num;
    OrbisNpMatching2JoinedSessionInfo joined_session_info;
    u64 joined_session_info_num;
};

// GetUserInfoList request
struct OrbisNpMatching2GetUserInfoListRequest {
    OrbisNpMatching2ServerId server_id;
    u8 padding[6];
    OrbisNpId* np_id;
    u64 np_id_num;
    const OrbisNpMatching2AttributeId* attr_id;
    u64 attr_id_num;
    s32 option;
};

// GetUserInfoList response
struct OrbisNpMatching2GetUserInfoListResponse {
    OrbisNpMatching2UserInfo* user_info;
    u64 user_info_num;
};

// GetUserInfoList response (account-id variant)
struct OrbisNpMatching2GetUserInfoListResponseA {
    OrbisNpMatching2UserInfoA* user_info;
    u64 user_info_num;
};

// Room data internal update information
struct OrbisNpMatching2RoomDataInternalUpdate {
    OrbisNpMatching2RoomDataInternal* chg_room_data_internal;
    OrbisNpMatching2FlagAttr* chg_flag_attr;
    OrbisNpMatching2FlagAttr* prev_flag_attr;
    OrbisNpMatching2RoomPasswordSlotMask* chg_room_password_slot_mask;
    OrbisNpMatching2RoomPasswordSlotMask* prev_room_password_slot_mask;
    OrbisNpMatching2RoomGroup** chg_room_group;
    u64 chg_room_group_num;
    OrbisNpMatching2RoomBinAttrInternal** chg_room_bin_attr_internal;
    u64 chg_room_bin_attr_internal_num;
};

// Room member data internal update information
struct OrbisNpMatching2RoomMemberDataInternalUpdate {
    OrbisNpMatching2RoomMemberDataInternal* chg_room_member_data_internal;
    OrbisNpMatching2FlagAttr* chg_flag_attr;
    OrbisNpMatching2FlagAttr* prev_flag_attr;
    OrbisNpMatching2TeamId* chg_team_id;
    OrbisNpMatching2RoomMemberBinAttrInternal** chg_room_member_bin_attr_internal;
    u64 chg_room_member_bin_attr_internal_num;
};
static_assert(sizeof(OrbisNpMatching2RoomMemberDataInternalUpdate) == 0x30);
static_assert(offsetof(OrbisNpMatching2RoomMemberDataInternalUpdate,
                       chg_room_member_bin_attr_internal) == 0x20);
static_assert(offsetof(OrbisNpMatching2RoomMemberDataInternalUpdate,
                       chg_room_member_bin_attr_internal_num) == 0x28);

// Room member data internal update information (account-id variant)
struct OrbisNpMatching2RoomMemberDataInternalUpdateA {
    OrbisNpMatching2RoomMemberDataInternalA* chg_room_member_data_internal;
    OrbisNpMatching2FlagAttr* chg_flag_attr;
    OrbisNpMatching2FlagAttr* prev_flag_attr;
    OrbisNpMatching2TeamId* chg_team_id;
    OrbisNpMatching2RoomMemberBinAttrInternal** chg_room_member_bin_attr_internal;
    u64 chg_room_member_bin_attr_internal_num;
};
static_assert(sizeof(OrbisNpMatching2RoomMemberDataInternalUpdateA) == 0x30);
static_assert(offsetof(OrbisNpMatching2RoomMemberDataInternalUpdateA,
                       chg_room_member_bin_attr_internal) == 0x20);
static_assert(offsetof(OrbisNpMatching2RoomMemberDataInternalUpdateA,
                       chg_room_member_bin_attr_internal_num) == 0x28);

// Room update information
struct OrbisNpMatching2RoomUpdate {
    OrbisNpMatching2EventCause event_cause;
    u8 padding[3];
    s32 error_code;
    OrbisNpMatching2PresenceOptionData opt_data;
};

// Room member update information
struct OrbisNpMatching2RoomMemberUpdate {
    OrbisNpMatching2RoomMemberDataInternal* room_member_data_internal;
    OrbisNpMatching2EventCause event_cause;
    u8 padding[7];
    OrbisNpMatching2PresenceOptionData opt_data;
};

// Room member update information (account-id variant)
struct OrbisNpMatching2RoomMemberUpdateA {
    OrbisNpMatching2RoomMemberDataInternalA* room_member_data_internal;
    OrbisNpMatching2EventCause event_cause;
    u8 padding[7];
    OrbisNpMatching2PresenceOptionData opt_data;
};

// Signaling connection information address
struct OrbisNpMatching2SignalingConnectionInfoAddr {
    u32 addr;
    u16 port;
    u8 pad[2];
};

// Signaling connection information
union OrbisNpMatching2SignalingConnectionInfo {
    u32 rtt;
    u32 bandwidth;
    OrbisNpId np_id;
    OrbisNpMatching2SignalingConnectionInfoAddr address;
    u32 packet_loss;
};
static_assert(sizeof(OrbisNpMatching2SignalingConnectionInfo) == 0x24);

// Signaling connection information (account-id variant)
union OrbisNpMatching2SignalingConnectionInfoA {
    u32 rtt;
    u32 bandwidth;
    OrbisNpPeerAddressA peer_addr_a;
    OrbisNpMatching2SignalingConnectionInfoAddr address;
    u32 packet_loss;
};
static_assert(sizeof(OrbisNpMatching2SignalingConnectionInfoA) == 0x10);

// SignalingGetPingInfo request
struct OrbisNpMatching2SignalingGetPingInfoRequest {
    OrbisNpMatching2RoomId room_id;
    u8 pad[16];

    s32 Validate() {
        return 0;
    }
};

// SignalingGetPingInfo response
struct OrbisNpMatching2SignalingGetPingInfoResponse {
    OrbisNpMatching2ServerId server_id;
    u8 pad[2];
    OrbisNpMatching2WorldId world_id;
    OrbisNpMatching2RoomId room_id;
    u32 rtt;
    u8 reserved[20];
};
static_assert(sizeof(OrbisNpMatching2SignalingGetPingInfoResponse) == 0x28);

// Signaling parameter
struct OrbisNpMatching2SignalingParam {
    s32 type;
    s32 flag;
    OrbisNpMatching2RoomMemberId main_member;
    u8 pad[4];
};

s32 sceNpMatching2CreateContext(const OrbisNpMatching2CreateContextParameter* param,
                                OrbisNpMatching2ContextId* ctx_id);
s32 sceNpMatching2CreateContextA(const OrbisNpMatching2CreateContextParameterA* param,
                                 OrbisNpMatching2ContextId* ctx_id);
s32 sceNpMatching2CreateJoinRoom(OrbisNpMatching2ContextId ctx_id,
                                 OrbisNpMatching2CreateJoinRoomRequest* request,
                                 OrbisNpMatching2RequestOptParam* request_opt,
                                 OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2JoinRoom(OrbisNpMatching2ContextId ctx_id,
                           OrbisNpMatching2JoinRoomRequest* request,
                           OrbisNpMatching2RequestOptParam* request_opt,
                           OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2CreateJoinRoomA(OrbisNpMatching2ContextId ctx_id,
                                  OrbisNpMatching2CreateJoinRoomRequestA* request,
                                  OrbisNpMatching2RequestOptParam* request_opt,
                                  OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2RegisterContextCallback(OrbisNpMatching2ContextCallback callback, void* userdata);
s32 sceNpMatching2RegisterLobbyEventCallback(OrbisNpMatching2ContextId ctx_id,
                                             OrbisNpMatching2LobbyEventCallback callback,
                                             void* userdata);
s32 sceNpMatching2RegisterRoomEventCallback(OrbisNpMatching2ContextId ctx_id,
                                            OrbisNpMatching2RoomEventCallback callback,
                                            void* userdata);
s32 sceNpMatching2RegisterSignalingCallback(OrbisNpMatching2ContextId ctx_id,
                                            OrbisNpMatching2SignalingCallback callback,
                                            void* userdata);
s32 sceNpMatching2ContextStart(OrbisNpMatching2ContextId ctx_id, u64 timeout);
s32 sceNpMatching2ContextStop(OrbisNpMatching2ContextId ctx_id);
s32 sceNpMatching2Initialize(OrbisNpMatching2InitializeParameter* param);
s32 sceNpMatching2Terminate();
s32 sceNpMatching2SetDefaultRequestOptParam(OrbisNpMatching2ContextId ctx_id,
                                            OrbisNpMatching2RequestOptParam* request_opt);
s32 sceNpMatching2GetServerId(OrbisNpMatching2ContextId ctx_id,
                              OrbisNpMatching2ServerId* server_id);
s32 sceNpMatching2GetWorldInfoList(OrbisNpMatching2ContextId ctx_id,
                                   OrbisNpMatching2GetWorldInfoListRequest* request,
                                   OrbisNpMatching2RequestOptParam* request_opt,
                                   OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2LeaveRoom(OrbisNpMatching2ContextId ctx_id,
                            OrbisNpMatching2LeaveRoomRequest* request,
                            OrbisNpMatching2RequestOptParam* request_opt,
                            OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2SearchRoom(OrbisNpMatching2ContextId ctx_id,
                             OrbisNpMatching2SearchRoomRequest* request,
                             OrbisNpMatching2RequestOptParam* request_opt,
                             OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2SetUserInfo(OrbisNpMatching2ContextId ctx_id,
                              OrbisNpMatching2SetUserInfoRequest* request,
                              OrbisNpMatching2RequestOptParam* request_opt,
                              OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2SendRoomMessage(OrbisNpMatching2ContextId ctx_id, void* request,
                                  OrbisNpMatching2RequestOptParam* request_opt,
                                  OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2SetRoomDataExternal(OrbisNpMatching2ContextId ctx_id,
                                      OrbisNpMatching2SetRoomDataExternalRequest* request,
                                      OrbisNpMatching2RequestOptParam* request_opt,
                                      OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2SetRoomDataInternal(OrbisNpMatching2ContextId ctx_id,
                                      OrbisNpMatching2SetRoomDataInternalRequest* request,
                                      OrbisNpMatching2RequestOptParam* request_opt,
                                      OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2SignalingGetConnectionStatus(OrbisNpMatching2ContextId ctx_id,
                                               OrbisNpMatching2RoomId room_id,
                                               OrbisNpMatching2RoomMemberId member_id,
                                               s32* conn_status, void* peer_addr, u16* peer_port);
s32 sceNpMatching2AbortContextStart(OrbisNpMatching2ContextId ctx_id);
s32 sceNpMatching2DestroyContext(OrbisNpMatching2ContextId ctx_id);
s32 sceNpMatching2RegisterLobbyMessageCallback(OrbisNpMatching2ContextId ctx_id,
                                               OrbisNpMatching2LobbyMessageCallback callback,
                                               void* userdata);
s32 sceNpMatching2RegisterRoomMessageCallback(OrbisNpMatching2ContextId ctx_id,
                                              OrbisNpMatching2RoomMessageCallback callback,
                                              void* userdata);
s32 sceNpMatching2GetMemoryInfo();
s32 sceNpMatching2GetSslMemoryInfo();
s32 sceNpMatching2GetRoomMemberIdListLocal();
s32 sceNpMatching2GetRoomPasswordLocal();
s32 sceNpMatching2GetSignalingOptParamLocal();
s32 sceNpMatching2GetLobbyInfoList();
s32 sceNpMatching2GetLobbyMemberDataInternal();
s32 sceNpMatching2GetLobbyMemberDataInternalList();
s32 sceNpMatching2GetRoomDataExternalList(OrbisNpMatching2ContextId ctx_id,
                                          OrbisNpMatching2GetRoomDataExternalListRequest* request,
                                          OrbisNpMatching2RequestOptParam* request_opt,
                                          OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2GetRoomDataInternal(OrbisNpMatching2ContextId ctx_id,
                                      OrbisNpMatching2GetRoomDataInternalRequest* request,
                                      OrbisNpMatching2RequestOptParam* request_opt,
                                      OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2GetRoomMemberDataExternalList(
    OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2GetRoomMemberDataExternalListRequest* request,
    OrbisNpMatching2RequestOptParam* request_opt, OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2GetRoomMemberDataInternal(
    OrbisNpMatching2ContextId ctx_id,
    const OrbisNpMatching2GetRoomMemberDataInternalRequest* request,
    const OrbisNpMatching2RequestOptParam* request_opt, OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2GetUserInfoListA(OrbisNpMatching2ContextId ctx_id,
                                   OrbisNpMatching2GetUserInfoListRequest* request,
                                   OrbisNpMatching2RequestOptParam* request_opt,
                                   OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2GetUserInfoList(OrbisNpMatching2ContextId ctx_id,
                                  OrbisNpMatching2GetUserInfoListRequest* request,
                                  OrbisNpMatching2RequestOptParam* request_opt,
                                  OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2GrantRoomOwner(OrbisNpMatching2ContextId ctx_id, void* req_param, void* opt_param,
                                 s32* req_id);
s32 sceNpMatching2JoinLobby(OrbisNpMatching2ContextId ctx_id, void* req_param, void* opt_param,
                            s32* req_id);
s32 sceNpMatching2JoinRoomA(OrbisNpMatching2ContextId ctx_id,
                            OrbisNpMatching2JoinRoomRequestA* request,
                            OrbisNpMatching2RequestOptParam* request_opt,
                            OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2KickoutRoomMember(OrbisNpMatching2ContextId ctx_id,
                                    OrbisNpMatching2KickoutRoomMemberRequest* request,
                                    OrbisNpMatching2RequestOptParam* request_opt,
                                    OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2LeaveLobby();
s32 sceNpMatching2SendLobbyChatMessage();
s32 sceNpMatching2SendRoomChatMessage(OrbisNpMatching2ContextId ctx_id, void* req_param,
                                      void* opt_param, s32* req_id);
s32 sceNpMatching2SetLobbyMemberDataInternal();
s32 sceNpMatching2SetRoomMemberDataInternal(
    OrbisNpMatching2ContextId ctx_id,
    const OrbisNpMatching2SetRoomMemberDataInternalRequest* request,
    const OrbisNpMatching2RequestOptParam* request_opt, OrbisNpMatching2RequestId* request_id);
s32 sceNpMatching2SetSignalingOptParam();
s32 sceNpMatching2SignalingGetPeerNetInfo();
s32 sceNpMatching2SignalingGetPingInfo(OrbisNpMatching2ContextId ctx_id, const void* req_param,
                                       const void* opt_param, s32* req_id);
s32 sceNpMatching2SignalingCancelPeerNetInfo();
s32 sceNpMatching2SignalingGetConnectionInfoA(OrbisNpMatching2ContextId ctx_id,
                                              OrbisNpMatching2RoomId room_id,
                                              OrbisNpMatching2RoomMemberId member_id, u32 info_type,
                                              void* conn_info);
s32 sceNpMatching2SignalingGetConnectionInfo(OrbisNpMatching2ContextId ctx_id,
                                             OrbisNpMatching2RoomId room_id,
                                             OrbisNpMatching2RoomMemberId member_id, u32 info_type,
                                             void* conn_info);
s32 sceNpMatching2SignalingGetLocalNetInfo(void* info);
s32 sceNpMatching2SignalingGetPeerNetInfoResult();

void RegisterHooks();
} // namespace Libraries::Np::NpMatching2
