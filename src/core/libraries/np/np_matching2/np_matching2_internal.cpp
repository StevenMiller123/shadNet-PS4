// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <arpa/inet.h>
#include <fmt/format.h>
#include <orbis/NpCommon.h>

#include "common/logging/log.h"
#include "common/thread.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_matching2/np_matching2_internal.h"
#include "shadnet.pb.h"

namespace Libraries::Np::NpMatching2 {

static NpMatching2State m_state{};
NpMatching2State* GetState() {
    return &m_state;
}

static ContextManager m_contexts{};
ContextManager* GetContextManager() {
    return &m_contexts;
}

std::mutex cache_lock{};
std::map<OrbisNpMatching2ContextId, std::shared_ptr<Matching2ContextCache>> m_cache;

Matching2CacheGuard LockCache(OrbisNpMatching2ContextId ctx_id) {
    std::shared_ptr<Matching2ContextCache> cache;
    {
        std::lock_guard lock(cache_lock);
        auto& entry = m_cache[ctx_id];
        if (!entry) {
            entry = std::make_shared<Matching2ContextCache>();
        }
        cache = entry;
    }
    return Matching2CacheGuard(std::move(cache));
}

void ResetCache(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(cache_lock);
    m_cache.erase(ctx_id);
}

void ResetCaches() {
    std::lock_guard lock(cache_lock);
    m_cache.clear();
}

std::string HexPreview(const void* data, u64 size, u64 max_bytes = 32) {
    if (!data || size == 0) {
        return {};
    }

    const auto* bytes = static_cast<const u8*>(data);
    const u64 count = std::min(size, max_bytes);
    std::string out;
    out.reserve(count * 3 + 4);
    for (u64 i = 0; i < count; ++i) {
        char buf[4]{};
        std::snprintf(buf, sizeof(buf), "%02x", bytes[i]);
        if (i != 0) {
            out.push_back(' ');
        }
        out.append(buf);
    }
    if (size > count) {
        out.append(" ...");
    }
    return out;
}

std::string HexPreview(std::string_view data, u64 max_bytes = 32) {
    return HexPreview(data.data(), data.size(), max_bytes);
}

u32 IpStringToAddr(std::string_view ip) {
    u32 a, b, c, d;
    if (std::sscanf(std::string(ip).c_str(), "%u.%u.%u.%u", &a, &b, &c, &d) != 4) {
        return 0;
    }
    return a | (b << 8) | (c << 16) | (d << 24);
}

std::string OnlineIdToString(const OrbisNpOnlineId& online_id) {
    return std::string(online_id.data, strnlen(online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH));
}

template <typename Reply>
void ReserveExternalRoomPayloadStorage(CallbackPayload& p, const Reply& resp) {
    u64 groups = 0;
    u64 int_attrs = 0;
    u64 bin_attrs = 0;
    u64 bin_buffers = 0;
    u64 owners = 0;

    for (s32 i = 0; i < resp.rooms_size(); ++i) {
        const auto& r = resp.rooms(i);
        groups += static_cast<u64>(r.groups_size());
        int_attrs += static_cast<u64>(r.external_search_int_attrs_size());
        const u64 room_bin_attrs = static_cast<u64>(r.external_search_bin_attrs_size()) +
                                   static_cast<u64>(r.external_bin_attrs_size());
        bin_attrs += room_bin_attrs;
        bin_buffers += room_bin_attrs;
        if (!r.owner_npid().empty()) {
            ++owners;
        }
    }

    p.ext_room_groups.reserve(groups);
    p.ext_int_attrs.reserve(int_attrs);
    p.ext_bin_attrs.reserve(bin_attrs);
    p.bin_buffers.reserve(bin_buffers);
    p.ext_owner_npids.reserve(owners);
}

void BuildCreateJoinRoomPayloadCommon(ContextObject& ctx, CallbackPayload& p,
                                      const shadnet::CreateJoinRoomResponse& resp) {
    const auto& rd = resp.room_data();
    auto cache = LockCache(ctx.ctx_id);

    LOG_DEBUG(Lib_NpMatching2,
              "room={} me={} owner={} max={} public/open={}/{} "
              "private/open={}/{} flags={:#x} passwd_mask={:#x} joined_mask={:#x} groups={} "
              "internal_bin_attrs={} members={}",
              rd.room_id(), resp.me_member_id(), resp.owner_member_id(), rd.max_slot(),
              rd.public_slots(), rd.open_public_slots(), rd.private_slots(),
              rd.open_private_slots(), rd.flags(), rd.passwd_slot_mask(), rd.joined_slot_mask(),
              rd.groups_size(), rd.bin_attrs_internal_size(), resp.members_size());

    p.room_groups.resize(rd.groups_size());
    for (s32 i = 0; i < rd.groups_size(); ++i) {
        const auto& g = rd.groups(i);
        LOG_DEBUG(
            Lib_NpMatching2,
            "  room_group[{}] id={} slots={} members={} has_passwd={} has_label={} label_size={}",
            i, g.group_id(), g.slot_count(), g.num_members(), g.has_passwd(), g.has_label(),
            g.label().size());
        auto& dst = p.room_groups[i];
        dst.id = static_cast<OrbisNpMatching2RoomGroupId>(g.group_id());
        dst.has_passwd = g.has_passwd();
        dst.has_label = g.has_label();
        std::memset(dst.label.data, 0, sizeof(dst.label.data));
        std::memcpy(dst.label.data, g.label().data(),
                    std::min(g.label().size(), sizeof(dst.label.data)));
        dst.slots = g.slot_count();
        dst.group_members = g.num_members();
    }

    p.room_bin_attrs.resize(rd.bin_attrs_internal_size());
    for (s32 i = 0; i < rd.bin_attrs_internal_size(); ++i) {
        const auto& a = rd.bin_attrs_internal(i);
        LOG_DEBUG(
            Lib_NpMatching2,
            "  room_internal_bin[{}] id={:#x} update_member={} update_date={} size={} data={}", i,
            a.attr_id(), a.update_member_id(), a.update_date(), a.data().size(),
            HexPreview(a.data(), 128));
        p.bin_buffers.emplace_back(a.data().begin(), a.data().end());
        auto& buf = p.bin_buffers.back();
        auto& dst = p.room_bin_attrs[i];
        dst = {};
        dst.last_update.tick = a.update_date();
        dst.member_id = static_cast<OrbisNpMatching2RoomMemberId>(a.update_member_id());
        dst.bin_attr.id = static_cast<OrbisNpMatching2AttributeId>(a.attr_id());
        dst.bin_attr.data = buf.empty() ? nullptr : buf.data();
        dst.bin_attr.data_size = buf.size();
    }
    p.room_data = std::make_unique<OrbisNpMatching2RoomDataInternal>();
    auto& room = *p.room_data;
    room = {};
    room.public_slots = static_cast<u16>(rd.public_slots());
    room.private_slots = static_cast<u16>(rd.private_slots());
    room.open_public_slots = static_cast<u16>(rd.open_public_slots());
    room.open_private_slots = static_cast<u16>(rd.open_private_slots());
    room.max_slot = static_cast<u16>(rd.max_slot());
    room.server_id = static_cast<OrbisNpMatching2ServerId>(rd.server_id());
    room.world_id = rd.world_id();
    room.lobby_id = rd.lobby_id();
    room.room_id = rd.room_id();
    room.passwd_slot_mask = rd.passwd_slot_mask();
    room.joined_slot_mask = rd.joined_slot_mask();
    room.flags = rd.flags();
    room.room_group = p.room_groups.empty() ? nullptr : p.room_groups.data();
    room.room_groups = p.room_groups.size();
    room.internal_bin_attr = p.room_bin_attrs.empty() ? nullptr : p.room_bin_attrs.data();
    room.internal_bin_attrs = p.room_bin_attrs.size();

    if (ctx.room_id != 0 && ctx.room_id != room.room_id) {
        cache->rooms.erase(ctx.room_id);
    }
    cache->peers.clear();

    RoomCache& rc = cache->rooms[room.room_id];
    rc = RoomCache{};
    rc.num_slots = room.max_slot;
    rc.mask_password = room.passwd_slot_mask;
    rc.server_id = room.server_id;
    rc.world_id = room.world_id;
    rc.lobby_id = room.lobby_id;
    rc.room_id = room.room_id;
    rc.max_slot = room.max_slot;
    rc.public_slots = room.public_slots;
    rc.private_slots = room.private_slots;
    rc.open_public_slots = room.open_public_slots;
    rc.open_private_slots = room.open_private_slots;
    rc.passwd_slot_mask = room.passwd_slot_mask;
    rc.joined_slot_mask = room.joined_slot_mask;
    rc.flags = room.flags;
    rc.owner = resp.me_member_id() == resp.owner_member_id();
    rc.signaling_type = ORBIS_NP_MATCHING2_SIGNALING_TYPE_MESH;
    rc.signaling_main_member = static_cast<OrbisNpMatching2RoomMemberId>(resp.owner_member_id());
    rc.bin_attrs_internal.resize(p.room_bin_attrs.size());
    rc.bin_buffers.clear();
    rc.bin_buffers.reserve(p.room_bin_attrs.size());
    for (u64 i = 0; i < p.room_bin_attrs.size(); ++i) {
        const auto& a = p.room_bin_attrs[i].bin_attr;
        if (a.data && a.data_size > 0) {
            rc.bin_buffers.emplace_back(a.data, a.data + a.data_size);
        } else {
            rc.bin_buffers.emplace_back();
        }
        auto& buf = rc.bin_buffers.back();
        auto& dst = rc.bin_attrs_internal[i];
        dst = OrbisNpMatching2RoomBinAttrInternal{};
        dst.last_update = p.room_bin_attrs[i].last_update;
        dst.member_id = p.room_bin_attrs[i].member_id;
        dst.bin_attr.id = a.id;
        dst.bin_attr.data = buf.empty() ? nullptr : buf.data();
        dst.bin_attr.data_size = buf.size();
    }
    ctx.room_id = room.room_id;
    ctx.my_member_id = static_cast<OrbisNpMatching2RoomMemberId>(resp.me_member_id());
    ctx.is_room_owner = rc.owner;
    for (const auto& g : p.room_groups) {
        rc.groups[g.id] = g;
    }
    const s32 member_count = resp.members_size();
    for (s32 i = 0; i < member_count; ++i) {
        const auto& m = resp.members(i);
        LOG_DEBUG(Lib_NpMatching2,
                  "  member[{}] id={} npid='{}' owner={} team={} nat={} flags={:#x} group={} "
                  "addr={}:{} account={} platform={} bins={}",
                  i, m.member_id(), m.npid(), m.is_owner(), m.team_id(), m.nat_type(),
                  m.flag_attr(), m.group_id(), m.addr(), m.port(), m.account_id(), m.platform(),
                  m.bin_attrs_internal_size());
        MemberCache& mc = rc.members[static_cast<OrbisNpMatching2RoomMemberId>(m.member_id())];
        mc = MemberCache{};
        mc.member_id = static_cast<OrbisNpMatching2RoomMemberId>(m.member_id());
        mc.team_id = static_cast<OrbisNpMatching2TeamId>(m.team_id());
        mc.nat_type = static_cast<OrbisNpMatching2NatType>(m.nat_type());
        mc.flag_attr = m.flag_attr();
        mc.group_id = static_cast<OrbisNpMatching2RoomGroupId>(m.group_id());
        mc.join_date = m.join_date();
        mc.addr = IpStringToAddr(m.addr());
        mc.port = htons(static_cast<u16>(m.port()));
        SetNpId(mc.np_id, m.npid());
        mc.account_id = static_cast<OrbisNpAccountId>(m.account_id());
        mc.platform = static_cast<OrbisNpPlatformType>(m.platform());
        for (const auto& a : m.bin_attrs_internal()) {
            LOG_INFO(Lib_NpMatching2, "    memberBin id={:#x} update_date={} size={} data={}",
                     a.attr_id(), a.update_date(), a.data().size(), HexPreview(a.data(), 128));
            MemberBinCache& b = mc.bins[a.attr_id()];
            b.id = static_cast<OrbisNpMatching2AttributeId>(a.attr_id());
            b.update_date = a.update_date();
            b.data.assign(a.data().begin(), a.data().end());
        }
        if (mc.member_id != ctx.my_member_id) {
            PeerInfo peer{};
            peer.member_id = mc.member_id;
            peer.addr = mc.addr;
            peer.port = mc.port;
            SetNpOnlineId(peer.online_id, m.npid());
            cache->peers[mc.member_id] = peer;
        }
    }
}

OrbisNpMatching2RoomGroup* FindPayloadGroup(CallbackPayload& p, OrbisNpMatching2RoomGroupId id) {
    for (auto& group : p.room_groups) {
        if (group.id == id) {
            return &group;
        }
    }
    return nullptr;
}

void ReserveMemberBinAttrs(CallbackPayload& p, const shadnet::CreateJoinRoomResponse& resp) {
    u64 total = 0;
    for (const auto& member : resp.members()) {
        total += static_cast<u64>(member.bin_attrs_internal_size());
    }
    p.member_bin_attrs.reserve(total);
}

OrbisNpMatching2RoomMemberBinAttrInternal* AppendMemberBinAttrs(
    CallbackPayload& p, const shadnet::MatchingRoomMemberData& member, u64& count) {
    const u64 first = p.member_bin_attrs.size();
    count = static_cast<u64>(member.bin_attrs_internal_size());
    for (const auto& a : member.bin_attrs_internal()) {
        p.bin_buffers.emplace_back(a.data().begin(), a.data().end());
        auto& buf = p.bin_buffers.back();
        auto& dst = p.member_bin_attrs.emplace_back();
        dst = OrbisNpMatching2RoomMemberBinAttrInternal{};
        dst.last_update.tick = a.update_date();
        dst.bin_attr.id = static_cast<OrbisNpMatching2AttributeId>(a.attr_id());
        dst.bin_attr.data = buf.empty() ? nullptr : buf.data();
        dst.bin_attr.data_size = buf.size();
    }
    return count == 0 ? nullptr : &p.member_bin_attrs[first];
}

void* BuildCreateJoinRoomPayload(ContextObject& ctx, CallbackPayload& p,
                                 const shadnet::CreateJoinRoomResponse& resp) {
    p.Reset();
    BuildCreateJoinRoomPayloadCommon(ctx, p, resp);

    const s32 member_count = resp.members_size();
    p.member_data.resize(member_count);
    ReserveMemberBinAttrs(p, resp);
    for (s32 i = 0; i < member_count; ++i) {
        const auto& m = resp.members(i);
        auto& dst = p.member_data[i];
        dst = OrbisNpMatching2RoomMemberDataInternal{};
        dst.next = (i + 1 < member_count) ? &p.member_data[i + 1] : nullptr;
        dst.join_date = m.join_date();
        SetNpId(dst.np_id, m.npid());
        dst.member_id = static_cast<OrbisNpMatching2RoomMemberId>(m.member_id());
        dst.team_id = static_cast<OrbisNpMatching2TeamId>(m.team_id());
        dst.nat_type = static_cast<OrbisNpMatching2NatType>(m.nat_type());
        dst.flag_attr = m.flag_attr();
        if (m.is_owner()) {
            dst.flag_attr |= ORBIS_NP_MATCHING2_ROOMMEMBER_FLAG_ATTR_OWNER;
        }
        dst.room_group =
            FindPayloadGroup(p, static_cast<OrbisNpMatching2RoomGroupId>(m.group_id()));
        dst.room_member_bin_attr_internal =
            AppendMemberBinAttrs(p, m, dst.room_member_bin_attr_internal_num);
    }

    p.create_join_response = std::make_unique<OrbisNpMatching2CreateJoinRoomResponse>();
    auto& out = *p.create_join_response;
    out.room_data = p.room_data.get();
    out.members.members = p.member_data.empty() ? nullptr : p.member_data.data();
    out.members.members_num = p.member_data.size();
    out.members.me = nullptr;
    out.members.owner = nullptr;
    for (auto& md : p.member_data) {
        if (md.member_id == static_cast<OrbisNpMatching2RoomMemberId>(resp.me_member_id())) {
            out.members.me = &md;
        }
        if (md.member_id == static_cast<OrbisNpMatching2RoomMemberId>(resp.owner_member_id())) {
            out.members.owner = &md;
        }
    }

    p.request_data = p.create_join_response.get();
    return p.request_data;
}

void* BuildCreateJoinRoomPayloadA(ContextObject& ctx, CallbackPayload& p,
                                  const shadnet::CreateJoinRoomResponse& resp) {
    p.Reset();
    BuildCreateJoinRoomPayloadCommon(ctx, p, resp);

    const s32 member_count = resp.members_size();
    p.member_data_a.resize(member_count);
    ReserveMemberBinAttrs(p, resp);
    for (s32 i = 0; i < member_count; ++i) {
        const auto& m = resp.members(i);
        auto& dst = p.member_data_a[i];
        dst = OrbisNpMatching2RoomMemberDataInternalA{};
        dst.next = (i + 1 < member_count) ? &p.member_data_a[i + 1] : nullptr;
        dst.join_date_ticks.tick = m.join_date();
        dst.user.account_id = static_cast<OrbisNpAccountId>(m.account_id());
        dst.user.platform = static_cast<OrbisNpPlatformType>(m.platform());
        SetNpOnlineId(dst.online_id, m.npid());
        dst.member_id = static_cast<OrbisNpMatching2RoomMemberId>(m.member_id());
        dst.team_id = static_cast<OrbisNpMatching2TeamId>(m.team_id());
        dst.nat_type = static_cast<OrbisNpMatching2NatType>(m.nat_type());
        dst.flags = m.flag_attr();
        if (m.is_owner()) {
            dst.flags |= ORBIS_NP_MATCHING2_ROOMMEMBER_FLAG_ATTR_OWNER;
        }
        dst.room_group =
            FindPayloadGroup(p, static_cast<OrbisNpMatching2RoomGroupId>(m.group_id()));
        dst.room_member_internal_bin_attr =
            AppendMemberBinAttrs(p, m, dst.room_member_internal_bin_attrs);
    }

    p.create_join_response_a = std::make_unique<OrbisNpMatching2CreateJoinRoomResponseA>();
    auto& out = *p.create_join_response_a;
    out.room_data = p.room_data.get();
    out.members.members = p.member_data_a.empty() ? nullptr : p.member_data_a.data();
    out.members.members_num = p.member_data_a.size();
    out.members.me = nullptr;
    out.members.owner = nullptr;
    for (auto& md : p.member_data_a) {
        if (md.member_id == static_cast<OrbisNpMatching2RoomMemberId>(resp.me_member_id())) {
            out.members.me = &md;
        }
        if (md.member_id == static_cast<OrbisNpMatching2RoomMemberId>(resp.owner_member_id())) {
            out.members.owner = &md;
        }
    }

    p.request_data = p.create_join_response_a.get();
    return p.request_data;
}

void* BuildLeaveRoomPayload(ContextObject& ctx, CallbackPayload& p,
                            const shadnet::LeaveRoomReply& resp) {
    p.Reset();

    p.leave_room_response = std::make_unique<OrbisNpMatching2LeaveRoomResponse>();
    p.leave_room_response->room_id = static_cast<OrbisNpMatching2RoomId>(resp.room_id());

    if (ctx.room_id == p.leave_room_response->room_id) {
        auto cache = LockCache(ctx.ctx_id);
        cache->rooms.erase(ctx.room_id);
        cache->peers.clear();
        ctx.room_id = 0;
        ctx.my_member_id = 0;
        ctx.is_room_owner = false;
    }

    p.request_data = p.leave_room_response.get();
    return p.request_data;
}

void* BuildGetWorldInfoListPayload(ContextObject& ctx, CallbackPayload& p,
                                   const shadnet::GetWorldInfoListReply& resp) {
    p.Reset();

    p.world_list.resize(resp.worlds_size());
    for (s32 i = 0; i < resp.worlds_size(); ++i) {
        const auto& w = resp.worlds(i);
        auto& dst = p.world_list[i];
        dst = OrbisNpMatching2World{};
        dst.world_id = static_cast<OrbisNpMatching2WorldId>(w.world_id());
        dst.lobbies_num = w.lobbies_num();
        dst.max_lobby_members_num = w.max_lobby_members();
        dst.lobby_members_num = w.lobby_members_num();
        dst.rooms_num = w.rooms_num();
        dst.room_members_num = w.room_members_num();
    }
    for (u64 i = 0; i + 1 < p.world_list.size(); ++i) {
        p.world_list[i].next = &p.world_list[i + 1];
    }

    p.world_info_response = std::make_unique<OrbisNpMatching2GetWorldInfoListResponse>();
    p.world_info_response->world = p.world_list.empty() ? nullptr : p.world_list.data();
    p.world_info_response->world_num = p.world_list.size();

    p.request_data = p.world_info_response.get();
    return p.request_data;
}

void* BuildSearchRoomPayload(ContextObject& ctx, CallbackPayload& p,
                             const shadnet::SearchRoomReply& resp) {
    p.Reset();
    ReserveExternalRoomPayloadStorage(p, resp);

    const s32 room_count = resp.rooms_size();
    LOG_INFO(Lib_NpMatching2, "Search response rooms={}", room_count);
    p.room_data_external.resize(room_count);
    for (s32 i = 0; i < room_count; ++i) {
        const auto& r = resp.rooms(i);
        LOG_DEBUG(Lib_NpMatching2,
                  "  room[{}] id={} max={} cur={} public/open={}/{} private/open={}/{} "
                  "flags={:#x} groups={} search_int={} search_bin={} ext_bin={}",
                  i, r.room_id(), r.max_slot(), r.cur_members(), r.public_slots(),
                  r.open_public_slots(), r.private_slots(), r.open_private_slots(), r.flags(),
                  r.groups_size(), r.external_search_int_attrs_size(),
                  r.external_search_bin_attrs_size(), r.external_bin_attrs_size());
        for (s32 a = 0; a < r.external_search_int_attrs_size(); ++a) {
            const auto& src = r.external_search_int_attrs(a);
            LOG_DEBUG(Lib_NpMatching2, "    search_int[{}] id={:#x} value={}", a, src.attr_id(),
                      src.attr_value());
        }
        for (s32 a = 0; a < r.external_search_bin_attrs_size(); ++a) {
            const auto& src = r.external_search_bin_attrs(a);
            LOG_DEBUG(Lib_NpMatching2, "    search_bin[{}] id={:#x} size={} data={}", a,
                      src.attr_id(), src.data().size(), HexPreview(src.data(), 128));
        }
        for (s32 a = 0; a < r.external_bin_attrs_size(); ++a) {
            const auto& src = r.external_bin_attrs(a);
            LOG_DEBUG(Lib_NpMatching2, "    ext_bin[{}] id={:#x} size={} data={}", a, src.attr_id(),
                      src.data().size(), HexPreview(src.data(), 128));
        }
        auto& dst = p.room_data_external[i];
        dst = OrbisNpMatching2RoomDataExternal{};
        dst.next = (i + 1 < room_count) ? &p.room_data_external[i + 1] : nullptr;
        dst.max_slot = static_cast<u16>(r.max_slot());
        dst.cur_members = static_cast<u16>(r.cur_members());
        dst.flags = r.flags();
        dst.server_id = static_cast<OrbisNpMatching2ServerId>(r.server_id());
        dst.world_id = static_cast<OrbisNpMatching2WorldId>(r.world_id());
        dst.lobby_id = r.lobby_id();
        dst.room_id = r.room_id();
        dst.passwd_slot_mask = r.passwd_slot_mask();
        dst.joined_slot_mask = r.joined_slot_mask();
        dst.public_slots = static_cast<u16>(r.public_slots());
        dst.private_slots = static_cast<u16>(r.private_slots());
        dst.open_public_slots = static_cast<u16>(r.open_public_slots());
        dst.open_private_slots = static_cast<u16>(r.open_private_slots());

        if (!r.owner_npid().empty()) {
            auto& npid = p.ext_owner_npids.emplace_back();
            SetNpId(npid, r.owner_npid());
            dst.owner_np_id = &npid;
        }

        if (r.groups_size() > 0) {
            OrbisNpMatching2RoomGroupInfo* first = nullptr;
            for (s32 g = 0; g < r.groups_size(); ++g) {
                const auto& src = r.groups(g);
                auto& gi = p.ext_room_groups.emplace_back();
                gi = OrbisNpMatching2RoomGroupInfo{};
                gi.id = static_cast<OrbisNpMatching2RoomGroupId>(src.group_id());
                gi.has_passwd = src.has_passwd();
                gi.slots = src.slot_count();
                gi.group_members = src.num_members();
                if (!first) {
                    first = &gi;
                }
            }
            dst.room_group = first;
            dst.room_groups = static_cast<u64>(r.groups_size());
        }

        if (r.external_search_int_attrs_size() > 0) {
            OrbisNpMatching2IntAttr* first = nullptr;
            for (s32 a = 0; a < r.external_search_int_attrs_size(); ++a) {
                const auto& src = r.external_search_int_attrs(a);
                auto& ia = p.ext_int_attrs.emplace_back();
                ia = OrbisNpMatching2IntAttr{};
                ia.id = static_cast<OrbisNpMatching2AttributeId>(src.attr_id());
                ia.num = src.attr_value();
                if (!first) {
                    first = &ia;
                }
            }
            dst.external_search_int_attr = first;
            dst.external_search_int_attrs = static_cast<u64>(r.external_search_int_attrs_size());
        }

        auto build_bin = [&](const auto& src_attrs, OrbisNpMatching2BinAttr*& out_ptr,
                             u64& out_num) {
            if (src_attrs.empty()) {
                return;
            }
            OrbisNpMatching2BinAttr* first = nullptr;
            for (const auto& src : src_attrs) {
                p.bin_buffers.emplace_back(src.data().begin(), src.data().end());
                auto& buf = p.bin_buffers.back();
                auto& ba = p.ext_bin_attrs.emplace_back();
                ba = OrbisNpMatching2BinAttr{};
                ba.id = static_cast<OrbisNpMatching2AttributeId>(src.attr_id());
                ba.data = buf.empty() ? nullptr : buf.data();
                ba.data_size = buf.size();
                if (!first) {
                    first = &ba;
                }
            }
            out_ptr = first;
            out_num = static_cast<u64>(src_attrs.size());
        };
        build_bin(r.external_search_bin_attrs(), dst.external_search_bin_attr,
                  dst.external_search_bin_attrs);
        build_bin(r.external_bin_attrs(), dst.external_bin_attr, dst.external_bin_attrs);
    }

    p.search_room_response = std::make_unique<OrbisNpMatching2SearchRoomResponse>();
    auto& out = *p.search_room_response;
    out = OrbisNpMatching2SearchRoomResponse{};
    out.range.start = resp.range_start();
    out.range.total = resp.range_total();
    out.range.results = resp.range_result();
    out.room_data_ext = p.room_data_external.empty() ? nullptr : p.room_data_external.data();

    p.request_data = p.search_room_response.get();
    return p.request_data;
}

void* BuildSearchRoomPayloadA(ContextObject& ctx, CallbackPayload& p,
                              const shadnet::SearchRoomReply& resp) {
    p.Reset();
    ReserveExternalRoomPayloadStorage(p, resp);

    const s32 room_count = resp.rooms_size();
    LOG_INFO(Lib_NpMatching2, "rooms={} range start={} total={} results={}", room_count,
             resp.range_start(), resp.range_total(), resp.range_result());
    p.room_data_external_a.resize(room_count);
    for (s32 i = 0; i < room_count; ++i) {
        const auto& r = resp.rooms(i);
        LOG_DEBUG(Lib_NpMatching2,
                  "  room_a[{}] id={} max={} cur={} public/open={}/{} private/open={}/{} "
                  "flags={:#x} groups={} search_int={} search_bin={} ext_bin={} owner='{}' "
                  "account={} platform={}",
                  i, r.room_id(), r.max_slot(), r.cur_members(), r.public_slots(),
                  r.open_public_slots(), r.private_slots(), r.open_private_slots(), r.flags(),
                  r.groups_size(), r.external_search_int_attrs_size(),
                  r.external_search_bin_attrs_size(), r.external_bin_attrs_size(), r.owner_npid(),
                  r.owner_account_id(), r.owner_platform());
        for (s32 a = 0; a < r.external_search_int_attrs_size(); ++a) {
            const auto& src = r.external_search_int_attrs(a);
            LOG_DEBUG(Lib_NpMatching2, "    search_int_a[{}] id={:#x} value={}", a, src.attr_id(),
                      src.attr_value());
        }
        for (s32 a = 0; a < r.external_search_bin_attrs_size(); ++a) {
            const auto& src = r.external_search_bin_attrs(a);
            LOG_DEBUG(Lib_NpMatching2, "    search_bin_a[{}] id={:#x} size={} data={}", a,
                      src.attr_id(), src.data().size(), HexPreview(src.data(), 128));
        }
        for (s32 a = 0; a < r.external_bin_attrs_size(); ++a) {
            const auto& src = r.external_bin_attrs(a);
            LOG_DEBUG(Lib_NpMatching2, "    ext_bin_a[{}] id={:#x} size={} data={}", a,
                      src.attr_id(), src.data().size(), HexPreview(src.data(), 128));
        }
        auto& dst = p.room_data_external_a[i];
        dst = OrbisNpMatching2RoomDataExternalA{};
        dst.next = (i + 1 < room_count) ? &p.room_data_external_a[i + 1] : nullptr;
        dst.max_slot = static_cast<u16>(r.max_slot());
        dst.cur_members = static_cast<u16>(r.cur_members());
        dst.flags = r.flags();
        dst.server_id = static_cast<OrbisNpMatching2ServerId>(r.server_id());
        dst.world_id = static_cast<OrbisNpMatching2WorldId>(r.world_id());
        dst.lobby_id = r.lobby_id();
        dst.room_id = r.room_id();
        dst.passwd_slot_mask = r.passwd_slot_mask();
        dst.joined_slot_mask = r.joined_slot_mask();
        dst.public_slots = static_cast<u16>(r.public_slots());
        dst.private_slots = static_cast<u16>(r.private_slots());
        dst.open_public_slots = static_cast<u16>(r.open_public_slots());
        dst.open_private_slots = static_cast<u16>(r.open_private_slots());

        if (!r.owner_npid().empty()) {
            SetNpOnlineId(dst.owner_online_id, r.owner_npid());
        }
        dst.owner.account_id = static_cast<OrbisNpAccountId>(r.owner_account_id());
        dst.owner.platform = static_cast<OrbisNpPlatformType>(r.owner_platform());

        if (r.groups_size() > 0) {
            OrbisNpMatching2RoomGroupInfo* first = nullptr;
            for (s32 g = 0; g < r.groups_size(); ++g) {
                const auto& src = r.groups(g);
                auto& gi = p.ext_room_groups.emplace_back();
                gi = OrbisNpMatching2RoomGroupInfo{};
                gi.id = static_cast<OrbisNpMatching2RoomGroupId>(src.group_id());
                gi.has_passwd = src.has_passwd();
                gi.slots = src.slot_count();
                gi.group_members = src.num_members();
                if (!first) {
                    first = &gi;
                }
            }
            dst.room_group = first;
            dst.room_groups = static_cast<u64>(r.groups_size());
        }

        if (r.external_search_int_attrs_size() > 0) {
            OrbisNpMatching2IntAttr* first = nullptr;
            for (s32 a = 0; a < r.external_search_int_attrs_size(); ++a) {
                const auto& src = r.external_search_int_attrs(a);
                auto& ia = p.ext_int_attrs.emplace_back();
                ia = OrbisNpMatching2IntAttr{};
                ia.id = static_cast<OrbisNpMatching2AttributeId>(src.attr_id());
                ia.num = src.attr_value();
                if (!first) {
                    first = &ia;
                }
            }
            dst.external_search_int_attr = first;
            dst.external_search_int_attrs = static_cast<u64>(r.external_search_int_attrs_size());
        }

        auto build_bin = [&](const auto& src_attrs, OrbisNpMatching2BinAttr*& out_ptr,
                             u64& out_num) {
            if (src_attrs.empty()) {
                return;
            }
            OrbisNpMatching2BinAttr* first = nullptr;
            for (const auto& src : src_attrs) {
                p.bin_buffers.emplace_back(src.data().begin(), src.data().end());
                auto& buf = p.bin_buffers.back();
                auto& ba = p.ext_bin_attrs.emplace_back();
                ba = OrbisNpMatching2BinAttr{};
                ba.id = static_cast<OrbisNpMatching2AttributeId>(src.attr_id());
                ba.data = buf.empty() ? nullptr : buf.data();
                ba.data_size = buf.size();
                if (!first) {
                    first = &ba;
                }
            }
            out_ptr = first;
            out_num = static_cast<u64>(src_attrs.size());
        };
        build_bin(r.external_search_bin_attrs(), dst.external_search_bin_attr,
                  dst.external_search_bin_attrs);
        build_bin(r.external_bin_attrs(), dst.external_bin_attr, dst.external_bin_attrs);
    }

    p.search_room_response_a = std::make_unique<OrbisNpMatching2SearchRoomResponseA>();
    auto& out = *p.search_room_response_a;
    out = OrbisNpMatching2SearchRoomResponseA{};
    out.range.start = resp.range_start();
    out.range.total = resp.range_total();
    out.range.results = resp.range_result();
    out.room_data_ext = p.room_data_external_a.empty() ? nullptr : p.room_data_external_a.data();

    p.request_data = p.search_room_response_a.get();
    return p.request_data;
}

void* BuildGetRoomDataExternalListPayload(ContextObject& ctx, CallbackPayload& p,
                                          const shadnet::GetRoomDataExternalListReply& resp) {
    p.Reset();
    ReserveExternalRoomPayloadStorage(p, resp);

    const s32 room_count = resp.rooms_size();
    LOG_INFO(Lib_NpMatching2, "response rooms={}", room_count);
    p.room_data_external.resize(room_count);
    for (s32 i = 0; i < room_count; ++i) {
        const auto& r = resp.rooms(i);
        LOG_DEBUG(Lib_NpMatching2,
                  "  room[{}] id={} max={} cur={} public/open={}/{} private/open={}/{} "
                  "flags={:#x} groups={} search_int={} search_bin={} ext_bin={}",
                  i, r.room_id(), r.max_slot(), r.cur_members(), r.public_slots(),
                  r.open_public_slots(), r.private_slots(), r.open_private_slots(), r.flags(),
                  r.groups_size(), r.external_search_int_attrs_size(),
                  r.external_search_bin_attrs_size(), r.external_bin_attrs_size());
        for (s32 a = 0; a < r.external_search_int_attrs_size(); ++a) {
            const auto& src = r.external_search_int_attrs(a);
            LOG_DEBUG(Lib_NpMatching2, "    search_int[{}] id={:#x} value={}", a, src.attr_id(),
                      src.attr_value());
        }
        for (s32 a = 0; a < r.external_search_bin_attrs_size(); ++a) {
            const auto& src = r.external_search_bin_attrs(a);
            LOG_DEBUG(Lib_NpMatching2, "    search_bin[{}] id={:#x} size={} data={}", a,
                      src.attr_id(), src.data().size(), HexPreview(src.data(), 128));
        }
        for (s32 a = 0; a < r.external_bin_attrs_size(); ++a) {
            const auto& src = r.external_bin_attrs(a);
            LOG_DEBUG(Lib_NpMatching2, "    ext_bin[{}] id={:#x} size={} data={}", a, src.attr_id(),
                      src.data().size(), HexPreview(src.data(), 128));
        }
        auto& dst = p.room_data_external[i];
        dst = OrbisNpMatching2RoomDataExternal{};
        dst.next = (i + 1 < room_count) ? &p.room_data_external[i + 1] : nullptr;
        dst.max_slot = static_cast<u16>(r.max_slot());
        dst.cur_members = static_cast<u16>(r.cur_members());
        dst.flags = r.flags();
        dst.server_id = static_cast<OrbisNpMatching2ServerId>(r.server_id());
        dst.world_id = static_cast<OrbisNpMatching2WorldId>(r.world_id());
        dst.lobby_id = r.lobby_id();
        dst.room_id = r.room_id();
        dst.passwd_slot_mask = r.passwd_slot_mask();
        dst.joined_slot_mask = r.joined_slot_mask();
        dst.public_slots = static_cast<u16>(r.public_slots());
        dst.private_slots = static_cast<u16>(r.private_slots());
        dst.open_public_slots = static_cast<u16>(r.open_public_slots());
        dst.open_private_slots = static_cast<u16>(r.open_private_slots());

        if (!r.owner_npid().empty()) {
            auto& npid = p.ext_owner_npids.emplace_back();
            SetNpId(npid, r.owner_npid());
            dst.owner_np_id = &npid;
        }

        if (r.groups_size() > 0) {
            OrbisNpMatching2RoomGroupInfo* first = nullptr;
            for (s32 g = 0; g < r.groups_size(); ++g) {
                const auto& src = r.groups(g);
                auto& gi = p.ext_room_groups.emplace_back();
                gi = OrbisNpMatching2RoomGroupInfo{};
                gi.id = static_cast<OrbisNpMatching2RoomGroupId>(src.group_id());
                gi.has_passwd = src.has_passwd();
                gi.slots = src.slot_count();
                gi.group_members = src.num_members();
                if (!first) {
                    first = &gi;
                }
            }
            dst.room_group = first;
            dst.room_groups = static_cast<u64>(r.groups_size());
        }

        if (r.external_search_int_attrs_size() > 0) {
            OrbisNpMatching2IntAttr* first = nullptr;
            for (s32 a = 0; a < r.external_search_int_attrs_size(); ++a) {
                const auto& src = r.external_search_int_attrs(a);
                auto& ia = p.ext_int_attrs.emplace_back();
                ia = OrbisNpMatching2IntAttr{};
                ia.id = static_cast<OrbisNpMatching2AttributeId>(src.attr_id());
                ia.num = src.attr_value();
                if (!first) {
                    first = &ia;
                }
            }
            dst.external_search_int_attr = first;
            dst.external_search_int_attrs = static_cast<u64>(r.external_search_int_attrs_size());
        }

        auto build_bin = [&](const auto& src_attrs, OrbisNpMatching2BinAttr*& out_ptr,
                             u64& out_num) {
            if (src_attrs.empty()) {
                return;
            }
            OrbisNpMatching2BinAttr* first = nullptr;
            for (const auto& src : src_attrs) {
                p.bin_buffers.emplace_back(src.data().begin(), src.data().end());
                auto& buf = p.bin_buffers.back();
                auto& ba = p.ext_bin_attrs.emplace_back();
                ba = OrbisNpMatching2BinAttr{};
                ba.id = static_cast<OrbisNpMatching2AttributeId>(src.attr_id());
                ba.data = buf.empty() ? nullptr : buf.data();
                ba.data_size = buf.size();
                if (!first) {
                    first = &ba;
                }
            }
            out_ptr = first;
            out_num = static_cast<u64>(src_attrs.size());
        };
        build_bin(r.external_search_bin_attrs(), dst.external_search_bin_attr,
                  dst.external_search_bin_attrs);
        build_bin(r.external_bin_attrs(), dst.external_bin_attr, dst.external_bin_attrs);
    }

    p.room_data_external_list_response =
        std::make_unique<OrbisNpMatching2GetRoomDataExternalListResponse>();
    auto& out = *p.room_data_external_list_response;
    out = OrbisNpMatching2GetRoomDataExternalListResponse{};
    out.room_data_external = p.room_data_external.empty() ? nullptr : p.room_data_external.data();
    out.room_data_external_num = static_cast<u64>(p.room_data_external.size());

    p.request_data = p.room_data_external_list_response.get();
    return p.request_data;
}

void* BuildGetRoomDataExternalListPayloadA(ContextObject& ctx, CallbackPayload& p,
                                           const shadnet::GetRoomDataExternalListReply& resp) {
    p.Reset();
    ReserveExternalRoomPayloadStorage(p, resp);

    const s32 room_count = resp.rooms_size();
    p.room_data_external_a.resize(room_count);
    for (s32 i = 0; i < room_count; ++i) {
        const auto& r = resp.rooms(i);
        auto& dst = p.room_data_external_a[i];
        dst = OrbisNpMatching2RoomDataExternalA{};
        dst.next = (i + 1 < room_count) ? &p.room_data_external_a[i + 1] : nullptr;
        dst.max_slot = static_cast<u16>(r.max_slot());
        dst.cur_members = static_cast<u16>(r.cur_members());
        dst.flags = r.flags();
        dst.server_id = static_cast<OrbisNpMatching2ServerId>(r.server_id());
        dst.world_id = static_cast<OrbisNpMatching2WorldId>(r.world_id());
        dst.lobby_id = r.lobby_id();
        dst.room_id = r.room_id();
        dst.passwd_slot_mask = r.passwd_slot_mask();
        dst.joined_slot_mask = r.joined_slot_mask();
        dst.public_slots = static_cast<u16>(r.public_slots());
        dst.private_slots = static_cast<u16>(r.private_slots());
        dst.open_public_slots = static_cast<u16>(r.open_public_slots());
        dst.open_private_slots = static_cast<u16>(r.open_private_slots());

        if (!r.owner_npid().empty()) {
            SetNpOnlineId(dst.owner_online_id, r.owner_npid());
        }
        dst.owner.account_id = static_cast<OrbisNpAccountId>(r.owner_account_id());
        dst.owner.platform = static_cast<OrbisNpPlatformType>(r.owner_platform());

        if (r.groups_size() > 0) {
            OrbisNpMatching2RoomGroupInfo* first = nullptr;
            for (s32 g = 0; g < r.groups_size(); ++g) {
                const auto& src = r.groups(g);
                auto& gi = p.ext_room_groups.emplace_back();
                gi = OrbisNpMatching2RoomGroupInfo{};
                gi.id = static_cast<OrbisNpMatching2RoomGroupId>(src.group_id());
                gi.has_passwd = src.has_passwd();
                gi.slots = src.slot_count();
                gi.group_members = src.num_members();
                if (!first) {
                    first = &gi;
                }
            }
            dst.room_group = first;
            dst.room_groups = static_cast<u64>(r.groups_size());
        }

        if (r.external_search_int_attrs_size() > 0) {
            OrbisNpMatching2IntAttr* first = nullptr;
            for (s32 a = 0; a < r.external_search_int_attrs_size(); ++a) {
                const auto& src = r.external_search_int_attrs(a);
                auto& ia = p.ext_int_attrs.emplace_back();
                ia = OrbisNpMatching2IntAttr{};
                ia.id = static_cast<OrbisNpMatching2AttributeId>(src.attr_id());
                ia.num = src.attr_value();
                if (!first) {
                    first = &ia;
                }
            }
            dst.external_search_int_attr = first;
            dst.external_search_int_attrs = static_cast<u64>(r.external_search_int_attrs_size());
        }

        auto build_bin = [&](const auto& src_attrs, OrbisNpMatching2BinAttr*& out_ptr,
                             u64& out_num) {
            if (src_attrs.empty()) {
                return;
            }
            OrbisNpMatching2BinAttr* first = nullptr;
            for (const auto& src : src_attrs) {
                p.bin_buffers.emplace_back(src.data().begin(), src.data().end());
                auto& buf = p.bin_buffers.back();
                auto& ba = p.ext_bin_attrs.emplace_back();
                ba = OrbisNpMatching2BinAttr{};
                ba.id = static_cast<OrbisNpMatching2AttributeId>(src.attr_id());
                ba.data = buf.empty() ? nullptr : buf.data();
                ba.data_size = buf.size();
                if (!first) {
                    first = &ba;
                }
            }
            out_ptr = first;
            out_num = static_cast<u64>(src_attrs.size());
        };
        build_bin(r.external_search_bin_attrs(), dst.external_search_bin_attr,
                  dst.external_search_bin_attrs);
        build_bin(r.external_bin_attrs(), dst.external_bin_attr, dst.external_bin_attrs);
    }

    p.room_data_external_list_response_a =
        std::make_unique<OrbisNpMatching2GetRoomDataExternalListResponseA>();
    auto& out = *p.room_data_external_list_response_a;
    out = OrbisNpMatching2GetRoomDataExternalListResponseA{};
    out.room_data_external =
        p.room_data_external_a.empty() ? nullptr : p.room_data_external_a.data();
    out.room_data_external_num = static_cast<u64>(p.room_data_external_a.size());

    p.request_data = p.room_data_external_list_response_a.get();
    return p.request_data;
}

void* BuildGetRoomMemberDataExternalListPayload(
    ContextObject& ctx, CallbackPayload& p,
    const shadnet::GetRoomMemberDataExternalListReply& resp) {
    p.Reset();

    const s32 member_count = resp.members_size();
    p.member_data_external.resize(member_count);
    for (s32 i = 0; i < member_count; ++i) {
        const auto& src = resp.members(i);
        auto& dst = p.member_data_external[i];
        dst = OrbisNpMatching2RoomMemberDataExternal{};
        dst.next = (i + 1 < member_count) ? &p.member_data_external[i + 1] : nullptr;
        SetNpId(dst.np_id, src.npid());
        dst.join_date.tick = src.join_date();
        dst.role = static_cast<OrbisNpMatching2Role>(src.role());
    }

    p.room_member_data_external_list_response =
        std::make_unique<OrbisNpMatching2GetRoomMemberDataExternalListResponse>();
    auto& out = *p.room_member_data_external_list_response;
    out = OrbisNpMatching2GetRoomMemberDataExternalListResponse{};
    out.room_member_data_external =
        p.member_data_external.empty() ? nullptr : p.member_data_external.data();
    out.room_member_data_external_num = static_cast<u64>(p.member_data_external.size());

    p.request_data = p.room_member_data_external_list_response.get();
    return p.request_data;
}

void* BuildGetRoomMemberDataExternalListPayloadA(
    ContextObject& ctx, CallbackPayload& p,
    const shadnet::GetRoomMemberDataExternalListReply& resp) {
    p.Reset();

    const s32 member_count = resp.members_size();
    p.member_data_external_a.resize(member_count);
    for (s32 i = 0; i < member_count; ++i) {
        const auto& src = resp.members(i);
        auto& dst = p.member_data_external_a[i];
        dst = OrbisNpMatching2RoomMemberDataExternalA{};
        dst.next = (i + 1 < member_count) ? &p.member_data_external_a[i + 1] : nullptr;
        dst.user.account_id = static_cast<OrbisNpAccountId>(src.account_id());
        dst.user.platform = static_cast<OrbisNpPlatformType>(src.platform());
        SetNpOnlineId(dst.online_id, src.npid());
        dst.join_date.tick = src.join_date();
        dst.role = static_cast<OrbisNpMatching2Role>(src.role());
    }

    p.room_member_data_external_list_response_a =
        std::make_unique<OrbisNpMatching2GetRoomMemberDataExternalListResponseA>();
    auto& out = *p.room_member_data_external_list_response_a;
    out = OrbisNpMatching2GetRoomMemberDataExternalListResponseA{};
    out.room_member_data_external =
        p.member_data_external_a.empty() ? nullptr : p.member_data_external_a.data();
    out.room_member_data_external_num = static_cast<u64>(p.member_data_external_a.size());

    p.request_data = p.room_member_data_external_list_response_a.get();
    return p.request_data;
}

void* BuildGetUserInfoListPayload(ContextObject& ctx, CallbackPayload& p,
                                  const shadnet::GetUserInfoListReply& resp) {
    p.Reset();

    u64 total_bin_attrs = 0;
    for (s32 i = 0; i < resp.users_size(); ++i) {
        total_bin_attrs += static_cast<u64>(resp.users(i).user_bin_attrs_size());
    }
    p.user_bin_attrs.reserve(total_bin_attrs);
    p.bin_buffers.reserve(total_bin_attrs);

    const s32 user_count = resp.users_size();
    p.user_info.resize(user_count);
    for (s32 i = 0; i < user_count; ++i) {
        const auto& u = resp.users(i);
        auto& dst = p.user_info[i];
        dst = OrbisNpMatching2UserInfo{};
        dst.next = (i + 1 < user_count) ? &p.user_info[i + 1] : nullptr;
        SetNpId(dst.np_id, u.npid());

        if (u.user_bin_attrs_size() > 0) {
            OrbisNpMatching2BinAttr* first = nullptr;
            for (s32 a = 0; a < u.user_bin_attrs_size(); ++a) {
                const auto& src = u.user_bin_attrs(a);
                p.bin_buffers.emplace_back(src.data().begin(), src.data().end());
                auto& buf = p.bin_buffers.back();
                auto& ba = p.user_bin_attrs.emplace_back();
                ba = OrbisNpMatching2BinAttr{};
                ba.id = static_cast<OrbisNpMatching2AttributeId>(src.attr_id());
                ba.data = buf.empty() ? nullptr : buf.data();
                ba.data_size = buf.size();
                if (!first) {
                    first = &ba;
                }
            }
            dst.user_bin_attr = first;
            dst.user_bin_attr_num = static_cast<u64>(u.user_bin_attrs_size());
        }
    }

    p.user_info_list_response = std::make_unique<OrbisNpMatching2GetUserInfoListResponse>();
    auto& out = *p.user_info_list_response;
    out = OrbisNpMatching2GetUserInfoListResponse{};
    out.user_info = p.user_info.empty() ? nullptr : p.user_info.data();
    out.user_info_num = static_cast<u64>(p.user_info.size());

    p.request_data = p.user_info_list_response.get();
    return p.request_data;
}

void* BuildGetUserInfoListPayloadA(ContextObject& ctx, CallbackPayload& p,
                                   const shadnet::GetUserInfoListReply& resp) {
    p.Reset();

    u64 total_bin_attrs = 0;
    for (s32 i = 0; i < resp.users_size(); ++i) {
        total_bin_attrs += static_cast<u64>(resp.users(i).user_bin_attrs_size());
    }
    p.user_bin_attrs.reserve(total_bin_attrs);
    p.bin_buffers.reserve(total_bin_attrs);

    const s32 user_count = resp.users_size();
    p.user_info_a.resize(user_count);
    for (s32 i = 0; i < user_count; ++i) {
        const auto& u = resp.users(i);
        auto& dst = p.user_info_a[i];
        dst = OrbisNpMatching2UserInfoA{};
        dst.next = (i + 1 < user_count) ? &p.user_info_a[i + 1] : nullptr;
        SetNpOnlineId(dst.user_online_id, u.npid());
        dst.user.account_id = static_cast<OrbisNpAccountId>(u.account_id());
        dst.user.platform = static_cast<OrbisNpPlatformType>(u.platform());

        if (u.user_bin_attrs_size() > 0) {
            OrbisNpMatching2BinAttr* first = nullptr;
            for (s32 a = 0; a < u.user_bin_attrs_size(); ++a) {
                const auto& src = u.user_bin_attrs(a);
                p.bin_buffers.emplace_back(src.data().begin(), src.data().end());
                auto& buf = p.bin_buffers.back();
                auto& ba = p.user_bin_attrs.emplace_back();
                ba = OrbisNpMatching2BinAttr{};
                ba.id = static_cast<OrbisNpMatching2AttributeId>(src.attr_id());
                ba.data = buf.empty() ? nullptr : buf.data();
                ba.data_size = buf.size();
                if (!first) {
                    first = &ba;
                }
            }
            dst.user_bin_attr = first;
            dst.user_bin_attr_num = static_cast<u64>(u.user_bin_attrs_size());
        }
    }

    p.user_info_list_response_a = std::make_unique<OrbisNpMatching2GetUserInfoListResponseA>();
    auto& out = *p.user_info_list_response_a;
    out = OrbisNpMatching2GetUserInfoListResponseA{};
    out.user_info = p.user_info_a.empty() ? nullptr : p.user_info_a.data();
    out.user_info_num = static_cast<u64>(p.user_info_a.size());

    p.request_data = p.user_info_list_response_a.get();
    return p.request_data;
}

void* BuildGetRoomDataInternalPayload(ContextObject& ctx, CallbackPayload& p,
                                      OrbisNpMatching2RoomId room_id) {
    auto cache = LockCache(ctx.ctx_id);
    const auto rc_it = cache->rooms.find(room_id);
    if (rc_it == cache->rooms.end()) {
        return nullptr;
    }
    const RoomCache& rc = rc_it->second;

    p.Reset();

    p.room_groups.clear();
    p.room_groups.reserve(rc.groups.size());
    for (const auto& [gid, g] : rc.groups) {
        p.room_groups.push_back(g);
    }

    p.room_bin_attrs.resize(rc.bin_attrs_internal.size());
    p.bin_buffers.clear();
    p.bin_buffers.reserve(rc.bin_attrs_internal.size());
    for (u64 i = 0; i < rc.bin_attrs_internal.size(); ++i) {
        const auto& src = rc.bin_attrs_internal[i];
        const auto* data = static_cast<const u8*>(src.bin_attr.data);
        if (data && src.bin_attr.data_size > 0) {
            p.bin_buffers.emplace_back(data, data + src.bin_attr.data_size);
        } else {
            p.bin_buffers.emplace_back();
        }
        auto& buf = p.bin_buffers.back();
        auto& dst = p.room_bin_attrs[i];
        dst = OrbisNpMatching2RoomBinAttrInternal{};
        dst.last_update = src.last_update;
        dst.bin_attr.id = src.bin_attr.id;
        dst.bin_attr.data = buf.empty() ? nullptr : buf.data();
        dst.bin_attr.data_size = buf.size();
    }

    p.room_data = std::make_unique<OrbisNpMatching2RoomDataInternal>();
    auto& room = *p.room_data;
    room = {};
    room.public_slots = rc.public_slots;
    room.private_slots = rc.private_slots;
    room.open_public_slots = rc.open_public_slots;
    room.open_private_slots = rc.open_private_slots;
    room.max_slot = rc.max_slot;
    room.server_id = rc.server_id;
    room.world_id = rc.world_id;
    room.lobby_id = rc.lobby_id;
    room.room_id = rc.room_id;
    room.passwd_slot_mask = rc.passwd_slot_mask;
    room.joined_slot_mask = rc.joined_slot_mask;
    room.flags = rc.flags;
    room.room_group = p.room_groups.empty() ? nullptr : p.room_groups.data();
    room.room_groups = p.room_groups.size();
    room.internal_bin_attr = p.room_bin_attrs.empty() ? nullptr : p.room_bin_attrs.data();
    room.internal_bin_attrs = p.room_bin_attrs.size();

    p.member_data.clear();
    p.member_data.reserve(rc.members.size());
    for (const auto& [member_id, mc] : rc.members) {
        auto& dst = p.member_data.emplace_back();
        dst = OrbisNpMatching2RoomMemberDataInternal{};
        dst.join_date = mc.join_date;
        dst.np_id = mc.np_id;
        dst.member_id = mc.member_id;
        dst.team_id = mc.team_id;
        dst.nat_type = mc.nat_type;
        dst.flag_attr = mc.flag_attr;
    }
    for (u64 i = 0; i < p.member_data.size(); ++i) {
        p.member_data[i].next = (i + 1 < p.member_data.size()) ? &p.member_data[i + 1] : nullptr;
    }

    p.create_join_response = std::make_unique<OrbisNpMatching2CreateJoinRoomResponse>();
    auto& out = *p.create_join_response;
    out = OrbisNpMatching2CreateJoinRoomResponse{};
    out.room_data = p.room_data.get();
    out.members.members = p.member_data.empty() ? nullptr : p.member_data.data();
    out.members.members_num = p.member_data.size();
    for (auto& md : p.member_data) {
        if (md.member_id == ctx.my_member_id) {
            out.members.me = &md;
        }
        if (md.flag_attr & ORBIS_NP_MATCHING2_ROOMMEMBER_FLAG_ATTR_OWNER) {
            out.members.owner = &md;
        }
    }

    p.request_data = p.create_join_response.get();
    return p.request_data;
}

void* BuildGetRoomMemberDataInternalPayload(
    ContextObject& ctx, CallbackPayload& p,
    const OrbisNpMatching2GetRoomMemberDataInternalRequest& request) {
    auto cache = LockCache(ctx.ctx_id);
    const auto room_it = cache->rooms.find(request.room_id);
    if (room_it == cache->rooms.end()) {
        return nullptr;
    }
    const RoomCache& room = room_it->second;
    const auto member_it = room.members.find(request.member_id);
    if (member_it == room.members.end()) {
        return nullptr;
    }
    const MemberCache& member = member_it->second;

    p.Reset();
    if (member.group_id != 0) {
        const auto group_it = room.groups.find(member.group_id);
        if (group_it != room.groups.end()) {
            p.room_groups.push_back(group_it->second);
        }
    }

    std::vector<OrbisNpMatching2AttributeId> ids;
    if (request.attr_id_num == 0) {
        for (const auto& entry : member.bins) {
            ids.push_back(entry.first);
        }
    } else {
        ids.assign(request.attr_id, request.attr_id + request.attr_id_num);
    }
    p.member_bin_attrs.reserve(ids.size());
    p.bin_buffers.reserve(ids.size());
    for (const OrbisNpMatching2AttributeId id : ids) {
        const auto bin_it = member.bins.find(id);
        auto& dst = p.member_bin_attrs.emplace_back();
        dst = {};
        dst.bin_attr.id = id;
        if (bin_it == member.bins.end()) {
            p.bin_buffers.emplace_back(64, u8{0});
            dst.bin_attr.data = p.bin_buffers.back().data();
            dst.bin_attr.data_size = 0;
            continue;
        }
        p.bin_buffers.push_back(bin_it->second.data);
        auto& data = p.bin_buffers.back();
        dst.last_update.tick = bin_it->second.update_date;
        dst.bin_attr.data = data.empty() ? nullptr : data.data();
        dst.bin_attr.data_size = data.size();
    }

    if (ctx.a_variant) {
        p.member_data_a.resize(1);
        auto& out_member = p.member_data_a.front();
        out_member = {};
        out_member.join_date_ticks.tick = member.join_date;
        out_member.user.account_id = member.account_id;
        out_member.user.platform = member.platform;
        out_member.online_id = member.np_id.handle;
        out_member.member_id = member.member_id;
        out_member.team_id = member.team_id;
        out_member.nat_type = member.nat_type;
        out_member.flags = member.flag_attr;
        out_member.room_group = p.room_groups.empty() ? nullptr : p.room_groups.data();
        out_member.room_member_internal_bin_attr =
            p.member_bin_attrs.empty() ? nullptr : p.member_bin_attrs.data();
        out_member.room_member_internal_bin_attrs = p.member_bin_attrs.size();

        p.room_member_data_internal_response_a =
            std::make_unique<OrbisNpMatching2GetRoomMemberDataInternalResponseA>();
        p.room_member_data_internal_response_a->room_member_data_internal = &out_member;
        p.request_data = p.room_member_data_internal_response_a.get();
        return p.request_data;
    }

    p.member_data.resize(1);
    auto& out_member = p.member_data.front();
    out_member = {};
    out_member.join_date = member.join_date;
    out_member.np_id = member.np_id;
    out_member.member_id = member.member_id;
    out_member.team_id = member.team_id;
    out_member.nat_type = member.nat_type;
    out_member.flag_attr = member.flag_attr;
    out_member.room_group = p.room_groups.empty() ? nullptr : p.room_groups.data();
    out_member.room_member_bin_attr_internal =
        p.member_bin_attrs.empty() ? nullptr : p.member_bin_attrs.data();
    out_member.room_member_bin_attr_internal_num = p.member_bin_attrs.size();

    p.room_member_data_internal_response =
        std::make_unique<OrbisNpMatching2GetRoomMemberDataInternalResponse>();
    p.room_member_data_internal_response->room_member_data_internal = &out_member;
    p.request_data = p.room_member_data_internal_response.get();
    return p.request_data;
}

void* BuildRoomMessagePayload(CallbackPayload& p, bool a_variant,
                              OrbisNpMatching2CastType cast_type,
                              const std::vector<OrbisNpMatching2RoomMemberId>& dst_members,
                              const MemberCache* src_member, const std::vector<u8>& msg) {
    p.Reset();

    p.room_message_dst = std::make_unique<OrbisNpMatching2RoomMessageDestination>();
    *p.room_message_dst = {};
    if (cast_type == ORBIS_NP_MATCHING2_CASTTYPE_UNICAST && !dst_members.empty()) {
        p.room_message_dst->unicast_target = dst_members.front();
    } else if (cast_type == ORBIS_NP_MATCHING2_CASTTYPE_MULTICAST && !dst_members.empty()) {
        p.room_message_multicast_members = dst_members;
        p.room_message_dst->multicast_target.member_id = p.room_message_multicast_members.data();
        p.room_message_dst->multicast_target.member_id_num =
            p.room_message_multicast_members.size();
    }

    p.room_message_data = msg;
    void* msg_ptr = p.room_message_data.empty() ? nullptr : p.room_message_data.data();

    LOG_INFO(Lib_NpMatching2,
             "variant={} cast={} dst_n={} src_member={} src_mid={} "
             "src_account={} src_platform={} msg={} msg_len={} preview=[{}]",
             a_variant ? "A" : "regular", cast_type, dst_members.size(), fmt::ptr(src_member),
             src_member ? src_member->member_id : 0, src_member ? src_member->account_id : 0,
             src_member ? static_cast<s32>(src_member->platform) : 0, fmt::ptr(msg_ptr), msg.size(),
             HexPreview(msg.data(), msg.size(), 32));

    if (a_variant) {
        p.room_message_info_a = std::make_unique<OrbisNpMatching2RoomMessageInfoA>();
        auto& info = *p.room_message_info_a;
        info = {};
        info.filtered = false;
        info.cast_type = cast_type;
        if (src_member) {
            info.src_member.account_id = src_member->account_id;
            info.src_member.platform = src_member->platform;
            info.src_member_online_id = src_member->np_id.handle;
        }
        info.dst = p.room_message_dst.get();
        info.msg = msg_ptr;
        info.msg_len = p.room_message_data.size();
        p.room_message_callback_data = p.room_message_info_a.get();
        LOG_INFO(Lib_NpMatching2,
                 "info={} dst={} src_member={} msg={} "
                 "msg_len={} account={} platform={} online_id='{}'",
                 fmt::ptr(&info), fmt::ptr(info.dst), fmt::ptr(&info.src_member),
                 fmt::ptr(info.msg), info.msg_len, info.src_member.account_id,
                 static_cast<s32>(info.src_member.platform),
                 src_member ? std::string_view(src_member->np_id.handle.data,
                                               strnlen(src_member->np_id.handle.data,
                                                       ORBIS_NP_ONLINEID_MAX_LENGTH))
                            : std::string_view{});
        return p.room_message_callback_data;
    }

    p.room_message_src_npid = std::make_unique<OrbisNpId>();
    *p.room_message_src_npid = {};
    if (src_member) {
        *p.room_message_src_npid = src_member->np_id;
    }

    p.room_message_info = std::make_unique<OrbisNpMatching2RoomMessageInfo>();
    auto& info = *p.room_message_info;
    info = {};
    info.filtered = false;
    info.cast_type = cast_type;
    info.dst = p.room_message_dst.get();
    info.src_member = p.room_message_src_npid.get();
    info.msg = msg_ptr;
    info.msg_len = static_cast<u32>(p.room_message_data.size());
    p.room_message_callback_data = p.room_message_info.get();
    LOG_DEBUG(Lib_NpMatching2, "info={} dst={} src_np_id={} msg={} msg_len={} online_id='{}'",
              fmt::ptr(&info), fmt::ptr(info.dst), fmt::ptr(info.src_member), fmt::ptr(info.msg),
              info.msg_len,
              src_member ? std::string_view(
                               src_member->np_id.handle.data,
                               strnlen(src_member->np_id.handle.data, ORBIS_NP_ONLINEID_MAX_LENGTH))
                         : std::string_view{});
    return p.room_message_callback_data;
}

std::shared_ptr<ContextObject> ContextManager::GetLocked(OrbisNpMatching2ContextId ctx_id) {
    if (ctx_id == 0 || ctx_id > kMaxContexts) {
        return {};
    }
    return m_contexts[ctx_id];
}

s32 ContextManager::CreateContext(const OrbisNpId* owner_np_id, OrbisNpServiceLabel service_label,
                                  OrbisNpMatching2ContextId* out_ctx_id, bool a_variant) {
    if (!out_ctx_id) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    std::lock_guard lock(m_mutex);

    OrbisNpMatching2ContextId id = 0;
    for (u32 i = 0; i < kMaxContexts; ++i) {
        const OrbisNpMatching2ContextId candidate =
            static_cast<OrbisNpMatching2ContextId>(((m_next_id - 1 + i) % kMaxContexts) + 1);
        if (!m_contexts[candidate]) {
            id = candidate;
            break;
        }
    }
    if (id == 0) {
        return ORBIS_NP_MATCHING2_ERROR_CONTEXT_MAX;
    }
    m_next_id = static_cast<OrbisNpMatching2ContextId>((id % kMaxContexts) + 1);

    auto ctx = std::make_shared<ContextObject>();
    ResetCache(id);
    ctx->ctx_id = id;
    ctx->a_variant = a_variant;
    ctx->service_label = service_label;
    LockCache(id)->Reset();
    if (owner_np_id) {
        ctx->owner_np_id = *owner_np_id;
        ctx->online_id = owner_np_id->handle;
    }
    ctx->context_callback = m_pending_context_callback;
    ctx->context_callback_arg = m_pending_context_callback_arg;
    m_contexts[id] = ctx;

    *out_ctx_id = id;
    LOG_INFO(Lib_NpMatching2, "id={} online_id={} service_label={:#x}", id, ctx->online_id.data,
             service_label);
    return ORBIS_OK;
}

std::shared_ptr<ContextObject> ContextManager::Get(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex);
    return GetLocked(ctx_id);
}

