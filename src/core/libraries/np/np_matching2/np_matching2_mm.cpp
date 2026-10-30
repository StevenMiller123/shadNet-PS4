// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <arpa/inet.h>
#include <fmt/format.h>
#include <orbis/Net.h>

#include "common/logging/log.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_matching2/np_matching2_internal.h"
#include "core/libraries/np/np_matching2/np_matching2_mm.h"
#include "core/libraries/np/np_matching2/np_matching2_signaling.h"
#include "core/libraries/np/np_signaling/np_signaling_handler.h"
#include "core/libraries/np/np_signaling/np_signaling_transport.h"
#include "core/net_util/net_p2p.h"
#include "shadnet.pb.h"
#include "shadnet/client.h"

namespace Libraries::Np::NpMatching2 {

struct PendingRequest {
    OrbisNpMatching2ContextId ctx_id = 0;
    OrbisNpMatching2RequestId req_id = 0;
    OrbisNpMatching2Event req_event{};
    bool a_variant = false;
    OrbisNpMatching2RequestCallback request_cb = nullptr;
    void* request_cb_arg = nullptr;
};

struct MmClientState {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    std::mutex mutex;
    u32 server_addr = 0;
    u16 server_udp_port = 0;
    bool matching2_enabled = false;

    std::mutex pending_mutex;
    std::map<u64, PendingRequest> pending;

