// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <magic_enum/magic_enum.hpp>

#include "common/logging/log.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "shadnet.pb.h"
#include "shadnet/client.h"

namespace Libraries::Np {

using namespace NpMatching2;

namespace {

// Converts shadNet errors to Matching2 server errors
s32 Matching2Error(ShadNet::ErrorType error) {
    switch (error) {
    case ShadNet::ErrorType::NoError:
        return ORBIS_OK;
    case ShadNet::ErrorType::Malformed:
    case ShadNet::ErrorType::Invalid:
    case ShadNet::ErrorType::InvalidInput:
        return ORBIS_NP_MATCHING2_SERVER_ERROR_BAD_REQUEST;
    case ShadNet::ErrorType::TooSoon:
        return ORBIS_NP_MATCHING2_SERVER_ERROR_BUSY;
    case ShadNet::ErrorType::RoomMissing:
        return ORBIS_NP_MATCHING2_SERVER_ERROR_NO_SUCH_ROOM;
    case ShadNet::ErrorType::RoomAlreadyJoined:
        return ORBIS_NP_MATCHING2_SERVER_ERROR_ALREADY_JOINED;
    case ShadNet::ErrorType::RoomFull:
        return ORBIS_NP_MATCHING2_SERVER_ERROR_ROOM_FULL;
    case ShadNet::ErrorType::RoomPasswordMismatch:
        return ORBIS_NP_MATCHING2_SERVER_ERROR_PASSWORD_MISMATCH;
    case ShadNet::ErrorType::RoomPasswordMissing:
        return ORBIS_NP_MATCHING2_SERVER_ERROR_NO_PASSWORD;
    case ShadNet::ErrorType::RoomGroupNoJoinLabel:
    case ShadNet::ErrorType::RoomGroupJoinLabelNotFound:
        return ORBIS_NP_MATCHING2_SERVER_ERROR_NO_SUCH_GROUP;
    case ShadNet::ErrorType::RoomGroupFull:
        return ORBIS_NP_MATCHING2_SERVER_ERROR_GROUP_FULL;
    case ShadNet::ErrorType::RoomGroupMaxSlotMismatch:
        return ORBIS_NP_MATCHING2_SERVER_ERROR_INVALID_GROUP_SLOT_NUM;
    case ShadNet::ErrorType::Unauthorized:
        return ORBIS_NP_MATCHING2_SERVER_ERROR_FORBIDDEN;
    case ShadNet::ErrorType::NotFound:
        return ORBIS_NP_MATCHING2_SERVER_ERROR_NO_SUCH_USER;
    case ShadNet::ErrorType::Blocked:
        return ORBIS_NP_MATCHING2_SERVER_ERROR_BLOCKED;
    default:
        return ORBIS_NP_MATCHING2_SERVER_ERROR_INTERNAL_SERVER_ERROR;
    }
}

template <typename T>
bool ParseReply(const std::vector<u8>& body, T& reply) {
    if (body.size() < sizeof(u32)) {
        return false;
    }
    const u32 size = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                     (static_cast<u32>(body[2]) << 16) | (static_cast<u32>(body[3]) << 24);
    if (body.size() < sizeof(u32) + size) {
        return false;
    }
    return reply.ParseFromArray(body.data() + sizeof(u32), static_cast<s32>(size));
}

} // namespace

s32 NpHandler::SubmitMatching2Request(s32 user_id,
                                      const std::function<u64(ShadNet::ShadNetClient&)>& submit,
                                      Matching2ReplyHandler on_reply) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        if (const auto it = m_clients.find(user_id); it != m_clients.end()) {
            client = it->second;
        }
    }
    if (!client || !client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "no shadnet session for user {}", user_id);
        return ORBIS_NP_MATCHING2_SERVER_ERROR_SERVICE_UNAVAILABLE;
    }

    std::lock_guard lock(m_mutex_pending_matching2);
    const u64 pkt_id = submit(*client);
    m_pending_matching2.emplace(pkt_id, std::move(on_reply));
    return ORBIS_OK;
}

void NpHandler::OnMatching2Reply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id,
                                 ShadNet::ErrorType error, const std::vector<u8>& body) {
    Matching2ReplyHandler on_reply;
    {
        std::lock_guard lock(m_mutex_pending_matching2);
        const auto it = m_pending_matching2.find(pkt_id);
        if (it == m_pending_matching2.end()) {
            LOG_WARNING(NpHandler, "matching2 reply {} for {} has no request",
                        magic_enum::enum_name(cmd), pkt_id);
            return;
        }
        on_reply = std::move(it->second);
        m_pending_matching2.erase(it);
    }
    if (error != ShadNet::ErrorType::NoError) {
        LOG_WARNING(NpHandler, "matching2 {} for user {} failed, error {}",
                    magic_enum::enum_name(cmd), user_id, magic_enum::enum_name(error));
    }
    on_reply(Matching2Error(error), body);
}

void NpHandler::GetWorldInfoList(
    s32 user_id, const OrbisNpMatching2GetWorldInfoListRequest& request,
    std::function<void(s32, std::span<const OrbisNpMatching2World>)> on_result) {
    const auto on_reply = [on_result](s32 result, const std::vector<u8>& body) {
        if (result < 0) {
            on_result(result, {});
            return;
        }
        shadnet::GetWorldInfoListReply reply;
        if (!ParseReply(body, reply)) {
            LOG_WARNING(NpHandler, "could not parse the world info list");
            on_result(ORBIS_NP_MATCHING2_SERVER_ERROR_INTERNAL_SERVER_ERROR, {});
            return;
        }
        std::vector<OrbisNpMatching2World> worlds(reply.worlds_size());
        for (s32 i = 0; i < reply.worlds_size(); i++) {
            const auto& world = reply.worlds(i);
            worlds[i].world_id = world.world_id();
            worlds[i].lobbies_num = world.lobbies_num();
            worlds[i].max_lobby_members_num = world.max_lobby_members();
            worlds[i].lobby_members_num = world.lobby_members_num();
            worlds[i].rooms_num = world.rooms_num();
            worlds[i].room_members_num = world.room_members_num();
        }
        on_result(ORBIS_OK, worlds);
    };
    const s32 result = SubmitMatching2Request(
        user_id, [&](ShadNet::ShadNetClient& client) { return client.GetWorldInfoList(request); },
        on_reply);
    if (result < 0) {
        on_result(result, {});
    }
}