bool ContextManager::Destroy(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex);
    auto ctx = GetLocked(ctx_id);
    if (!ctx) {
        return false;
    }
    LOG_INFO(Lib_NpMatching2, "id={}", ctx_id);
    if (ctx->stop_pending) {
        ctx->destroy_pending = true;
        return true;
    }
    ResetCache(ctx_id);
    m_contexts[ctx_id].reset();
    return true;
}

void ContextManager::CompleteStop(OrbisNpMatching2ContextId ctx_id,
                                  const std::shared_ptr<ContextObject>& expected_context) {
    std::lock_guard lock(m_mutex);
    auto ctx = GetLocked(ctx_id);
    if (!ctx || ctx != expected_context) {
        return;
    }
    ctx->stop_pending = false;
    if (!ctx->destroy_pending) {
        return;
    }
    ResetCache(ctx_id);
    m_contexts[ctx_id].reset();
}

s32 ContextManager::Start(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex);
    auto ctx = GetLocked(ctx_id);
    if (!ctx) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }
    if (ctx->started) {
        return ORBIS_NP_MATCHING2_ERROR_CONTEXT_ALREADY_STARTED;
    }
    ctx->started = true;
    ctx->stop_pending = false;
    ctx->destroy_pending = false;
    LOG_INFO(Lib_NpMatching2, "id={}", ctx_id);
    return ORBIS_OK;
}