    std::mutex sig_mutex;
    std::condition_variable sig_cv;
    std::map<u64, std::pair<ShadNet::ErrorType, std::vector<u8>>> sig_replies;
};

MmClientState g_mm;

bool IsMatching2BackendDisabled() {
    std::lock_guard lock(g_mm.mutex);
    return !g_mm.matching2_enabled;
}

bool IsContextLifecycleEvent(OrbisNpMatching2Event event) {
    return event == ORBIS_NP_MATCHING2_CONTEXT_EVENT_STARTED ||
           event == ORBIS_NP_MATCHING2_CONTEXT_EVENT_STOPPED;
}

template <typename T>
std::vector<u8> MakeProtoPayload(const T& msg) {
    std::string bytes;
    (void)msg.SerializeToString(&bytes);
    const u32 len = static_cast<u32>(bytes.size());
    std::vector<u8> out;
    out.reserve(4 + bytes.size());
    out.push_back(static_cast<u8>(len));
    out.push_back(static_cast<u8>(len >> 8));
    out.push_back(static_cast<u8>(len >> 16));
    out.push_back(static_cast<u8>(len >> 24));
    out.insert(out.end(), bytes.begin(), bytes.end());
    return out;
}

std::string HexPreview(const void* data, u32 size, u32 max_bytes = 96) {
    if (!data || size == 0) {
        return {};
    }

    const auto* bytes = static_cast<const u8*>(data);
    const u32 count = std::min(size, max_bytes);
    std::string out;
    out.reserve(count * 3 + 16);
    char byte_text[4]{};
    for (u32 i = 0; i < count; ++i) {
        std::snprintf(byte_text, sizeof(byte_text), "%02x", bytes[i]);
        if (!out.empty()) {
            out.push_back(' ');
        }
        out += byte_text;
    }
    if (count < size) {
        out += " ...";
    }
    return out;
}

void LogBinAttr(std::string_view prefix, u64 index, const OrbisNpMatching2BinAttr& attr) {
    LOG_INFO(Lib_NpMatching2, "{}[{}]: id={:#x} size={} data={}", prefix, index, attr.id,
             attr.data_size, HexPreview(attr.data, attr.data_size));
}

void LogIntAttr(std::string_view prefix, u64 index, const OrbisNpMatching2IntAttr& attr) {
    LOG_INFO(Lib_NpMatching2, "{}[{}]: id={:#x} value={}", prefix, index, attr.id, attr.num);
}

void AppendBinAttr(shadnet::MatchingBinAttr* dst, const OrbisNpMatching2BinAttr& src) {
    dst->set_attr_id(src.id);
    if (src.data && src.data_size > 0) {
        dst->set_data(src.data, src.data_size);
    }
}

void AppendCreateRoomBinAttrById(shadnet::CreateRoomRequest& req,
                                 const OrbisNpMatching2BinAttr& src, std::string_view source) {
    switch (src.id) {
    case ORBIS_NP_MATCHING2_ROOM_BIN_ATTR_INTERNAL_1_ID:
    case ORBIS_NP_MATCHING2_ROOM_BIN_ATTR_INTERNAL_2_ID:
        AppendBinAttr(req.add_internal_bin_attrs(), src);
        LOG_INFO(Lib_NpMatching2, "routed bin attr id={:#x} from {} to internal size={}", src.id,
                 source, src.data_size);
        break;
    case ORBIS_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_1_ID:
    case ORBIS_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_2_ID:
        AppendBinAttr(req.add_external_bin_attrs(), src);
        LOG_INFO(Lib_NpMatching2, "routed bin attr id={:#x} from {} to external size={}", src.id,
                 source, src.data_size);
        break;
    case ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_BIN_ATTR_EXTERNAL_1_ID:
        AppendBinAttr(req.add_external_search_bin_attrs(), src);
        LOG_INFO(Lib_NpMatching2, "routed bin attr id={:#x} from {} to search external size={}",
                 src.id, source, src.data_size);
        break;
    default:
        LOG_WARNING(Lib_NpMatching2, "unexpected bin attr id={:#x} from {} size={}", src.id, source,
                    src.data_size);
        break;
    }
}

void AppendIntAttr(shadnet::MatchingIntAttr* dst, const OrbisNpMatching2IntAttr& src) {
    dst->set_attr_id(src.id);
    dst->set_attr_value(src.num);
}

void AppendRoomGroupConfig(shadnet::MatchingRoomGroupConfig* dst,
                           const OrbisNpMatching2RoomGroupConfig& src) {
    dst->set_slot_count(src.slots);
    dst->set_has_label(src.has_label);
    dst->set_has_passwd(src.has_password);
    if (src.has_label) {
        dst->set_label(src.label.data, sizeof(src.label.data));
    }
}

void AppendCreateJoinRoomCommon(shadnet::CreateRoomRequest& req, const void* room_passwd,
                                const OrbisNpMatching2RoomPasswordSlotMask* passwd_slot_mask,
                                const void* group_config, u64 group_configs,
                                const void* join_group_label) {
    if (room_passwd) {
        const auto* password = static_cast<const OrbisNpMatching2SessionPassword*>(room_passwd);
        req.set_room_password(password->data, sizeof(password->data));
    }
    if (passwd_slot_mask) {
        req.set_has_passwd_slot_mask(true);
        req.set_passwd_slot_mask(*passwd_slot_mask);
    }
    if (group_config) {
        const auto* groups = static_cast<const OrbisNpMatching2RoomGroupConfig*>(group_config);
        for (u64 i = 0; i < group_configs; ++i) {
            AppendRoomGroupConfig(req.add_group_configs(), groups[i]);
        }
    }
    if (join_group_label) {
        const auto* label = static_cast<const OrbisNpMatching2GroupLabel*>(join_group_label);
        req.set_join_group_label(label->data, sizeof(label->data));
    }
}

std::string ExtractProtoBytes(const std::vector<u8>& payload, u64 offset = 0) {
    if (payload.size() < offset + 4) {
        return {};
    }
    const u32 len = static_cast<u32>(payload[offset]) |
                    (static_cast<u32>(payload[offset + 1]) << 8) |
                    (static_cast<u32>(payload[offset + 2]) << 16) |
                    (static_cast<u32>(payload[offset + 3]) << 24);
    if (payload.size() < offset + 4 + len) {
        return {};
    }
    return std::string(reinterpret_cast<const char*>(payload.data() + offset + 4), len);
}

void DispatchRequestComplete(const PendingRequest& pr, ShadNet::ErrorType error,
                             const std::vector<u8>& body) {
    auto ctx = GetContextManager()->Get(pr.ctx_id);
    if (!ctx) {
        return;
    }

    const s32 error_code = (error == ShadNet::ErrorType::NoError) ? 0 : static_cast<s32>(error);

    if (pr.req_event == ORBIS_NP_MATCHING2_CONTEXT_EVENT_STARTED ||
        pr.req_event == ORBIS_NP_MATCHING2_CONTEXT_EVENT_STOPPED) {
        PendingEvent ev{};
        ev.type = PendingEvent::CONTEXT_CB;
        ev.ctx_id = pr.ctx_id;
        ev.context_owner = ctx;
        ev.fire_at = std::chrono::steady_clock::now();
        ev.ctx_event = pr.req_event;
        ev.ctx_event_cause = ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION;
        ev.error_code = error_code;
        ScheduleEvent(std::move(ev));
        return;
    }

    void* request_data = nullptr;
    std::shared_ptr<CallbackPayload> request_payload_owner;

    if (error_code == 0) {
        request_payload_owner = std::make_shared<CallbackPayload>();
        auto& payload = *request_payload_owner;
        const std::string proto = ExtractProtoBytes(body);
        if (pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM ||
            pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM_A) {
            shadnet::CreateRoomReply reply;
            if (reply.ParseFromString(proto) && reply.has_details()) {
                request_data = pr.a_variant
                                   ? BuildCreateJoinRoomPayloadA(*ctx, payload, reply.details())
                                   : BuildCreateJoinRoomPayload(*ctx, payload, reply.details());
            }
        } else if (pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM ||
                   pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM_A) {
            shadnet::JoinRoomReply reply;
            if (reply.ParseFromString(proto) && reply.has_details()) {
                request_data = pr.a_variant
                                   ? BuildCreateJoinRoomPayloadA(*ctx, payload, reply.details())
                                   : BuildCreateJoinRoomPayload(*ctx, payload, reply.details());
            }
        } else if (pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_LEAVE_ROOM) {
            shadnet::LeaveRoomReply reply;
            if (reply.ParseFromString(proto)) {
                QueueMatching2DeadForRoomPeers(
                    *ctx, static_cast<OrbisNpMatching2RoomId>(reply.room_id()),
                    ORBIS_NP_MATCHING2_SIGNALING_ERROR_TERMINATED_BY_MYSELF);
                request_data = BuildLeaveRoomPayload(*ctx, payload, reply);
            }
        } else if (pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_WORLD_INFO_LIST) {
            shadnet::GetWorldInfoListReply reply;
            if (reply.ParseFromString(proto)) {
                request_data = BuildGetWorldInfoListPayload(*ctx, payload, reply);
            }
        } else if (pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_SEARCH_ROOM ||
                   pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_SEARCH_ROOM_A) {
            shadnet::SearchRoomReply reply;
            if (reply.ParseFromString(proto)) {
                request_data = pr.a_variant ? BuildSearchRoomPayloadA(*ctx, payload, reply)
                                            : BuildSearchRoomPayload(*ctx, payload, reply);
            }
        } else if (pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_DATA_EXTERNAL_LIST ||
                   pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_DATA_EXTERNAL_LIST_A) {
            shadnet::GetRoomDataExternalListReply reply;
            if (reply.ParseFromString(proto)) {
                request_data = pr.a_variant
                                   ? BuildGetRoomDataExternalListPayloadA(*ctx, payload, reply)
                                   : BuildGetRoomDataExternalListPayload(*ctx, payload, reply);
            }
        } else if (pr.req_event ==
                       ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_MEMBER_DATA_EXTERNAL_LIST ||
                   pr.req_event ==
                       ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_MEMBER_DATA_EXTERNAL_LIST_A) {
            shadnet::GetRoomMemberDataExternalListReply reply;
            if (reply.ParseFromString(proto)) {
                request_data =
                    pr.a_variant ? BuildGetRoomMemberDataExternalListPayloadA(*ctx, payload, reply)
                                 : BuildGetRoomMemberDataExternalListPayload(*ctx, payload, reply);
            }
        } else if (pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_USER_INFO_LIST) {
            shadnet::GetUserInfoListReply reply;
            if (reply.ParseFromString(proto)) {
                request_data = pr.a_variant ? BuildGetUserInfoListPayloadA(*ctx, payload, reply)
                                            : BuildGetUserInfoListPayload(*ctx, payload, reply);
            }
        }
    }

    PendingEvent ev{};
    ev.type = PendingEvent::REQUEST_CB;
    ev.ctx_id = pr.ctx_id;
    ev.context_owner = ctx;
    ev.fire_at = std::chrono::steady_clock::now();
    ev.req_id = pr.req_id;
    ev.req_event = pr.req_event;
    ev.error_code = error_code;
    ev.request_cb = pr.request_cb;
    ev.request_cb_arg = pr.request_cb_arg;
    ev.request_data = request_data;
    ev.payload_owner = std::move(request_payload_owner);
    ScheduleEvent(std::move(ev));

    if (error_code == 0 && (pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM ||
                            pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM_A)) {
        NpSignaling::StartMatching2(*ctx);
    } else if (error_code == 0 &&
               (pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM ||
                pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM_A)) {
        NpSignaling::StartMatching2(*ctx);
    }
}

void AppendEventMemberBinAttrs(CallbackPayload& p, const MemberCache& mc) {
    p.member_bin_attrs.resize(mc.bins.size());
    p.bin_buffers.reserve(mc.bins.size());
    u64 i = 0;
    for (const auto& [attr_id, bin] : mc.bins) {
        p.bin_buffers.emplace_back(bin.data.begin(), bin.data.end());
        auto& buf = p.bin_buffers.back();
        auto& dst = p.member_bin_attrs[i++];
        dst = OrbisNpMatching2RoomMemberBinAttrInternal{};
        dst.last_update.tick = bin.update_date;
        dst.bin_attr.id = bin.id;
        dst.bin_attr.data = buf.empty() ? nullptr : buf.data();
        dst.bin_attr.data_size = buf.size();
    }
}

void BuildMemberData(CallbackPayload& p, const RoomCache& rc, const MemberCache& mc,
                     bool a_variant) {
    if (mc.group_id != 0) {
        auto grp_it = rc.groups.find(mc.group_id);
        if (grp_it != rc.groups.end()) {
            p.room_groups.resize(1);
            p.room_groups[0] = grp_it->second;
        }
    }

    AppendEventMemberBinAttrs(p, mc);

    if (a_variant) {
        p.event_member_a = std::make_unique<OrbisNpMatching2RoomMemberDataInternalA>();
        OrbisNpMatching2RoomMemberDataInternalA& m = *p.event_member_a;
        m = OrbisNpMatching2RoomMemberDataInternalA{};
        m.member_id = mc.member_id;
        m.team_id = mc.team_id;
        m.nat_type = mc.nat_type;
        m.flags = mc.flag_attr;
        m.join_date_ticks.tick = mc.join_date;
        m.user.account_id = mc.account_id;
        m.user.platform = mc.platform;
        m.online_id = mc.np_id.handle;
        m.room_group = p.room_groups.empty() ? nullptr : p.room_groups.data();
        m.room_member_internal_bin_attr =
            p.member_bin_attrs.empty() ? nullptr : p.member_bin_attrs.data();
        m.room_member_internal_bin_attrs = p.member_bin_attrs.size();

        return;
    }

    p.event_member = std::make_unique<OrbisNpMatching2RoomMemberDataInternal>();
    OrbisNpMatching2RoomMemberDataInternal& m = *p.event_member;
    m = OrbisNpMatching2RoomMemberDataInternal{};
    m.member_id = mc.member_id;
    m.team_id = mc.team_id;
    m.nat_type = mc.nat_type;
    m.flag_attr = mc.flag_attr;
    m.join_date = mc.join_date;
    m.np_id = mc.np_id;
    m.room_group = p.room_groups.empty() ? nullptr : p.room_groups.data();
    m.room_member_bin_attr_internal =
        p.member_bin_attrs.empty() ? nullptr : p.member_bin_attrs.data();
    m.room_member_bin_attr_internal_num = p.member_bin_attrs.size();
}

void BuildMemberUpdate(CallbackPayload& p, const RoomCache& rc, const MemberCache& mc,
                       OrbisNpMatching2EventCause cause, bool a_variant) {
    BuildMemberData(p, rc, mc, a_variant);

    if (a_variant) {
        p.room_member_update_a = std::make_unique<OrbisNpMatching2RoomMemberUpdateA>();
        auto& u = *p.room_member_update_a;
        u = {};
        u.room_member_data_internal = p.event_member_a.get();
        u.event_cause = cause;
        p.room_event_data = p.room_member_update_a.get();
        return;
    }

    p.room_member_update = std::make_unique<OrbisNpMatching2RoomMemberUpdate>();
    auto& u = *p.room_member_update;
    u = {};
    u.room_member_data_internal = p.event_member.get();
    u.event_cause = cause;
    p.room_event_data = p.room_member_update.get();
}

void BuildMemberDataInternalUpdate(CallbackPayload& p, const RoomCache& rc, const MemberCache& mc,
                                   OrbisNpMatching2FlagAttr prev_flag_attr,
                                   const std::vector<OrbisNpMatching2AttributeId>& attr_ids,
                                   bool a_variant) {
    BuildMemberData(p, rc, mc, a_variant);

    p.event_chg_flag_attr = std::make_unique<OrbisNpMatching2FlagAttr>(
        static_cast<OrbisNpMatching2FlagAttr>(mc.flag_attr));
    p.event_prev_flag_attr = std::make_unique<OrbisNpMatching2FlagAttr>(prev_flag_attr);
    p.event_chg_team_id = std::make_unique<OrbisNpMatching2TeamId>(mc.team_id);

    p.member_bin_attr_ptrs.reserve(attr_ids.size());
    for (const auto attr_id : attr_ids) {
        const auto attr =
            std::ranges::find_if(p.member_bin_attrs, [attr_id](const auto& candidate) {
                return candidate.bin_attr.id == attr_id;
            });
        if (attr != p.member_bin_attrs.end()) {
            p.member_bin_attr_ptrs.push_back(&*attr);
        }
    }

    if (a_variant) {
        p.room_member_data_internal_update_a =
            std::make_unique<OrbisNpMatching2RoomMemberDataInternalUpdateA>();
        auto& u = *p.room_member_data_internal_update_a;
        u = {};
        u.chg_room_member_data_internal = p.event_member_a.get();
        u.chg_flag_attr = p.event_chg_flag_attr.get();
        u.prev_flag_attr = p.event_prev_flag_attr.get();
        u.chg_team_id = p.event_chg_team_id.get();
        u.chg_room_member_bin_attr_internal =
            p.member_bin_attr_ptrs.empty() ? nullptr : p.member_bin_attr_ptrs.data();
        u.chg_room_member_bin_attr_internal_num = p.member_bin_attr_ptrs.size();
        p.room_event_data = p.room_member_data_internal_update_a.get();
        return;
    }

    p.room_member_data_internal_update =
        std::make_unique<OrbisNpMatching2RoomMemberDataInternalUpdate>();
    auto& u = *p.room_member_data_internal_update;
    u = {};
    u.chg_room_member_data_internal = p.event_member.get();
    u.chg_flag_attr = p.event_chg_flag_attr.get();
    u.prev_flag_attr = p.event_prev_flag_attr.get();
    u.chg_team_id = p.event_chg_team_id.get();
    u.chg_room_member_bin_attr_internal =
        p.member_bin_attr_ptrs.empty() ? nullptr : p.member_bin_attr_ptrs.data();
    u.chg_room_member_bin_attr_internal_num = p.member_bin_attr_ptrs.size();
    p.room_event_data = p.room_member_data_internal_update.get();
}

void HandleRoomEvent(const ShadNet::NotifyRoomEvent& n) {
    auto ctx = GetContextManager()->Get(static_cast<OrbisNpMatching2ContextId>(n.ctx_id));
    if (!ctx) {
        return;
    }
    const auto event = static_cast<OrbisNpMatching2Event>(n.event);
    const auto cause = static_cast<OrbisNpMatching2EventCause>(n.event_cause);
    const auto room_id = static_cast<OrbisNpMatching2RoomId>(n.room_id);
    const auto member_id = static_cast<OrbisNpMatching2RoomMemberId>(n.member_id);

    auto payload_owner = std::make_shared<CallbackPayload>();
    CallbackPayload& p = *payload_owner;

    {
        auto cache = LockCache(ctx->ctx_id);
        switch (event) {
        case ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED: {
            auto room_it = cache->rooms.find(room_id);
            if (room_it == cache->rooms.end()) {
                return;
            }
            MemberCache& mc = room_it->second.members[member_id];
            mc = MemberCache{};
            mc.member_id = member_id;
            mc.team_id = static_cast<OrbisNpMatching2TeamId>(n.member_team_id);
            mc.nat_type = static_cast<OrbisNpMatching2NatType>(n.member_nat_type);
            mc.flag_attr = n.member_flag_attr;
            mc.group_id = static_cast<OrbisNpMatching2RoomGroupId>(n.member_group_id);
            mc.join_date = n.member_join_date;
            mc.addr = IpStringToAddr(n.member_addr);
            mc.port = htons(static_cast<u16>(n.member_port));
            SetNpId(mc.np_id, n.member_npid);
            mc.account_id = static_cast<OrbisNpAccountId>(n.member_account_id);
            mc.platform = static_cast<OrbisNpPlatformType>(n.member_platform);
            for (const auto& a : n.member_bin_attrs) {
                MemberBinCache& b = mc.bins[a.attr_id];
                b.id = static_cast<OrbisNpMatching2AttributeId>(a.attr_id);
                b.data = a.data;
            }

            PeerInfo pi{};
            pi.member_id = member_id;
            pi.addr = mc.addr;
            pi.port = mc.port;
            SetNpOnlineId(pi.online_id, n.member_npid);
            cache->peers[member_id] = pi;

            BuildMemberUpdate(p, room_it->second, mc, cause, ctx->a_variant);
            break;
        }
        case ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_LEFT: {
            auto room_it = cache->rooms.find(room_id);
            if (room_it == cache->rooms.end()) {
                return;
            }
            auto mem_it = room_it->second.members.find(member_id);
            if (mem_it == room_it->second.members.end()) {
                return;
            }
            BuildMemberUpdate(p, room_it->second, mem_it->second, cause, ctx->a_variant);
            QueueMatching2SignalingEvent(*ctx, room_id, member_id,
                                         ORBIS_NP_MATCHING2_SIGNALING_EVENT_DEAD,
                                         ORBIS_NP_MATCHING2_SIGNALING_ERROR_TERMINATED_BY_PEER);
            room_it->second.members.erase(mem_it);
            cache->peers.erase(member_id);
            break;
        }
        case ORBIS_NP_MATCHING2_ROOM_EVENT_KICKEDOUT: {
            p.room_update = std::make_unique<OrbisNpMatching2RoomUpdate>();
            OrbisNpMatching2RoomUpdate& u = *p.room_update;
            u = OrbisNpMatching2RoomUpdate{};
            u.event_cause = cause;
            u.error_code = n.error_code;
            p.room_event_data = p.room_update.get();
            break;
        }
        case ORBIS_NP_MATCHING2_ROOM_EVENT_UPDATED_ROOM_DATA_INTERNAL: {
            auto room_it = cache->rooms.find(room_id);
            if (room_it == cache->rooms.end()) {
                return;
            }
            RoomCache& rc = room_it->second;
            const auto prev_flags = static_cast<OrbisNpMatching2FlagAttr>(rc.flags);
            const auto prev_passwd_slot_mask = rc.passwd_slot_mask;
            rc.flags = n.flags;
            if (n.has_passwd_mask) {
                rc.passwd_slot_mask = n.passwd_slot_mask;
            }

            std::vector<OrbisNpMatching2RoomBinAttrInternal> changed_attrs;
            changed_attrs.reserve(n.bin_attrs.size());
            for (const auto& a : n.bin_attrs) {
                rc.bin_buffers.emplace_back(a.data.begin(), a.data.end());
                auto& buf = rc.bin_buffers.back();

                OrbisNpMatching2RoomBinAttrInternal updated{};
                updated.bin_attr.id = static_cast<OrbisNpMatching2AttributeId>(a.attr_id);
                updated.bin_attr.data = buf.empty() ? nullptr : buf.data();
                updated.bin_attr.data_size = buf.size();
                updated.last_update.tick = a.update_date;
                updated.member_id = static_cast<OrbisNpMatching2RoomMemberId>(a.update_member_id);

                bool replaced = false;
                for (auto& existing : rc.bin_attrs_internal) {
                    if (existing.bin_attr.id == updated.bin_attr.id) {
                        existing = updated;
                        replaced = true;
                        break;
                    }
                }
                if (!replaced) {
                    rc.bin_attrs_internal.push_back(updated);
                }
                changed_attrs.push_back(updated);
            }

            p.room_groups.reserve(rc.groups.size());
            for (const auto& [gid, group] : rc.groups) {
                p.room_groups.push_back(group);
            }
            p.room_group_ptrs.reserve(p.room_groups.size());
            for (auto& group : p.room_groups) {
                p.room_group_ptrs.push_back(&group);
            }

            p.room_bin_attrs.resize(changed_attrs.size());
            p.bin_buffers.clear();
            p.bin_buffers.reserve(changed_attrs.size());
            for (u64 i = 0; i < changed_attrs.size(); ++i) {
                const auto& src = changed_attrs[i];
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
                dst.member_id = src.member_id;
                dst.bin_attr.id = src.bin_attr.id;
                dst.bin_attr.data = buf.empty() ? nullptr : buf.data();
                dst.bin_attr.data_size = buf.size();
            }
            p.room_bin_attr_ptrs.reserve(p.room_bin_attrs.size());
            for (auto& attr : p.room_bin_attrs) {
                p.room_bin_attr_ptrs.push_back(&attr);
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
            room.room_group = p.room_groups.empty() ? nullptr : p.room_groups.data();
            room.room_groups = p.room_groups.size();
            room.flags = rc.flags;
            room.internal_bin_attr = p.room_bin_attrs.empty() ? nullptr : p.room_bin_attrs.data();
            room.internal_bin_attrs = p.room_bin_attrs.size();

            p.event_chg_flag_attr = std::make_unique<OrbisNpMatching2FlagAttr>(
                static_cast<OrbisNpMatching2FlagAttr>(rc.flags));
            p.event_prev_flag_attr = std::make_unique<OrbisNpMatching2FlagAttr>(prev_flags);
            p.event_chg_passwd_slot_mask =
                std::make_unique<OrbisNpMatching2RoomPasswordSlotMask>(rc.passwd_slot_mask);
            p.event_prev_passwd_slot_mask =
                std::make_unique<OrbisNpMatching2RoomPasswordSlotMask>(prev_passwd_slot_mask);

            p.room_data_internal_update =
                std::make_unique<OrbisNpMatching2RoomDataInternalUpdate>();
            auto& u = *p.room_data_internal_update;
            u = {};
            u.chg_room_data_internal = p.room_data.get();
            u.chg_flag_attr = p.event_chg_flag_attr.get();
            u.prev_flag_attr = p.event_prev_flag_attr.get();
            u.chg_room_password_slot_mask = p.event_chg_passwd_slot_mask.get();
            u.prev_room_password_slot_mask = p.event_prev_passwd_slot_mask.get();
            u.chg_room_group = p.room_group_ptrs.empty() ? nullptr : p.room_group_ptrs.data();
            u.chg_room_group_num = p.room_group_ptrs.size();
            u.chg_room_bin_attr_internal =
                p.room_bin_attr_ptrs.empty() ? nullptr : p.room_bin_attr_ptrs.data();
            u.chg_room_bin_attr_internal_num = p.room_bin_attr_ptrs.size();
            p.room_event_data = p.room_data_internal_update.get();
            break;
        }
        case ORBIS_NP_MATCHING2_ROOM_EVENT_UPDATED_ROOM_MEMBER_DATA_INTERNAL: {
            auto room_it = cache->rooms.find(room_id);
            if (room_it == cache->rooms.end()) {
                return;
            }
            auto member_it = room_it->second.members.find(member_id);
            if (member_it == room_it->second.members.end()) {
                return;
            }
            MemberCache& member = member_it->second;
            const auto prev_flag_attr = static_cast<OrbisNpMatching2FlagAttr>(member.flag_attr);
            member.team_id = static_cast<OrbisNpMatching2TeamId>(n.member_team_id);
            member.flag_attr = n.member_flag_attr;
            member.group_id = static_cast<OrbisNpMatching2RoomGroupId>(n.member_group_id);
            std::vector<OrbisNpMatching2AttributeId> attr_ids;
            attr_ids.reserve(n.member_bin_attrs.size());
            for (const auto& attr : n.member_bin_attrs) {
                MemberBinCache& bin = member.bins[attr.attr_id];
                bin.id = static_cast<OrbisNpMatching2AttributeId>(attr.attr_id);
                bin.update_date = attr.update_date;
                bin.data = attr.data;
                attr_ids.push_back(bin.id);
            }
            BuildMemberDataInternalUpdate(p, room_it->second, member, prev_flag_attr, attr_ids,
                                          ctx->a_variant);
            break;
        }
        default:
            LOG_WARNING(Lib_NpMatching2, "unhandled room event {:#x}", n.event);
            return;
        }
    }

    LOG_DEBUG(Lib_NpMatching2, "ctx={} room={} event={:#x} cause={}", n.ctx_id, n.room_id, n.event,
              n.event_cause);

    OrbisNpMatching2Event fired_event = event;
    if (ctx->a_variant) {
        switch (event) {
        case ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED:
            fired_event = ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED_A;
            break;
        case ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_LEFT:
            fired_event = ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_LEFT_A;
            break;
        case ORBIS_NP_MATCHING2_ROOM_EVENT_UPDATED_ROOM_MEMBER_DATA_INTERNAL:
            fired_event = ORBIS_NP_MATCHING2_ROOM_EVENT_UPDATED_ROOM_MEMBER_DATA_INTERNAL_A;
            break;
        default:
            break;
        }
    }

    PendingEvent ev{};
    ev.type = PendingEvent::ROOM_EVENT_CB;
    ev.ctx_id = ctx->ctx_id;
    ev.context_owner = ctx;
    ev.fire_at = std::chrono::steady_clock::now();
    ev.room_id = room_id;
    ev.room_event = fired_event;
    ev.room_event_data = p.room_event_data;
    ev.payload_owner = std::move(payload_owner);
    ScheduleEvent(std::move(ev));

    if (event == ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED) {
        NpSignaling::StartMatching2(*ctx);
    }
}

void HandleRoomMessage(const ShadNet::NotifyRoomMessage& n) {
    auto ctx = GetContextManager()->Get(static_cast<OrbisNpMatching2ContextId>(n.ctx_id));
    if (!ctx) {
        return;
    }

    const auto room_id = static_cast<OrbisNpMatching2RoomId>(n.room_id);
    const auto src_member_id = static_cast<OrbisNpMatching2RoomMemberId>(n.src_member_id);
    MemberCache src_member_copy{};
    bool have_src_member = false;
    {
        auto cache = LockCache(ctx->ctx_id);
        const auto room_it = cache->rooms.find(room_id);
        if (room_it != cache->rooms.end()) {
            const auto member_it = room_it->second.members.find(src_member_id);
            if (member_it != room_it->second.members.end()) {
                src_member_copy = member_it->second;
                have_src_member = true;
            }
        }
    }

    MemberCache notification_src{};
    const MemberCache* src_member = &src_member_copy;
    if (!have_src_member) {
        notification_src.member_id = src_member_id;
        SetNpId(notification_src.np_id, n.src_npid);
        notification_src.account_id = static_cast<OrbisNpAccountId>(n.src_account_id);
        notification_src.platform = static_cast<OrbisNpPlatformType>(n.src_platform);
        src_member = &notification_src;
    }

    std::vector<OrbisNpMatching2RoomMemberId> dst_members;
    dst_members.reserve(n.dst_member_ids.size());
    for (const u32 member_id : n.dst_member_ids) {
        dst_members.push_back(static_cast<OrbisNpMatching2RoomMemberId>(member_id));
    }

    auto payload_owner = std::make_shared<CallbackPayload>();
    void* data = BuildRoomMessagePayload(*payload_owner, ctx->a_variant,
                                         static_cast<OrbisNpMatching2CastType>(n.cast_type),
                                         dst_members, src_member, n.msg);

    PendingEvent ev{};
    ev.type = PendingEvent::ROOM_MESSAGE_CB;
    ev.ctx_id = ctx->ctx_id;
    ev.context_owner = ctx;
    ev.fire_at = std::chrono::steady_clock::now();
    ev.room_id = room_id;
    ev.src_member_id = src_member_id;
    ev.msg_event = ctx->a_variant ? ORBIS_NP_MATCHING2_ROOM_MSG_EVENT_MESSAGE_A
                                  : ORBIS_NP_MATCHING2_ROOM_MSG_EVENT_MESSAGE;
    ev.message_data = data;
    ev.payload_owner = std::move(payload_owner);
    ScheduleEvent(std::move(ev));
}

void OnMatching2Reply(ShadNet::CommandType cmd, u64 pkt_id, ShadNet::ErrorType error,
                      const std::vector<u8>& body) {
    if (cmd == ShadNet::CommandType::RequestSignalingInfos) {
        std::lock_guard lock(g_mm.sig_mutex);
        g_mm.sig_replies[pkt_id] = {error, body};
        g_mm.sig_cv.notify_all();
        return;
    }

    PendingRequest pr;
    {
        std::lock_guard lock(g_mm.pending_mutex);
        auto it = g_mm.pending.find(pkt_id);
        if (it == g_mm.pending.end()) {
            LOG_WARNING(Lib_NpMatching2, "reply pkt_id={} cmd={} has no pending entry (have {})",
                        pkt_id, static_cast<u16>(cmd), g_mm.pending.size());
            return;
        }
        pr = it->second;
        g_mm.pending.erase(it);
    }
    LOG_DEBUG(Lib_NpMatching2, "reply pkt_id={} cmd={} -> ctx={} req_id={} event={:#x}", pkt_id,
              static_cast<u16>(cmd), pr.ctx_id, pr.req_id, static_cast<u16>(pr.req_event));
    DispatchRequestComplete(pr, error, body);
}

void SetMmShadNetClient(std::shared_ptr<ShadNet::ShadNetClient> client,
                        std::string_view server_host, u16 tcp_port) {
    u32 server_addr = 0;
    u16 server_udp_port = 0;
    bool matching2_enabled = false;
    {
        std::lock_guard lock(g_mm.mutex);
        g_mm.client = client;
        g_mm.matching2_enabled = client ? client->IsMatching2Enabled() : false;
        matching2_enabled = g_mm.matching2_enabled;
        g_mm.server_addr = client ? client->GetAddrServer() : 0;
        if (g_mm.server_addr == 0) {
            g_mm.server_addr = IpStringToAddr(server_host);
        }
        g_mm.server_udp_port = matching2_enabled ? htons(static_cast<u16>(tcp_port + 1)) : 0;
        server_addr = g_mm.server_addr;
        server_udp_port = g_mm.server_udp_port;
    }
    LOG_INFO(Lib_NpMatching2, "matching2_enabled={}", matching2_enabled);

    if (!client) {
        NpSignaling::SetTransportHooks({});
        NpSignaling::SetPeerResolver(nullptr);
        NpSignaling::SetMatching2Enabled(false);
        NpSignaling::SetMmServerEndpoint(0, 0);
        StopMatching2SignalingRuntime();
        return;
    }

    NpSignaling::SetTransportHooks({
        .signaling_send = NetUtil::P2PSignalingSendTo,
        .signaling_recv = NetUtil::P2PSignalingRecvFrom,
        .control_send = NetUtil::P2PControlSendTo,
        .control_recv = NetUtil::P2PControlRecvFrom,
        .transport_ready = NetUtil::P2PTransportIsReady,
        .configured_port = NetUtil::GetP2PAdvertisedPort,
        .advertised_addr = NetUtil::GetP2PAdvertisedAddr,
        .ensure_transport = NetUtil::EnsureP2PTransport,
    });
    NpSignaling::SetPeerResolver(matching2_enabled ? RequestSignalingInfos : nullptr);
    NpSignaling::SetMatching2Enabled(matching2_enabled);
    NpSignaling::SetMmServerEndpoint(server_addr, server_udp_port);
    if (!matching2_enabled) {
        StopMatching2SignalingRuntime();
    }
    client->on_room_event = [](const ShadNet::NotifyRoomEvent& n) { HandleRoomEvent(n); };
    client->on_room_message = [](const ShadNet::NotifyRoomMessage& n) { HandleRoomMessage(n); };
}

void ClearMmShadNetClient() {
    std::shared_ptr<ShadNet::ShadNetClient> old_client;
    {
        std::lock_guard lock(g_mm.mutex);
        old_client = std::move(g_mm.client);
        g_mm.server_addr = 0;
        g_mm.server_udp_port = 0;
        g_mm.matching2_enabled = false;
    }
    if (old_client) {
        old_client->on_room_event = nullptr;
        old_client->on_room_message = nullptr;
    }
    NpSignaling::SetTransportHooks({});
    NpSignaling::SetPeerResolver(nullptr);
    NpSignaling::SetMatching2Enabled(false);
    NpSignaling::SetMmServerEndpoint(0, 0);
    StopMatching2SignalingRuntime();
    {
        std::lock_guard lock(g_mm.pending_mutex);
        g_mm.pending.clear();
    }
    g_mm.sig_cv.notify_all();
}

bool IsMmClientRunning() {
    std::lock_guard lock(g_mm.mutex);
    return g_mm.client && g_mm.client->IsAuthenticated();
}

void MmContextStart(OrbisNpMatching2ContextId ctx_id) {
    shadnet::ContextStartRequest req;
    req.set_ctx_id(ctx_id);
    MmSubmitRequest(ctx_id, 0, ORBIS_NP_MATCHING2_CONTEXT_EVENT_STARTED, MmCommand::ContextStart,
                    MakeProtoPayload(req));
    if (auto ctx = GetContextManager()->Get(ctx_id)) {
        SendMatching2StunPing(*ctx);
    }
}

s32 MmContextStop(OrbisNpMatching2ContextId ctx_id) {
    shadnet::ContextStopRequest req;
    req.set_ctx_id(ctx_id);
    return MmSubmitRequest(ctx_id, 0, ORBIS_NP_MATCHING2_CONTEXT_EVENT_STOPPED,
                           MmCommand::ContextStop, MakeProtoPayload(req));
}

s32 MmSubmitRequest(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                    OrbisNpMatching2Event req_event, MmCommand cmd, const std::vector<u8>& payload,
                    bool a_variant) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(g_mm.mutex);
        client = g_mm.client;
    }
    if (!client || !client->IsAuthenticated()) {
        LOG_ERROR(Lib_NpMatching2, "not connected");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    auto ctx = GetContextManager()->Get(ctx_id);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id {}", ctx_id);
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }
    const RequestCallbackInfo request_cb = ConsumeRequestCallback(ctx);
    if (IsMatching2BackendDisabled() && !IsContextLifecycleEvent(req_event)) {
        LOG_INFO(Lib_NpMatching2, "matching2 backend disabled");

        s32 error_code = ORBIS_NP_MATCHING2_ERROR_SERVER_NOT_AVAILABLE;
        void* request_data = nullptr;
        std::shared_ptr<CallbackPayload> request_payload_owner;
        if (req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_WORLD_INFO_LIST) {
            shadnet::GetWorldInfoListReply reply;
            auto* world = reply.add_worlds();
            world->set_world_id(1);
            world->set_lobbies_num(0);
            world->set_max_lobby_members(0);
            world->set_lobby_members_num(0);
            world->set_rooms_num(0);
            world->set_room_members_num(0);
            request_payload_owner = std::make_shared<CallbackPayload>();
            request_data = BuildGetWorldInfoListPayload(*ctx, *request_payload_owner, reply);
            error_code = ORBIS_OK;
        }

        PendingEvent ev{};
        ev.type = PendingEvent::REQUEST_CB;
        ev.ctx_id = ctx_id;
        ev.context_owner = ctx;
        ev.fire_at = std::chrono::steady_clock::now();
        ev.req_id = req_id;
        ev.req_event = req_event;
        ev.error_code = error_code;
        ev.request_cb = request_cb.callback;
        ev.request_cb_arg = request_cb.arg;
        ev.request_data = request_data;
        ev.payload_owner = std::move(request_payload_owner);
        ScheduleEvent(std::move(ev));
        return ORBIS_OK;
    }