void NpHandler::CreateJoinRoom(
    s32 user_id, OrbisNpMatching2RequestId req_id, const CreateJoinRoomParams& params,
    std::function<void(s32, const shadnet::CreateRoomReply&)> on_result) {
    const auto on_reply = [on_result](s32 result, const std::vector<u8>& body) {
        shadnet::CreateRoomReply reply;
        if (result >= 0 && !ParseReply(body, reply)) {
            LOG_WARNING(NpHandler, "could not parse the created room");
            result = ORBIS_NP_MATCHING2_SERVER_ERROR_INTERNAL_SERVER_ERROR;
        }
        on_result(result, reply);
    };
    const s32 result = SubmitMatching2Request(
        user_id,
        [&](ShadNet::ShadNetClient& client) {
            return params.UsesAccountIds() ? client.CreateJoinRoomA(req_id, params.GetRequestA())
                                           : client.CreateJoinRoom(req_id, params.GetRequest());
        },
        on_reply);
    if (result < 0) {
        on_result(result, shadnet::CreateRoomReply{});
    }
}

void NpHandler::SetRoomDataExternal(s32 user_id, OrbisNpMatching2RequestId req_id,
                                    const OrbisNpMatching2SetRoomDataExternalRequest& request,
                                    std::function<void(s32)> on_result) {
    const s32 result = SubmitMatching2Request(
        user_id,
        [&](ShadNet::ShadNetClient& client) { return client.SetRoomDataExternal(req_id, request); },
        [on_result](s32 result, const std::vector<u8>&) { on_result(result); });
    if (result < 0) {
        on_result(result);
    }
}

void NpHandler::LeaveRoom(s32 user_id, OrbisNpMatching2RequestId req_id,
                          const OrbisNpMatching2LeaveRoomRequest& request,
                          std::function<void(s32)> on_result) {
    const s32 result = SubmitMatching2Request(
        user_id, [&](ShadNet::ShadNetClient& client) { return client.LeaveRoom(req_id, request); },
        [on_result](s32 result, const std::vector<u8>&) { on_result(result); });
    if (result < 0) {
        on_result(result);
    }
}

void NpHandler::SetRoomDataInternal(s32 user_id, OrbisNpMatching2RequestId req_id,
                                    const OrbisNpMatching2SetRoomDataInternalRequest& request,
                                    std::function<void(s32)> on_result) {
    const s32 result = SubmitMatching2Request(
        user_id,
        [&](ShadNet::ShadNetClient& client) { return client.SetRoomDataInternal(req_id, request); },
        [on_result](s32 result, const std::vector<u8>&) { on_result(result); });
    if (result < 0) {
        on_result(result);
    }
}

void NpHandler::SearchRoom(
    s32 user_id, const OrbisNpMatching2SearchRoomRequest& request,
    std::function<void(s32 result, const shadnet::SearchRoomReply& reply)> on_result) {
    const auto on_reply = [on_result](s32 result, const std::vector<u8>& body) {
        shadnet::SearchRoomReply reply;
        if (result >= 0 && !ParseReply(body, reply)) {
            LOG_WARNING(NpHandler, "could not parse the room search results");
            result = ORBIS_NP_MATCHING2_SERVER_ERROR_INTERNAL_SERVER_ERROR;
        }
        on_result(result, reply);
    };
    const s32 result = SubmitMatching2Request(
        user_id, [&](ShadNet::ShadNetClient& client) { return client.SearchRoom(request); },
        on_reply);
    if (result < 0) {
        on_result(result, shadnet::SearchRoomReply{});
    }
}

void NpHandler::GetRoomDataExternalList(
    s32 user_id, const OrbisNpMatching2GetRoomDataExternalListRequest& request,
    std::function<void(s32 result, const shadnet::GetRoomDataExternalListReply& reply)> on_result) {
    const auto on_reply = [on_result](s32 result, const std::vector<u8>& body) {
        shadnet::GetRoomDataExternalListReply reply;
        if (result >= 0 && !ParseReply(body, reply)) {
            LOG_WARNING(NpHandler, "could not parse the room data external list");
            result = ORBIS_NP_MATCHING2_SERVER_ERROR_INTERNAL_SERVER_ERROR;
        }
        on_result(result, reply);
    };
    const s32 result = SubmitMatching2Request(
        user_id,
        [&](ShadNet::ShadNetClient& client) { return client.GetRoomDataExternalList(request); },
        on_reply);
    if (result < 0) {
        on_result(result, shadnet::GetRoomDataExternalListReply{});
    }
}

void NpHandler::StartMatching2Context(s32 user_id) {
    // The real library manages its own contexts, shadNet only needs to know one started.
    constexpr OrbisNpMatching2ContextId ContextId = 1;
    SubmitMatching2Request(
        user_id, [](ShadNet::ShadNetClient& client) { return client.ContextStart(ContextId); },
        [user_id](s32 result, const std::vector<u8>&) {
            if (result < 0) {
                LOG_WARNING(NpHandler, "matching2 context start for user {} failed, {:#x}", user_id,
                            static_cast<u32>(result));
            }
        });
}

} // namespace Libraries::Np