s32 ContextManager::Stop(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex);
    auto ctx = GetLocked(ctx_id);
    if (!ctx) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }
    if (!ctx->started) {
        return ORBIS_NP_MATCHING2_ERROR_CONTEXT_NOT_STARTED;
    }
    ctx->started = false;
    ctx->stop_pending = true;
    LOG_INFO(Lib_NpMatching2, "id={}", ctx_id);
    return ORBIS_OK;
}

void ContextManager::ApplyContextCallback(OrbisNpMatching2ContextCallback callback, void* arg) {
    std::lock_guard lock(m_mutex);
    m_pending_context_callback = callback;
    m_pending_context_callback_arg = arg;
    for (u32 id = 1; id <= kMaxContexts; ++id) {
        if (m_contexts[id]) {
            m_contexts[id]->context_callback = callback;
            m_contexts[id]->context_callback_arg = arg;
        }
    }
}

void ContextManager::Reset() {
    std::lock_guard lock(m_mutex);
    m_contexts = {};
    ResetCaches();
    m_next_id = 1;
    m_pending_context_callback = nullptr;
    m_pending_context_callback_arg = nullptr;
}

OrbisNpMatching2RequestId AllocRequestId() {
    auto* state = GetState();
    std::lock_guard lock(state->mutex);
    const OrbisNpMatching2RequestId id = state->next_request_id++;
    if (state->next_request_id == 0) {
        state->next_request_id = 1;
    }
    return id;
}