    LOG_INFO(Lib_NpMatching2,
             "ctx={} req_id={} event={:#x} cmd={} a_variant={} callback={:#x} "
             "arg={}",
             ctx_id, req_id, static_cast<u16>(req_event), static_cast<u16>(cmd), a_variant,
             reinterpret_cast<std::uintptr_t>(request_cb.callback), fmt::ptr(request_cb.arg));

    const u64 pkt_id = client->SubmitRequest(static_cast<ShadNet::CommandType>(cmd), payload);
    {
        std::lock_guard lock(g_mm.pending_mutex);
        g_mm.pending[pkt_id] = {ctx_id,        req_id, req_event, a_variant, request_cb.callback,
                                request_cb.arg};
    }
    LOG_DEBUG(Lib_NpMatching2, "submit cmd={} pkt_id={} ctx={} req_id={}", static_cast<u16>(cmd),
              pkt_id, ctx_id, req_id);
    return ORBIS_OK;
}

s32 MmCreateJoinRoom(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                     const OrbisNpMatching2CreateJoinRoomRequest& request) {
    shadnet::CreateRoomRequest req;
    req.set_req_id(req_id);
    req.set_max_slots(request.max_slot);
    req.set_team_id(request.team_id);
    req.set_world_id(request.world_id);
    req.set_lobby_id(static_cast<u32>(request.lobby_id));
    req.set_flags(request.flags);
    req.set_group_config_count(static_cast<u32>(request.group_configs));
    req.set_allowed_user_count(static_cast<u32>(request.allowed_users));
    req.set_blocked_user_count(static_cast<u32>(request.blocked_users));
    req.set_internal_bin_attr_count(static_cast<u32>(request.internal_bin_attrs));
    LOG_INFO(Lib_NpMatching2,
             "ctx={} req_id={} max_slot={} world={} lobby={} flags={:#x} "
             "internal_bin={} search_int={} search_bin={} ext_bin={} member_bin={}",
             ctx_id, req_id, request.max_slot, request.world_id, request.lobby_id, request.flags,
             request.internal_bin_attrs, request.external_search_int_attrs,
             request.external_search_bin_attrs, request.external_bin_attrs,
             request.member_internal_bin_attrs);
    AppendCreateJoinRoomCommon(req, request.room_passwd, request.passwd_slot_mask,
                               request.group_config, request.group_configs,
                               request.join_group_label);
    req.set_user_id_kind(shadnet::MATCHING_USER_ID_ONLINE_ID);
    if (request.allowed_user) {
        for (u64 i = 0; i < request.allowed_users; ++i) {
            req.add_allowed_online_ids(OnlineIdToString(request.allowed_user[i]));
        }
    }
    if (request.blocked_user) {
        for (u64 i = 0; i < request.blocked_users; ++i) {
            req.add_blocked_online_ids(OnlineIdToString(request.blocked_user[i]));
        }
    }
    for (u64 i = 0; i < request.internal_bin_attrs; ++i) {
        LogBinAttr("  createInternalBin", i, request.internal_bin_attr[i]);
        AppendCreateRoomBinAttrById(req, request.internal_bin_attr[i], "internalBinAttr");
    }
    for (u64 i = 0; i < request.external_search_int_attrs; ++i) {
        LogIntAttr("  createExternalSearchInt", i, request.external_search_int_attr[i]);
        AppendIntAttr(req.add_external_search_int_attrs(), request.external_search_int_attr[i]);
    }
    for (u64 i = 0; i < request.external_search_bin_attrs; ++i) {
        LogBinAttr("  createExternalSearchBin", i, request.external_search_bin_attr[i]);
        AppendCreateRoomBinAttrById(req, request.external_search_bin_attr[i],
                                    "externalSearchBinAttr");
    }
    for (u64 i = 0; i < request.external_bin_attrs; ++i) {
        LogBinAttr("  createExternalBin", i, request.external_bin_attr[i]);
        AppendCreateRoomBinAttrById(req, request.external_bin_attr[i], "externalBinAttr");
    }
    for (u64 i = 0; i < request.member_internal_bin_attrs; ++i) {
        LogBinAttr("  createMemberInternalBin", i, request.member_internal_bin_attr[i]);
        AppendBinAttr(req.add_member_bin_attrs(), request.member_internal_bin_attr[i]);
    }
    req.set_join_group_label_present(request.join_group_label != nullptr);
    req.set_room_password_present(request.room_passwd != nullptr);
    if (request.signaling_param) {
        req.set_sig_type(request.signaling_param->type);
        req.set_sig_flag(request.signaling_param->flag);
        req.set_sig_main_member(request.signaling_param->member_id);
    }
    return MmSubmitRequest(ctx_id, req_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM,
                           MmCommand::CreateRoom, MakeProtoPayload(req));
}

