// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <magic_enum/magic_enum.hpp>

#include "common/logging/log.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "shadnet.pb.h"
#include "shadnet/client.h"

namespace Libraries::Np {

void NpHandler::ResolveOnlineId(
    s32 user_id, const std::string& online_id,
    std::function<void(s32 result, u64 account_id, const std::string& canonical_online_id)>
        on_result) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "No shadNet session for user_id={}", user_id);
            if (on_result) {
                on_result(ORBIS_NP_ERROR_SIGNED_OUT, 0, {});
            }
            return;
        }
        client = it->second;
    }
    if (!client || !client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
        if (on_result) {
            on_result(ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN, 0, {});
        }
        return;
    }
    if (online_id.empty()) {
        if (on_result) {
            on_result(ORBIS_NP_COMMUNITY_SERVER_ERROR_NO_SUCH_USER_NPID, 0, {});
        }
        return;
    }

    const u64 pkt_id = client->LookupOnlineId(online_id);
    if (on_result) {
        std::lock_guard lock(m_mutex_pending_lookup);
        PendingLookupRequest pending;
        pending.user_id = user_id;
        pending.on_result = std::move(on_result);
        m_pending_lookup.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler, "user_id={} online_id='{}' pkt_id={}", user_id, online_id, pkt_id);
}

void NpHandler::OnLookupReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id,
                              ShadNet::ErrorType error, const std::vector<u8>& body) {
    std::function<void(s32, u64, const std::string&)> on_result;
    {
        std::lock_guard lock(m_mutex_pending_lookup);
        auto it = m_pending_lookup.find(pkt_id);
        if (it == m_pending_lookup.end()) {
            return;
        }
        on_result = std::move(it->second.on_result);
        m_pending_lookup.erase(it);
    }
    if (!on_result) {
        return;
    }

    if (error == ShadNet::ErrorType::NotFound) {
        LOG_INFO(NpHandler, "user_id={} pkt_id={} not found", user_id, pkt_id);
        on_result(ORBIS_NP_COMMUNITY_SERVER_ERROR_NO_SUCH_USER_NPID, 0, {});
        return;
    }
    if (error == ShadNet::ErrorType::InvalidInput) {
        on_result(ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT, 0, {});
        return;
    }
    if (error != ShadNet::ErrorType::NoError) {
        LOG_WARNING(NpHandler, "failed, error={}", magic_enum::enum_name(error));
        on_result(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE, 0, {});
        return;
    }

    if (body.size() < 4) {
        LOG_WARNING(NpHandler, "truncated reply");
        on_result(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE, 0, {});
        return;
    }
    const u32 blob_sz = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                        (static_cast<u32>(body[2]) << 16) | (static_cast<u32>(body[3]) << 24);
    if (body.size() < 4 + blob_sz) {
        LOG_WARNING(NpHandler, "reply shorter than declared blob");
        on_result(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE, 0, {});
        return;
    }

    shadnet::LookupOnlineIdReply pb;
    if (!pb.ParseFromArray(body.data() + 4, static_cast<s32>(blob_sz))) {
        LOG_WARNING(NpHandler, "could not parse reply");
        on_result(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE, 0, {});
        return;
    }

    LOG_INFO(NpHandler, "user_id={} pkt_id={} -> account_id={} npid='{}'", user_id, pkt_id,
             pb.account_id(), pb.npid());
    on_result(ORBIS_OK, pb.account_id(), pb.npid());
}

} // namespace Libraries::Np