bool IsInitialized() {
    return GetState()->initialized.load();
}

void SetInitialized(bool initialized) {
    GetState()->initialized.store(initialized);
}

void StoreRequestCallback(const std::shared_ptr<ContextObject>& ctx,
                          const OrbisNpMatching2RequestOptParam* request_opt) {
    ctx->per_request_callback = nullptr;
    ctx->per_request_callback_arg = nullptr;
    if (request_opt && request_opt->callback) {
        ctx->per_request_callback = request_opt->callback;
        ctx->per_request_callback_arg = request_opt->arg;
    }
}

RequestCallbackInfo ConsumeRequestCallback(const std::shared_ptr<ContextObject>& ctx) {
    RequestCallbackInfo cb{};
    if (ctx->per_request_callback) {
        cb.callback = ctx->per_request_callback;
        cb.arg = ctx->per_request_callback_arg;
        ctx->per_request_callback = nullptr;
        ctx->per_request_callback_arg = nullptr;
        return cb;
    }

    cb.callback = ctx->default_request_callback;
    cb.arg = ctx->default_request_callback_arg;
    return cb;
}

std::string CallbackOnlineId(const OrbisNpId& npid) {
    return std::string(npid.handle.data, strnlen(npid.handle.data, ORBIS_NP_ONLINEID_MAX_LENGTH));
}