s32 MmCreateJoinRoomA(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                      const OrbisNpMatching2CreateJoinRoomRequestA& request) {
    shadnet::CreateRoomRequest req;
    req.set_req_id(req_id);
    req.set_max_slots(request.max_slot);
    req.set_team_id(request.team_id);
    req.set_world_id(request.world_id);
    req.set_lobby_id(static_cast<u32>(request.lobby_id));
    req.set_flags(request.flags);
    req.set_group_config_count(static_cast<u32>(request.group_configs));
    req.set_allowed_user_count(static_cast<u32>(request.allowed_users));
    req.set_blocked_user_count(static_cast<u32>(request.blocked_users));
    req.set_internal_bin_attr_count(static_cast<u32>(request.internal_bin_attrs));
    LOG_INFO(Lib_NpMatching2,
             "ctx={} req_id={} max_slot={} world={} lobby={} flags={:#x} "
             "internal_bin={} search_int={} search_bin={} ext_bin={} member_bin={}",
             ctx_id, req_id, request.max_slot, request.world_id, request.lobby_id, request.flags,
             request.internal_bin_attrs, request.external_search_int_attrs,
             request.external_search_bin_attrs, request.external_bin_attrs,
             request.member_internal_bin_attrs);
    AppendCreateJoinRoomCommon(req, request.room_passwd, request.passwd_slot_mask,
                               request.group_config, request.group_configs,
                               request.join_group_label);
    req.set_user_id_kind(shadnet::MATCHING_USER_ID_ACCOUNT_ID);
    if (request.allowed_user) {
        for (u64 i = 0; i < request.allowed_users; ++i) {
            req.add_allowed_account_ids(request.allowed_user[i]);
        }
    }
    if (request.blocked_user) {
        for (u64 i = 0; i < request.blocked_users; ++i) {
            req.add_blocked_account_ids(request.blocked_user[i]);
        }
    }
    for (u64 i = 0; i < request.internal_bin_attrs; ++i) {
        LogBinAttr("  createInternalBinA", i, request.internal_bin_attr[i]);
        AppendCreateRoomBinAttrById(req, request.internal_bin_attr[i], "internalBinAttrA");
    }
    for (u64 i = 0; i < request.external_search_int_attrs; ++i) {
        LogIntAttr("  createExternalSearchIntA", i, request.external_search_int_attr[i]);
        AppendIntAttr(req.add_external_search_int_attrs(), request.external_search_int_attr[i]);
    }
    for (u64 i = 0; i < request.external_search_bin_attrs; ++i) {
        LogBinAttr("  createExternalSearchBinA", i, request.external_search_bin_attr[i]);
        AppendCreateRoomBinAttrById(req, request.external_search_bin_attr[i],
                                    "externalSearchBinAttrA");
    }
    for (u64 i = 0; i < request.external_bin_attrs; ++i) {
        LogBinAttr("  createExternalBinA", i, request.external_bin_attr[i]);
        AppendCreateRoomBinAttrById(req, request.external_bin_attr[i], "externalBinAttrA");
    }
    for (u64 i = 0; i < request.member_internal_bin_attrs; ++i) {
        LogBinAttr("  createMemberInternalBinA", i, request.member_internal_bin_attr[i]);
        AppendBinAttr(req.add_member_bin_attrs(), request.member_internal_bin_attr[i]);
    }
    req.set_join_group_label_present(request.join_group_label != nullptr);
    req.set_room_password_present(request.room_passwd != nullptr);
    if (request.signaling_param) {
        req.set_sig_type(request.signaling_param->type);
        req.set_sig_flag(request.signaling_param->flag);
        req.set_sig_main_member(request.signaling_param->member_id);
    }
    return MmSubmitRequest(ctx_id, req_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM_A,
                           MmCommand::CreateRoom, MakeProtoPayload(req), true);
}