template <typename T>
void LogCallbackObject(std::string_view label, const T* object) {
    if (!object) {
        return;
    }
    LOG_INFO(Lib_NpMatching2, "Callback payload {}: ptr={} size={:#x} raw={}", label,
             fmt::ptr(object), sizeof(T), HexPreview(object, sizeof(T), sizeof(T)));
}

bool IsCreateJoinRequestEvent(OrbisNpMatching2Event event) {
    return event == ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM ||
           event == ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM ||
           event == ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM_A ||
           event == ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM_A;
}

bool IsMemberJoinedEvent(OrbisNpMatching2Event event) {
    return event == ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED ||
           event == ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED_A;
}

void FireEvent(const PendingEvent& ev) {
    const auto& ctx = ev.context_owner;
    if (!ctx) {
        LOG_WARNING(Lib_NpMatching2, "type={} dropped: ctx={} not found", static_cast<s32>(ev.type),
                    ev.ctx_id);
        return;
    }
    switch (ev.type) {
    case PendingEvent::CONTEXT_CB:
        if (ctx->context_callback) {
            LOG_INFO(Lib_NpMatching2, "callback CONTEXT ctx={} event={:#x} cause={} err={:#x}",
                     ev.ctx_id, static_cast<u16>(ev.ctx_event), static_cast<u8>(ev.ctx_event_cause),
                     ev.error_code);
            ctx->context_callback(ev.ctx_id, ev.ctx_event, ev.ctx_event_cause, ev.error_code,
                                  ctx->context_callback_arg);
        } else {
            LOG_WARNING(Lib_NpMatching2, "callback CONTEXT ctx={} event={:#x} SKIPPED: no callback",
                        ev.ctx_id, static_cast<u16>(ev.ctx_event));
        }
        if (ev.ctx_event == ORBIS_NP_MATCHING2_CONTEXT_EVENT_STOPPED) {
            GetContextManager()->CompleteStop(ev.ctx_id, ctx);
        }
        break;
    case PendingEvent::REQUEST_CB:
        if (ev.request_cb) {
            LOG_INFO(Lib_NpMatching2,
                     "callback REQUEST ctx={} req_id={} event={:#x} err={:#x} data={}", ev.ctx_id,
                     ev.req_id, static_cast<u16>(ev.req_event), ev.error_code,
                     fmt::ptr(ev.request_data));
            ev.request_cb(ev.ctx_id, ev.req_id, ev.req_event, ev.error_code, ev.request_data,
                          ev.request_cb_arg);
        } else {
            LOG_WARNING(Lib_NpMatching2,
                        "callback REQUEST ctx={} req_id={} event={:#x} SKIPPED: no callback",
                        ev.ctx_id, ev.req_id, static_cast<u16>(ev.req_event));
        }
        break;
    case PendingEvent::SIGNALING_CB:
        if (ctx->signaling_callback) {
            LOG_INFO(Lib_NpMatching2,
                     "callback SIGNALING ctx={} room={} member={} event={:#x} err={:#x}", ev.ctx_id,
                     ev.room_id, ev.member_id, static_cast<u16>(ev.sig_event), ev.error_code);
            ctx->signaling_callback(ev.ctx_id, ev.room_id, ev.member_id, ev.sig_event,
                                    ev.error_code, ctx->signaling_callback_arg);
        } else {
            LOG_WARNING(Lib_NpMatching2,
                        "callback SIGNALING ctx={} room={} event={:#x} SKIPPED: no callback",
                        ev.ctx_id, ev.room_id, static_cast<u16>(ev.sig_event));
        }
        break;
    case PendingEvent::ROOM_EVENT_CB:
        if (ctx->room_event_callback) {
            LOG_INFO(Lib_NpMatching2, "callback ROOM_EVENT ctx={} room={} event={:#x} data={}",
                     ev.ctx_id, ev.room_id, static_cast<u16>(ev.room_event),
                     fmt::ptr(ev.room_event_data));
            ctx->room_event_callback(ev.ctx_id, ev.room_id, ev.room_event, ev.room_event_data,
                                     ctx->room_event_callback_arg);
        } else {
            LOG_WARNING(Lib_NpMatching2,
                        "callback ROOM_EVENT ctx={} room={} event={:#x} SKIPPED: no callback",
                        ev.ctx_id, ev.room_id, static_cast<u16>(ev.room_event));
        }
        break;
    case PendingEvent::LOBBY_EVENT_CB:
        if (ctx->lobby_event_callback) {
            LOG_INFO(Lib_NpMatching2, "callback LOBBY_EVENT ctx={} lobby={} event={:#x} data={}",
                     ev.ctx_id, ev.lobby_id, static_cast<u16>(ev.lobby_event),
                     fmt::ptr(ev.lobby_event_data));
            ctx->lobby_event_callback(ev.ctx_id, ev.lobby_id, ev.lobby_event, ev.lobby_event_data,
                                      ctx->lobby_event_callback_arg);
        } else {
            LOG_WARNING(Lib_NpMatching2,
                        "callback LOBBY_EVENT ctx={} lobby={} event={:#x} SKIPPED: no callback",
                        ev.ctx_id, ev.lobby_id, static_cast<u16>(ev.lobby_event));
        }
        break;
    case PendingEvent::LOBBY_MESSAGE_CB:
        if (ctx->lobby_message_callback) {
            LOG_INFO(Lib_NpMatching2, "callback LOBBY_MESSAGE ctx={} lobby={} src={} event={:#x}",
                     ev.ctx_id, ev.lobby_id, ev.src_member_id, static_cast<u16>(ev.msg_event));
            ctx->lobby_message_callback(ev.ctx_id, ev.lobby_id, ev.src_member_id, ev.msg_event,
                                        ev.message_data, ctx->lobby_message_callback_arg);
        } else {
            LOG_WARNING(Lib_NpMatching2,
                        "callback LOBBY_MESSAGE ctx={} lobby={} SKIPPED: no callback", ev.ctx_id,
                        ev.lobby_id);
        }
        break;
    case PendingEvent::ROOM_MESSAGE_CB:
        if (ctx->room_message_callback) {
            LOG_INFO(Lib_NpMatching2,
                     "callback ROOM_MESSAGE ctx={} room={} src={} event={:#x} cb={} data={} "
                     "userdata={} a_variant={}",
                     ev.ctx_id, ev.room_id, ev.src_member_id, static_cast<u16>(ev.msg_event),
                     fmt::ptr((void*)ctx->room_message_callback), fmt::ptr(ev.message_data),
                     fmt::ptr((void*)ctx->room_message_callback_arg), ctx->a_variant);
            if (ctx->a_variant && ev.message_data) {
                const auto* info =
                    static_cast<const OrbisNpMatching2RoomMessageInfoA*>(ev.message_data);
                LOG_DEBUG(Lib_NpMatching2,
                          "callback ROOM_MESSAGE_A data: filtered={} cast={} dst={} src_member={} "
                          "msg={} msg_len={}",
                          info->filtered, info->cast_type, fmt::ptr(info->dst),
                          fmt::ptr(&info->src_member), fmt::ptr(info->msg), info->msg_len);
            } else if (ev.message_data) {
                const auto* info =
                    static_cast<const OrbisNpMatching2RoomMessageInfo*>(ev.message_data);
                LOG_DEBUG(Lib_NpMatching2,
                          "callback ROOM_MESSAGE data: filtered={} cast={} dst={} src_np_id={} "
                          "msg={} msg_len={}",
                          info->filtered, info->cast_type, fmt::ptr(info->dst),
                          fmt::ptr(info->src_member), fmt::ptr(info->msg), info->msg_len);
            }
            ctx->room_message_callback(ev.ctx_id, ev.room_id, ev.src_member_id, ev.msg_event,
                                       ev.message_data, ctx->room_message_callback_arg);
        } else {
            LOG_WARNING(Lib_NpMatching2,
                        "callback ROOM_MESSAGE ctx={} room={} SKIPPED: no callback", ev.ctx_id,
                        ev.room_id);
        }
        break;
    }
}