s32 MmJoinRoom(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
               const OrbisNpMatching2JoinRoomRequest& request) {
    shadnet::JoinRoomRequest req;
    req.set_room_id(request.room_id);
    req.set_req_id(req_id);
    req.set_team_id(request.team_id);
    req.set_join_flags(request.flags);
    req.set_blocked_user_count(static_cast<u32>(request.blocked_users));
    req.set_user_id_kind(shadnet::MATCHING_USER_ID_ONLINE_ID);
    if (request.blocked_user) {
        for (u64 i = 0; i < request.blocked_users; ++i) {
            req.add_blocked_online_ids(OnlineIdToString(request.blocked_user[i]));
        }
    }
    for (u64 i = 0; i < request.room_member_bin_internal_attr_num; ++i) {
        AppendBinAttr(req.add_member_bin_attrs(), request.room_member_bin_internal_attr[i]);
    }
    req.set_room_password_present(request.room_passwd != nullptr);
    req.set_join_group_label_present(request.join_group_label != nullptr);
    return MmSubmitRequest(ctx_id, req_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM,
                           MmCommand::JoinRoom, MakeProtoPayload(req));
}

s32 MmJoinRoomA(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                const OrbisNpMatching2JoinRoomRequestA& request) {
    shadnet::JoinRoomRequest req;
    req.set_room_id(request.room_id);
    req.set_req_id(req_id);
    req.set_team_id(request.team_id);
    req.set_join_flags(request.flags);
    req.set_blocked_user_count(static_cast<u32>(request.blocked_users));
    req.set_user_id_kind(shadnet::MATCHING_USER_ID_ACCOUNT_ID);
    if (request.blocked_user) {
        for (u64 i = 0; i < request.blocked_users; ++i) {
            req.add_blocked_account_ids(request.blocked_user[i]);
        }
    }
    for (u64 i = 0; i < request.room_member_bin_internal_attr_num; ++i) {
        AppendBinAttr(req.add_member_bin_attrs(), request.room_member_bin_internal_attr[i]);
    }
    req.set_room_password_present(request.room_passwd != nullptr);
    req.set_join_group_label_present(request.join_group_label != nullptr);
    return MmSubmitRequest(ctx_id, req_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM_A,
                           MmCommand::JoinRoom, MakeProtoPayload(req), true);
}

s32 MmLeaveRoom(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                const OrbisNpMatching2LeaveRoomRequest& request) {
    shadnet::LeaveRoomRequest req;
    req.set_room_id(request.room_id);
    req.set_req_id(req_id);
    return MmSubmitRequest(ctx_id, req_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_LEAVE_ROOM,
                           MmCommand::LeaveRoom, MakeProtoPayload(req));
}

s32 MmGetWorldInfoList(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                       const OrbisNpMatching2GetWorldInfoListRequest& request) {
    shadnet::GetWorldInfoListRequest req;
    req.set_server_id(request.server_id);
    return MmSubmitRequest(ctx_id, req_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_WORLD_INFO_LIST,
                           MmCommand::GetWorldInfoList, MakeProtoPayload(req));
}

s32 MmSearchRoom(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                 const OrbisNpMatching2SearchRoomRequest& request, bool a_variant) {
    shadnet::SearchRoomRequest req;
    req.set_world_id(request.world_id);
    req.set_lobby_id(request.lobby_id);
    req.set_range_filter_start(request.range_filter.start);
    req.set_range_filter_max(request.range_filter.max);
    req.set_flag_filter(request.flags1);
    req.set_flag_attr(request.flags2);
    LOG_DEBUG(Lib_NpMatching2,
              "search world={} lobby={} range={}+{} flags={:#x}/{:#x} int_n={} bin_n={} attr_n={}",
              request.world_id, request.lobby_id, request.range_filter.start,
              request.range_filter.max, request.flags1, request.flags2, request.int_filters,
              request.bin_filters, request.attrs);
    for (u64 i = 0; i < request.int_filters; ++i) {
        const auto& f = request.int_filter[i];
        LOG_DEBUG(Lib_NpMatching2, "  int_filter[{}] op={} id={:#x} num={}", i, f.search_operator,
                  f.attr.id, f.attr.num);
        auto* dst = req.add_int_filters();
        dst->set_op(f.search_operator);
        dst->set_attr_id(f.attr.id);
        dst->set_attr_value(f.attr.num);
    }
    for (u64 i = 0; i < request.bin_filters; ++i) {
        const auto& f = request.bin_filter[i];
        auto* dst = req.add_bin_filters();
        dst->set_op(f.search_operator);
        dst->set_attr_id(f.attr.id);
        if (f.attr.data && f.attr.data_size > 0) {
            dst->set_data(f.attr.data, f.attr.data_size);
        }
    }
    for (u64 i = 0; i < request.attrs; ++i) {
        req.add_attr_ids(request.attr[i]);
    }
    return MmSubmitRequest(ctx_id, req_id,
                           a_variant ? ORBIS_NP_MATCHING2_REQUEST_EVENT_SEARCH_ROOM_A
                                     : ORBIS_NP_MATCHING2_REQUEST_EVENT_SEARCH_ROOM,
                           MmCommand::SearchRoom, MakeProtoPayload(req), a_variant);
}