void* EventDispatcherThreadMain(void*) {
    Common::SetCurrentThreadName("Matching2:Dispatch");
    auto* state = GetState();
    std::unique_lock lock(state->queue_mutex);
    while (state->dispatch_running.load()) {
        const auto now = std::chrono::steady_clock::now();

        std::vector<PendingEvent> ready;
        auto nearest = std::chrono::steady_clock::time_point::max();
        for (auto it = state->pending_events.begin(); it != state->pending_events.end();) {
            if (it->fire_at <= now) {
                ready.push_back(std::move(*it));
                it = state->pending_events.erase(it);
            } else {
                nearest = std::min(nearest, it->fire_at);
                ++it;
            }
        }

        if (!ready.empty()) {
            lock.unlock();
            for (const PendingEvent& ev : ready) {
                FireEvent(ev);
            }
            lock.lock();
            continue;
        }

        if (nearest == std::chrono::steady_clock::time_point::max()) {
            state->queue_cv.wait(lock);
        } else {
            state->queue_cv.wait_until(lock, nearest);
        }
    }
    return nullptr;
}

void InitEventDispatcher() {
    auto* state = GetState();
    if (state->dispatch_running.exchange(true)) {
        return;
    }
    const s32 rc =
        sceNpCreateThread(&state->dispatch_thread, EventDispatcherThreadMain, nullptr, 700,
                          ORBIS_NP_MATCHING2_THREAD_STACK_SIZE_DEFAULT, 0, "SceNpMatching2Ex");
    if (rc < 0) {
        LOG_ERROR(Lib_NpMatching2, "failed to create event dispatcher thread: {:#x}", rc);
        state->dispatch_running.store(false);
        state->dispatch_thread = 0;
        return;
    }
    LOG_INFO(Lib_NpMatching2, "event dispatcher thread created");
}

void TermEventDispatcher() {
    auto* state = GetState();
    if (!state->dispatch_running.exchange(false)) {
        return;
    }
    state->queue_cv.notify_all();
    if (state->dispatch_thread) {
        sceNpJoinThread(state->dispatch_thread, nullptr);
        state->dispatch_thread = 0;
    }
    std::lock_guard lock(state->queue_mutex);
    state->pending_events.clear();
}

void ScheduleEvent(PendingEvent ev) {
    if (!ev.context_owner) {
        LOG_WARNING(Lib_NpMatching2, "type={} dropped: ctx={} not found", static_cast<s32>(ev.type),
                    ev.ctx_id);
        return;
    }
    auto* state = GetState();
    {
        std::lock_guard lock(state->queue_mutex);
        state->pending_events.push_back(std::move(ev));
    }
    state->queue_cv.notify_all();
}

} // namespace Libraries::Np::NpMatching2