s32 MmGetRoomDataExternalList(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                              const OrbisNpMatching2GetRoomDataExternalListRequest& request,
                              bool a_variant) {
    shadnet::GetRoomDataExternalListRequest req;
    for (u64 i = 0; i < request.room_id_num; ++i) {
        req.add_room_ids(request.room_id[i]);
    }
    for (u64 i = 0; i < request.attr_id_num; ++i) {
        req.add_attr_ids(request.attr_id[i]);
    }
    LOG_INFO(Lib_NpMatching2, "room_n={} attr_n={}", request.room_id_num, request.attr_id_num);
    for (u64 i = 0; i < request.room_id_num; ++i) {
        LOG_INFO(Lib_NpMatching2, "  room_id[{}]={}", i, request.room_id[i]);
    }
    for (u64 i = 0; i < request.attr_id_num; ++i) {
        LOG_INFO(Lib_NpMatching2, "  attr_id[{}]={:#x}", i, request.attr_id[i]);
    }
    return MmSubmitRequest(ctx_id, req_id,
                           a_variant
                               ? ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_DATA_EXTERNAL_LIST_A
                               : ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_DATA_EXTERNAL_LIST,
                           MmCommand::GetRoomDataExternalList, MakeProtoPayload(req), a_variant);
}

s32 MmGetRoomMemberDataExternalList(
    OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
    const OrbisNpMatching2GetRoomMemberDataExternalListRequest& request, bool a_variant) {
    shadnet::GetRoomMemberDataExternalListRequest req;
    req.set_room_id(request.room_id);
    LOG_DEBUG(Lib_NpMatching2, "room={}", request.room_id);
    return MmSubmitRequest(
        ctx_id, req_id,
        a_variant ? ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_MEMBER_DATA_EXTERNAL_LIST_A
                  : ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_MEMBER_DATA_EXTERNAL_LIST,
        MmCommand::GetRoomMemberDataExternalList, MakeProtoPayload(req), a_variant);
}

s32 MmGetUserInfoList(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                      const OrbisNpMatching2GetUserInfoListRequest& request, bool a_variant) {
    shadnet::GetUserInfoListRequest req;
    for (u64 i = 0; i < request.np_id_num; ++i) {
        req.add_npids(request.np_id[i].handle.data);
    }
    for (u64 i = 0; i < request.attr_id_num; ++i) {
        req.add_attr_ids(request.attr_id[i]);
    }
    req.set_option(request.option);
    LOG_DEBUG(Lib_NpMatching2, "np_n={} attr_n={}", request.np_id_num, request.attr_id_num);
    return MmSubmitRequest(ctx_id, req_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_USER_INFO_LIST,
                           MmCommand::GetUserInfoList, MakeProtoPayload(req), a_variant);
}

s32 MmSetUserInfo(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                  const OrbisNpMatching2SetUserInfoRequest& request) {
    shadnet::SetUserInfoRequest req;
    req.set_server_id(request.server_id);
    for (u64 i = 0; request.user_bin_attr && i < request.user_bin_attrs; ++i) {
        AppendBinAttr(req.add_user_bin_attrs(), request.user_bin_attr[i]);
    }
    LOG_DEBUG(Lib_NpMatching2, "server={} bin_attrs={}", request.server_id, request.user_bin_attrs);
    return MmSubmitRequest(ctx_id, req_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_USER_INFO,
                           MmCommand::SetUserInfo, MakeProtoPayload(req));
}

s32 MmSendRoomMessage(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                      const OrbisNpMatching2SendRoomMessageRequest& request) {
    shadnet::SendRoomMessageRequest req;
    req.set_room_id(request.room_id);
    req.set_cast_type(request.cast_type);
    req.set_option(request.option);

    if (request.cast_type == ORBIS_NP_MATCHING2_CASTTYPE_UNICAST) {
        req.add_dst_member_ids(request.dst.unicast_target);
    } else if (request.cast_type == ORBIS_NP_MATCHING2_CASTTYPE_MULTICAST &&
               request.dst.multicast_target.member_id) {
        for (u64 i = 0; i < request.dst.multicast_target.member_id_num; ++i) {
            req.add_dst_member_ids(request.dst.multicast_target.member_id[i]);
        }
    }

    if (request.msg && request.msg_len > 0) {
        req.set_msg(request.msg, request.msg_len);
    }

    LOG_DEBUG(Lib_NpMatching2, "room={} cast={} dst_n={} bytes={}", request.room_id,
              request.cast_type, req.dst_member_ids_size(), request.msg_len);
    return MmSubmitRequest(ctx_id, req_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_SEND_ROOM_MESSAGE,
                           MmCommand::SendRoomMessage, MakeProtoPayload(req));
}

s32 MmSetRoomDataInternal(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                          const OrbisNpMatching2SetRoomDataInternalRequest& request) {
    shadnet::SetRoomDataInternalRequest req;
    req.set_req_id(req_id);
    req.set_room_id(request.room_id);
    req.set_flag_filter(request.flag_filter);
    req.set_flag_attr(request.flag_attr);
    LOG_INFO(Lib_NpMatching2,
             "ctx={} req_id={} room={} flags={:#x}/{:#x} bin_attrs={} "
             "passwd_mask={}",
             ctx_id, req_id, request.room_id, request.flag_filter, request.flag_attr,
             request.room_bin_attr_internal_num, fmt::ptr(request.password_slot_mask));
    for (u64 i = 0; i < request.room_bin_attr_internal_num; ++i) {
        LogBinAttr("  internalBin", i, request.room_bin_attr_internal[i]);
        AppendBinAttr(req.add_bin_attrs(), request.room_bin_attr_internal[i]);
    }
    if (request.password_slot_mask) {
        req.set_has_passwd_mask(true);
        req.set_passwd_slot_mask(*request.password_slot_mask);
    }
    return MmSubmitRequest(ctx_id, req_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_ROOM_DATA_INTERNAL,
                           MmCommand::SetRoomDataInternal, MakeProtoPayload(req));
}

s32 MmSetRoomMemberDataInternal(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                                const OrbisNpMatching2SetRoomMemberDataInternalRequest& request) {
    shadnet::SetRoomMemberDataInternalRequest req;
    req.set_req_id(req_id);
    req.set_room_id(request.room_id);
    req.set_member_id(request.member_id);
    req.set_team_id(request.team_id);
    req.set_flag_filter(request.flag_filter);
    req.set_flag_attr(request.flag_attr);
    LOG_INFO(Lib_NpMatching2,
             "ctx={} req_id={} room={} member={} team={} "
             "flags={:#x}/{:#x} bin_attrs={}",
             ctx_id, req_id, request.room_id, request.member_id, request.team_id,
             request.flag_filter, request.flag_attr, request.room_member_bin_attr_internal_num);
    for (u64 i = 0; i < request.room_member_bin_attr_internal_num; ++i) {
        LogBinAttr("  memberInternalBin", i, request.room_member_bin_attr_internal[i]);
        AppendBinAttr(req.add_bin_attrs(), request.room_member_bin_attr_internal[i]);
    }
    return MmSubmitRequest(ctx_id, req_id,
                           ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_ROOM_MEMBER_DATA_INTERNAL,
                           MmCommand::SetRoomMemberDataInternal, MakeProtoPayload(req));
}

s32 MmSetRoomDataExternal(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                          const OrbisNpMatching2SetRoomDataExternalRequest& request) {
    shadnet::SetRoomDataExternalRequest req;
    req.set_req_id(req_id);
    req.set_room_id(request.room_id);
    LOG_INFO(Lib_NpMatching2,
             "ctx={} req_id={} room={} search_int={} search_bin={} "
             "ext_bin={}",
             ctx_id, req_id, request.room_id, request.room_searchable_int_attr_external_num,
             request.room_searchable_bin_attr_external_num, request.room_bin_attr_external_num);
    for (u64 i = 0; i < request.room_searchable_int_attr_external_num; ++i) {
        LogIntAttr("  externalSearchInt", i, request.room_searchable_int_attr_external[i]);
        AppendIntAttr(req.add_search_int_attrs(), request.room_searchable_int_attr_external[i]);
    }
    for (u64 i = 0; i < request.room_searchable_bin_attr_external_num; ++i) {
        LogBinAttr("  externalSearchBin", i, request.room_searchable_bin_attr_external[i]);
        AppendBinAttr(req.add_search_bin_attrs(), request.room_searchable_bin_attr_external[i]);
    }
    for (u64 i = 0; i < request.room_bin_attr_external_num; ++i) {
        LogBinAttr("  externalBin", i, request.room_bin_attr_external[i]);
        AppendBinAttr(req.add_ext_bin_attrs(), request.room_bin_attr_external[i]);
    }
    return MmSubmitRequest(ctx_id, req_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_ROOM_DATA_EXTERNAL,
                           MmCommand::SetRoomDataExternal, MakeProtoPayload(req));
}

s32 MmKickoutRoomMember(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                        const OrbisNpMatching2KickoutRoomMemberRequest& request) {
    shadnet::KickoutRoomMemberRequest req;
    req.set_req_id(req_id);
    req.set_room_id(request.room_id);
    req.set_target_member_id(request.member_id);
    req.set_block_kick_flag(request.block_kick_flag);
    return MmSubmitRequest(ctx_id, req_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_KICKOUT_ROOM_MEMBER,
                           MmCommand::KickoutRoomMember, MakeProtoPayload(req));
}

u32 GetMmServerAddr() {
    std::lock_guard lock(g_mm.mutex);
    return g_mm.server_addr;
}

u16 GetMmServerUdpPort() {
    std::lock_guard lock(g_mm.mutex);
    return g_mm.server_udp_port;
}

bool RequestSignalingInfos(std::string_view target_online_id, u32* out_addr, u16* out_port) {
    if (!out_addr || !out_port) {
        return false;
    }
    if (IsMatching2BackendDisabled()) {
        return false;
    }
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(g_mm.mutex);
        client = g_mm.client;
    }
    if (!client || !client->IsAuthenticated()) {
        return false;
    }
    if (ShadNet::ShadNetClient::OnReaderThread()) {
        LOG_DEBUG(Lib_NpMatching2, "'{}': deferred to the handshake thread", target_online_id);
        return false;
    }

    shadnet::RequestSignalingInfosRequest req;
    req.set_target_npid(std::string(target_online_id));
    const u64 pkt_id =
        client->SubmitRequest(ShadNet::CommandType::RequestSignalingInfos, MakeProtoPayload(req));

    std::pair<ShadNet::ErrorType, std::vector<u8>> reply;
    {
        std::unique_lock lock(g_mm.sig_mutex);
        if (!g_mm.sig_cv.wait_for(lock, std::chrono::seconds(5),
                                  [&] { return g_mm.sig_replies.count(pkt_id) > 0; })) {
            LOG_WARNING(Lib_NpMatching2, "timed out for '{}'", target_online_id);
            return false;
        }
        reply = std::move(g_mm.sig_replies[pkt_id]);
        g_mm.sig_replies.erase(pkt_id);
    }
    if (reply.first != ShadNet::ErrorType::NoError) {
        return false;
    }

    shadnet::RequestSignalingInfosReply rep;
    const std::string proto = ExtractProtoBytes(reply.second);
    if (proto.empty() || !rep.ParseFromString(proto)) {
        return false;
    }
    const std::string addr_str = rep.target_ip();
    const u16 port_host = static_cast<u16>(rep.target_port());
    if (addr_str.empty() || port_host == 0) {
        return false;
    }

    u32 addr_nbo = 0;
    if (sceNetInetPton(ORBIS_NET_AF_INET, addr_str.c_str(), &addr_nbo) <= 0) {
        return false;
    }
    *out_addr = addr_nbo;
    *out_port = htons(port_host);
    LOG_INFO(Lib_NpMatching2, "received peer='{}' ip={} port={} addr_nbo={:#x} port_nbo={:#x}",
             target_online_id, addr_str, port_host, addr_nbo, *out_port);
    return true;
}

} // namespace Libraries::Np::NpMatching2
