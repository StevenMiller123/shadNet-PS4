// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>

#include "common/logging/log.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "shadnet.pb.h"

namespace Libraries::Np {

s32 NpHandler::RecordScore(s32 user_id, s32 service_label, u32 board_id, s32 pc_id, s64 score,
                           const char* comment, u64 comment_len, const u8* game_info_data,
                           u64 game_info_size, std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    // Look up the user's shadNet session.
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::RecordScoreRequest proto;
    proto.set_boardid(board_id);
    proto.set_pcid(pc_id);
    proto.set_score(score);
    if (comment && comment_len > 0) {
        proto.set_comment(std::string(comment, comment_len));
    }
    if (game_info_data && game_info_size > 0) {
        proto.set_data(std::string(reinterpret_cast<const char*>(game_info_data), game_info_size));
    }

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "missing NP communication ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::RecordScore, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::RecordScore;
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} pc_id={} score={} comment_len={} "
             "game_info_size={} pkt_id={} com_id='{}'",
             user_id, service_label, board_id, pc_id, score, comment_len, game_info_size, pkt_id,
             com_id);
    return ORBIS_OK;
}

s32 NpHandler::RecordGameData(s32 user_id, s32 service_label, u32 board_id, s32 pc_id, s64 score,
                              const u8* data, u64 size,
                              std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::RecordScoreGameDataRequest proto;
    proto.set_boardid(board_id);
    proto.set_pcid(pc_id);
    proto.set_score(score);

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "missing NP communication ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size() + size);
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());
    if (data != nullptr && size > 0) {
        payload.insert(payload.end(), data, data + size);
    }

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::RecordScoreData, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::RecordScoreData;
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} pc_id={} score={} data_size={} "
             "pkt_id={} com_id='{}'",
             user_id, service_label, board_id, pc_id, score, size, pkt_id, com_id);
    return ORBIS_OK;
}

s32 NpHandler::GetGameData(s32 user_id, s32 service_label, u32 board_id, const std::string& np_id,
                           s32 pc_id, void* data_out, u64 recv_size, u64* total_size_out,
                           std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::GetScoreGameDataRequest proto;
    proto.set_boardid(board_id);
    proto.set_npid(np_id);
    proto.set_pcid(pc_id);

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "missing NP communication ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::GetScoreData, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::GetScoreData;
        pending.data_out = data_out;
        pending.recv_size = recv_size;
        pending.total_size_out = total_size_out;
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} np_id='{}' pc_id={} recv_size={} "
             "pkt_id={} com_id='{}'",
             user_id, service_label, board_id, np_id, pc_id, recv_size, pkt_id, com_id);
    return ORBIS_OK;
}

s32 NpHandler::GetGameDataByAccountId(s32 user_id, s32 service_label, u32 board_id, u64 account_id,
                                      s32 pc_id, void* data_out, u64 recv_size, u64* total_size_out,
                                      std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::GetScoreGameDataByAccountIdRequest proto;
    proto.set_boardid(board_id);
    proto.set_accountid(account_id);
    proto.set_pcid(pc_id);

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "missing NP communication ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());

    const u64 pkt_id =
        client->SubmitRequest(ShadNet::CommandType::GetScoreGameDataByAccId, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::GetScoreGameDataByAccId;
        pending.data_out = data_out;
        pending.recv_size = recv_size;
        pending.total_size_out = total_size_out;
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} account_id={} pc_id={} "
             "recv_size={} pkt_id={} com_id='{}'",
             user_id, service_label, board_id, account_id, pc_id, recv_size, pkt_id, com_id);
    return ORBIS_OK;
}

s32 NpHandler::GetBoardInfo(s32 user_id, s32 service_label, u32 board_id,
                            NpScore::OrbisNpScoreBoardInfo* board_info,
                            std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "missing NP communication ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    std::vector<u8> payload;
    payload.reserve(12 + 4);
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    payload.push_back(static_cast<u8>(board_id));
    payload.push_back(static_cast<u8>(board_id >> 8));
    payload.push_back(static_cast<u8>(board_id >> 16));
    payload.push_back(static_cast<u8>(board_id >> 24));

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::GetBoardInfos, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::GetBoardInfos;
        pending.board_info = board_info;
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler, "user_id={} service_label={} board={} pkt_id={} com_id='{}'", user_id,
             service_label, board_id, pkt_id, com_id);
    return ORBIS_OK;
}

s32 NpHandler::GetRankingByNpId(s32 user_id, s32 service_label, u32 board_id,
                                const std::vector<std::string>& np_ids,
                                const std::vector<s32>& pc_ids,
                                NpScore::OrbisNpScorePlayerRankData* rank_array,
                                NpScore::OrbisNpScoreComment* comment_array,
                                NpScore::OrbisNpScoreGameInfo* info_array,
                                Rtc::OrbisRtcTick* last_sort_date, u32* total_record,
                                std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    // pcIds must either be empty (use 0 for everything) or match npIds in size.
    if (!pc_ids.empty() && pc_ids.size() != np_ids.size()) {
        LOG_ERROR(NpHandler, "invalid pcIds size {}", pc_ids.size());
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    // Look up the user's session.
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::GetScoreNpIdRequest proto;
    proto.set_boardid(board_id);
    proto.set_withcomment(comment_array != nullptr);
    proto.set_withgameinfo(info_array != nullptr);
    for (u64 i = 0; i < np_ids.size(); ++i) {
        auto* entry = proto.add_npids();
        entry->set_npid(np_ids[i]);
        entry->set_pcid(pc_ids.empty() ? 0 : pc_ids[i]);
    }

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "missing NP communication ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::GetScoreNpId, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::GetScoreNpId;
        pending.requested_np_ids = np_ids;
        pending.rank_array = rank_array;
        pending.comment_array = comment_array;
        pending.info_array = info_array;
        pending.last_sort_date = last_sort_date;
        pending.total_record = total_record;
        pending.array_num = np_ids.size();
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} np_id_count={} "
             "with_pc_id={} with_comment={} with_game_info={} pkt_id={} com_id='{}'",
             user_id, service_label, board_id, np_ids.size(), !pc_ids.empty(),
             comment_array != nullptr, info_array != nullptr, pkt_id, com_id);
    return ORBIS_OK;
}

s32 NpHandler::GetRankingByRange(s32 user_id, s32 service_label, u32 board_id,
                                 u32 start_serial_rank, u32 array_num,
                                 NpScore::OrbisNpScoreRankData* rank_array,
                                 NpScore::OrbisNpScoreComment* comment_array,
                                 NpScore::OrbisNpScoreGameInfo* info_array,
                                 Rtc::OrbisRtcTick* last_sort_date, u32* total_record,
                                 std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::GetScoreRangeRequest proto;
    proto.set_boardid(board_id);
    proto.set_startrank(start_serial_rank);
    proto.set_numranks(array_num);
    proto.set_withcomment(comment_array != nullptr);
    proto.set_withgameinfo(info_array != nullptr);

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "missing NP communication ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::GetScoreRange, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::GetScoreRange;
        pending.plain_rank_array = rank_array;
        pending.comment_array = comment_array;
        pending.info_array = info_array;
        pending.last_sort_date = last_sort_date;
        pending.total_record = total_record;
        pending.array_num = array_num;
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} start_rank={} num_ranks={} "
             "with_comment={} with_game_info={} pkt_id={} com_id='{}'",
             user_id, service_label, board_id, start_serial_rank, array_num,
             comment_array != nullptr, info_array != nullptr, pkt_id, com_id);
    return ORBIS_OK;
}

s32 NpHandler::GetRankingByRangeA(s32 user_id, s32 service_label, u32 board_id,
                                  u32 start_serial_rank, u32 array_num,
                                  NpScore::OrbisNpScoreRankDataA* rank_array,
                                  NpScore::OrbisNpScoreComment* comment_array,
                                  NpScore::OrbisNpScoreGameInfo* info_array,
                                  Rtc::OrbisRtcTick* last_sort_date, u32* total_record,
                                  std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::GetScoreRangeRequest proto;
    proto.set_boardid(board_id);
    proto.set_startrank(start_serial_rank);
    proto.set_numranks(array_num);
    proto.set_withcomment(comment_array != nullptr);
    proto.set_withgameinfo(info_array != nullptr);

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "missing NP communication ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::GetScoreRange, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::GetScoreRange;
        pending.a_rank_array = rank_array;
        pending.comment_array = comment_array;
        pending.info_array = info_array;
        pending.last_sort_date = last_sort_date;
        pending.total_record = total_record;
        pending.array_num = array_num;
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} start_rank={} num_ranks={} "
             "with_comment={} with_game_info={} pkt_id={} com_id='{}'",
             user_id, service_label, board_id, start_serial_rank, array_num,
             comment_array != nullptr, info_array != nullptr, pkt_id, com_id);
    return ORBIS_OK;
}

s32 NpHandler::GetRankingByAccountId(s32 user_id, s32 service_label, u32 board_id,
                                     const std::vector<u64>& account_ids,
                                     const std::vector<s32>& pc_ids,
                                     NpScore::OrbisNpScorePlayerRankDataA* rank_array,
                                     NpScore::OrbisNpScoreComment* comment_array,
                                     NpScore::OrbisNpScoreGameInfo* info_array,
                                     Rtc::OrbisRtcTick* last_sort_date, u32* total_record,
                                     std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::GetScoreAccountIdRequest proto;
    proto.set_boardid(board_id);
    for (u64 i = 0; i < account_ids.size(); ++i) {
        auto* entry = proto.add_ids();
        entry->set_accountid(static_cast<s64>(account_ids[i]));
        entry->set_pcid(i < pc_ids.size() ? pc_ids[i] : 0);
    }
    proto.set_withcomment(comment_array != nullptr);
    proto.set_withgameinfo(info_array != nullptr);

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "missing NP communication ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::GetScoreAccountId, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::GetScoreAccountId;
        pending.a_player_rank_array = rank_array;
        pending.comment_array = comment_array;
        pending.info_array = info_array;
        pending.last_sort_date = last_sort_date;
        pending.total_record = total_record;
        pending.array_num = account_ids.size();
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} account_id_count={} "
             "with_comment={} with_game_info={} pkt_id={} com_id='{}'",
             user_id, service_label, board_id, account_ids.size(), comment_array != nullptr,
             info_array != nullptr, pkt_id, com_id);
    return ORBIS_OK;
}

s32 NpHandler::GetFriendsRanking(s32 user_id, s32 service_label, u32 board_id, bool include_self,
                                 u32 array_num, NpScore::OrbisNpScoreRankData* rank_array,
                                 NpScore::OrbisNpScoreComment* comment_array,
                                 NpScore::OrbisNpScoreGameInfo* info_array,
                                 Rtc::OrbisRtcTick* last_sort_date, u32* total_record,
                                 std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::GetScoreFriendsRequest proto;
    proto.set_boardid(board_id);
    proto.set_includeself(include_self);
    proto.set_max(array_num);
    proto.set_withcomment(comment_array != nullptr);
    proto.set_withgameinfo(info_array != nullptr);

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "missing NP communication ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::GetScoreFriends, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::GetScoreFriends;
        pending.plain_rank_array = rank_array;
        pending.comment_array = comment_array;
        pending.info_array = info_array;
        pending.last_sort_date = last_sort_date;
        pending.total_record = total_record;
        pending.array_num = array_num;
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} include_self={} "
             "array_num={} with_comment={} with_game_info={} pkt_id={} com_id='{}'",
             user_id, service_label, board_id, include_self, array_num, comment_array != nullptr,
             info_array != nullptr, pkt_id, com_id);
    return ORBIS_OK;
}

s32 NpHandler::GetFriendsRankingA(s32 user_id, s32 service_label, u32 board_id, bool include_self,
                                  u32 array_num, NpScore::OrbisNpScoreRankDataA* rank_array,
                                  NpScore::OrbisNpScoreComment* comment_array,
                                  NpScore::OrbisNpScoreGameInfo* info_array,
                                  Rtc::OrbisRtcTick* last_sort_date, u32* total_record,
                                  std::shared_ptr<NpScore::ScoreRequestCtx> req) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            LOG_WARNING(NpHandler, "user_id={} not connected", user_id);
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    if (!client->IsAuthenticated()) {
        LOG_WARNING(NpHandler, "user_id={} not authenticated", user_id);
        return ORBIS_NP_COMMUNITY_ERROR_NO_LOGIN;
    }

    shadnet::GetScoreFriendsRequest proto;
    proto.set_boardid(board_id);
    proto.set_includeself(include_self);
    proto.set_max(array_num);
    proto.set_withcomment(comment_array != nullptr);
    proto.set_withgameinfo(info_array != nullptr);

    const std::string proto_bytes = proto.SerializeAsString();
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        LOG_ERROR(NpHandler, "missing NP communication ID");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());

    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::GetScoreFriends, payload);
    {
        std::lock_guard lock(m_mutex_pending_score);
        PendingScoreRequest pending;
        pending.req = std::move(req);
        pending.cmd = ShadNet::CommandType::GetScoreFriends;
        pending.a_rank_array = rank_array;
        pending.comment_array = comment_array;
        pending.info_array = info_array;
        pending.last_sort_date = last_sort_date;
        pending.total_record = total_record;
        pending.array_num = array_num;
        pending.user_id = user_id;
        m_pending_score.emplace(pkt_id, std::move(pending));
    }
    LOG_INFO(NpHandler,
             "user_id={} service_label={} board={} include_self={} "
             "array_num={} with_comment={} with_game_info={} pkt_id={} com_id='{}'",
             user_id, service_label, board_id, include_self, array_num, comment_array != nullptr,
             info_array != nullptr, pkt_id, com_id);
    return ORBIS_OK;
}

static u32 FillPlainRankArrayFromProto(const shadnet::GetScoreResponse& resp, u64 max_slots,
                                       NpScore::OrbisNpScoreRankData* rank_array,
                                       NpScore::OrbisNpScoreComment* comment_array,
                                       NpScore::OrbisNpScoreGameInfo* info_array) {
    const s32 n_resp = resp.rankarray_size();
    u32 out_i = 0;
    for (s32 i = 0; i < n_resp && out_i < max_slots; ++i) {
        const auto& r = resp.rankarray(i);
        if (r.npid().empty()) {
            continue;
        }
        auto& out = rank_array[out_i];
        const std::string& npid = r.npid();
        const u64 cp = std::min<u64>(npid.size(), sizeof(out.np_id.handle.data));
        std::memcpy(out.np_id.handle.data, npid.data(), cp);
        out.np_id.handle.term = 0;
        LOG_INFO(NpHandler,
                 "out[{}] (resp[{}]) npid='{}' (len={}) rank={} "
                 "score={}",
                 out_i, i, npid, npid.size(), r.rank(), r.score());
        out.pc_id = r.pcid();
        out.serial_rank = r.rank();
        out.rank = r.rank();
        out.highest_rank = r.rank();
        out.has_game_data = r.hasgamedata() ? 1 : 0;
        out.score_value = r.score();
        out.record_date.tick = r.recorddate();

        if (comment_array != nullptr && i < resp.commentarray_size()) {
            const std::string& cmt = resp.commentarray(i);
            const u64 ccp =
                std::min<u64>(cmt.size(), sizeof(comment_array[out_i].utf8_comment) - 1);
            std::memcpy(comment_array[out_i].utf8_comment, cmt.data(), ccp);
        }
        if (info_array != nullptr && i < resp.infoarray_size()) {
            const std::string& gi = resp.infoarray(i).data();
            const u64 gcp = std::min<u64>(gi.size(), sizeof(info_array[out_i].data));
            info_array[out_i].info_size = static_cast<u32>(gcp);
            std::memcpy(info_array[out_i].data, gi.data(), gcp);
        }
        ++out_i;
    }
    return out_i;
}

static u32 FillPlainRankArrayAFromProto(const shadnet::GetScoreResponse& resp, u64 max_slots,
                                        NpScore::OrbisNpScoreRankDataA* rank_array,
                                        NpScore::OrbisNpScoreComment* comment_array,
                                        NpScore::OrbisNpScoreGameInfo* info_array) {
    const s32 n_resp = resp.rankarray_size();
    u32 out_i = 0;
    for (s32 i = 0; i < n_resp && out_i < max_slots; ++i) {
        const auto& r = resp.rankarray(i);
        if (r.npid().empty()) {
            continue;
        }
        auto& out = rank_array[out_i];
        const std::string& npid = r.npid();
        const u64 cp = std::min<u64>(npid.size(), sizeof(out.online_id.data));
        std::memcpy(out.online_id.data, npid.data(), cp);
        out.online_id.term = 0;
        LOG_INFO(NpHandler,
                 "out[{}] (resp[{}]) npid='{}' (len={}) rank={} "
                 "score={} account_id={}",
                 out_i, i, npid, npid.size(), r.rank(), r.score(), r.accountid());
        out.pc_id = r.pcid();
        out.serial_rank = r.rank();
        out.rank = r.rank();
        out.highest_rank = r.rank();
        out.has_game_data = r.hasgamedata() ? 1 : 0;
        out.score_value = r.score();
        out.record_date.tick = r.recorddate();
        out.account_id = static_cast<OrbisNpAccountId>(r.accountid());

        if (comment_array != nullptr && i < resp.commentarray_size()) {
            const std::string& cmt = resp.commentarray(i);
            const u64 ccp =
                std::min<u64>(cmt.size(), sizeof(comment_array[out_i].utf8_comment) - 1);
            std::memcpy(comment_array[out_i].utf8_comment, cmt.data(), ccp);
        }
        if (info_array != nullptr && i < resp.infoarray_size()) {
            const std::string& gi = resp.infoarray(i).data();
            const u64 gcp = std::min<u64>(gi.size(), sizeof(info_array[out_i].data));
            info_array[out_i].info_size = static_cast<u32>(gcp);
            std::memcpy(info_array[out_i].data, gi.data(), gcp);
        }
        ++out_i;
    }
    return out_i;
}

static u32 FillPlayerRankArrayAFromProto(const shadnet::GetScoreResponse& resp, u64 requested_count,
                                         NpScore::OrbisNpScorePlayerRankDataA* rank_array,
                                         NpScore::OrbisNpScoreComment* comment_array,
                                         NpScore::OrbisNpScoreGameInfo* info_array) {
    const s32 n_resp = resp.rankarray_size();
    const s32 n_req = static_cast<s32>(requested_count);
    if (n_resp != n_req) {
        LOG_WARNING(NpHandler, "response count {} != request count {}", n_resp, n_req);
    }
    const s32 n = std::min(n_resp, n_req);
    u32 found = 0;
    for (s32 i = 0; i < n; ++i) {
        const auto& r = resp.rankarray(i);
        if (r.npid().empty()) {
            continue;
        }
        auto& out = rank_array[i];
        out.has_data = 1;
        const std::string& npid = r.npid();
        const u64 cp = std::min<u64>(npid.size(), sizeof(out.rank_data.online_id.data));
        std::memcpy(out.rank_data.online_id.data, npid.data(), cp);
        out.rank_data.online_id.term = 0;
        out.rank_data.pc_id = r.pcid();
        out.rank_data.serial_rank = r.rank();
        out.rank_data.rank = r.rank();
        out.rank_data.highest_rank = r.rank();
        out.rank_data.has_game_data = r.hasgamedata() ? 1 : 0;
        out.rank_data.score_value = r.score();
        out.rank_data.record_date.tick = r.recorddate();
        out.rank_data.account_id = static_cast<OrbisNpAccountId>(r.accountid());

        if (comment_array != nullptr && i < resp.commentarray_size()) {
            const std::string& cmt = resp.commentarray(i);
            const u64 ccp = std::min<u64>(cmt.size(), sizeof(comment_array[i].utf8_comment) - 1);
            std::memcpy(comment_array[i].utf8_comment, cmt.data(), ccp);
        }
        if (info_array != nullptr && i < resp.infoarray_size()) {
            const std::string& gi = resp.infoarray(i).data();
            const u64 gcp = std::min<u64>(gi.size(), sizeof(info_array[i].data));
            info_array[i].info_size = static_cast<u32>(gcp);
            std::memcpy(info_array[i].data, gi.data(), gcp);
        }
        ++found;
    }
    return found;
}

static u32 FillRankArrayFromProto(const shadnet::GetScoreResponse& resp,
                                  const std::vector<std::string>& requested_np_ids,
                                  NpScore::OrbisNpScorePlayerRankData* rank_array,
                                  NpScore::OrbisNpScoreComment* comment_array,
                                  NpScore::OrbisNpScoreGameInfo* info_array) {
    const s32 n_resp = resp.rankarray_size();
    const s32 n_req = static_cast<s32>(requested_np_ids.size());
    const s32 n = std::min(n_resp, n_req);
    u32 found = 0;
    for (s32 i = 0; i < n; ++i) {
        const auto& r = resp.rankarray(i);
        if (r.npid().empty()) {
            continue;
        }

        auto& out = rank_array[i];
        out.has_data = 1;
        const std::string& npid = r.npid();
        const u64 cp = std::min<u64>(npid.size(), sizeof(out.rank_data.np_id.handle.data));
        std::memcpy(out.rank_data.np_id.handle.data, npid.data(), cp);
        out.rank_data.np_id.handle.term = 0;
        out.rank_data.pc_id = r.pcid();
        out.rank_data.serial_rank = r.rank();
        out.rank_data.rank = r.rank();
        out.rank_data.highest_rank = r.rank();
        out.rank_data.has_game_data = r.hasgamedata() ? 1 : 0;
        out.rank_data.score_value = r.score();
        out.rank_data.record_date.tick = r.recorddate();

        if (comment_array != nullptr && i < resp.commentarray_size()) {
            const std::string& cmt = resp.commentarray(i);
            const u64 ccp = std::min<u64>(cmt.size(), sizeof(comment_array[i].utf8_comment) - 1);
            std::memcpy(comment_array[i].utf8_comment, cmt.data(), ccp);
        }
        if (info_array != nullptr && i < resp.infoarray_size()) {
            const std::string& gi = resp.infoarray(i).data();
            const u64 gcp = std::min<u64>(gi.size(), sizeof(info_array[i].data));
            info_array[i].info_size = static_cast<u32>(gcp);
            std::memcpy(info_array[i].data, gi.data(), gcp);
        }
        ++found;
    }
    return found;
}

void NpHandler::OnScoreReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id,
                             ShadNet::ErrorType error, const std::vector<u8>& body) {
    PendingScoreRequest pending;
    {
        std::lock_guard lock(m_mutex_pending_score);
        auto it = m_pending_score.find(pkt_id);
        if (it == m_pending_score.end()) {
            LOG_WARNING(NpHandler, "no pending request for pkt_id={} cmd={}", pkt_id,
                        static_cast<s32>(cmd));
            return;
        }
        pending = std::move(it->second);
        m_pending_score.erase(it);
    }
    auto& req = pending.req;

    // Server-side errors take precedence over success-path parsing.
    if (error != ShadNet::ErrorType::NoError) {
        s32 orbis_err = ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE;
        switch (error) {
        case ShadNet::ErrorType::ScoreNotBest:
            orbis_err = ORBIS_NP_COMMUNITY_SERVER_ERROR_NOT_BEST_SCORE;
            break;
        case ShadNet::ErrorType::ScoreInvalid:
            orbis_err = ORBIS_NP_COMMUNITY_SERVER_ERROR_INVALID_SCORE;
            break;
        case ShadNet::ErrorType::NotFound:
            orbis_err = ORBIS_NP_COMMUNITY_SERVER_ERROR_RANKING_BOARD_MASTER_NOT_FOUND;
            break;
        case ShadNet::ErrorType::Unauthorized:
            orbis_err = ORBIS_NP_COMMUNITY_SERVER_ERROR_FORBIDDEN;
            break;
        case ShadNet::ErrorType::DbFail:
            orbis_err = ORBIS_NP_COMMUNITY_SERVER_ERROR_INTERNAL_SERVER_ERROR;
            break;
        case ShadNet::ErrorType::Malformed:
        case ShadNet::ErrorType::Invalid:
        case ShadNet::ErrorType::InvalidInput:
            orbis_err = ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE;
            break;
        default:
            break;
        }
        LOG_WARNING(NpHandler, "user_id={} pkt_id={} cmd={} server_error={} orbis_err={:#x}",
                    user_id, pkt_id, static_cast<s32>(cmd), static_cast<s32>(error), orbis_err);
        req->SetResult(orbis_err);
        return;
    }

    // Per-command success-path parsing.
    switch (cmd) {
    case ShadNet::CommandType::RecordScore: {
        if (req->tmp_rank_out != nullptr && body.size() >= 4) {
            const u32 rank = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                             (static_cast<u32>(body[2]) << 16) | (static_cast<u32>(body[3]) << 24);
            *req->tmp_rank_out = rank;
            LOG_INFO(NpHandler, "RecordScore user_id={} pkt_id={} rank={}", user_id, pkt_id, rank);
        }
        req->SetResult(ORBIS_OK);
        break;
    }

    case ShadNet::CommandType::RecordScoreData: {
        LOG_INFO(NpHandler, "RecordScoreData user_id={} pkt_id={} ok", user_id, pkt_id);
        req->SetResult(ORBIS_OK);
        break;
    }

    case ShadNet::CommandType::GetScoreData:
    case ShadNet::CommandType::GetScoreGameDataByAccId: {
        const char* cmd_name = (cmd == ShadNet::CommandType::GetScoreData)
                                   ? "GetScoreData"
                                   : "GetScoreGameDataByAccId";
        if (body.size() < 4) {
            LOG_ERROR(NpHandler, "invalid body size {}", body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 stored_size = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                                (static_cast<u32>(body[2]) << 16) |
                                (static_cast<u32>(body[3]) << 24);
        if (static_cast<u64>(4) + stored_size > body.size()) {
            LOG_ERROR(NpHandler, "invalid blob size {}", stored_size);
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        if (pending.total_size_out != nullptr) {
            *pending.total_size_out = stored_size;
        }
        if (pending.data_out != nullptr && pending.recv_size > 0) {
            const u64 copy_n = std::min<u64>(static_cast<u64>(stored_size), pending.recv_size);
            std::memcpy(pending.data_out, body.data() + 4, static_cast<u64>(copy_n));
            if (copy_n < stored_size) {
                LOG_WARNING(NpHandler, "{} truncated user_id={} pkt_id={} stored={} recv={}",
                            cmd_name, user_id, pkt_id, stored_size, pending.recv_size);
            }
        }
        LOG_INFO(NpHandler, "{} user_id={} pkt_id={} stored_size={} recv_size={}", cmd_name,
                 user_id, pkt_id, stored_size, pending.recv_size);
        req->SetResult(ORBIS_OK);
        break;
    }

    case ShadNet::CommandType::GetBoardInfos: {
        if (body.size() < 4) {
            LOG_ERROR(NpHandler, "invalid body size {}", body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 proto_size = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                               (static_cast<u32>(body[2]) << 16) |
                               (static_cast<u32>(body[3]) << 24);
        if (static_cast<u64>(4) + proto_size > body.size()) {
            LOG_ERROR(NpHandler, "invalid proto size {}", proto_size);
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        shadnet::BoardInfo bi;
        if (!bi.ParseFromArray(body.data() + 4, static_cast<s32>(proto_size))) {
            LOG_ERROR(NpHandler, "failed to parse reply");
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        if (pending.board_info != nullptr) {
            pending.board_info->rank_limit = bi.ranklimit();
            pending.board_info->update_mode = bi.updatemode();
            pending.board_info->sort_mode = bi.sortmode();
            pending.board_info->upload_num_limit = bi.uploadnumlimit();
            pending.board_info->upload_size_limit = bi.uploadsizelimit();
        }
        LOG_INFO(NpHandler,
                 "GetBoardInfos user_id={} pkt_id={} rank_limit={} update_mode={} "
                 "sort_mode={} upload_num_limit={} upload_size_limit={}",
                 user_id, pkt_id, bi.ranklimit(), bi.updatemode(), bi.sortmode(),
                 bi.uploadnumlimit(), bi.uploadsizelimit());
        req->SetResult(ORBIS_OK);
        break;
    }
    case ShadNet::CommandType::GetScoreNpId: {
        if (body.size() < 4) {
            LOG_ERROR(NpHandler, "invalid body size {}", body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 proto_size = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                               (static_cast<u32>(body[2]) << 16) |
                               (static_cast<u32>(body[3]) << 24);
        if (static_cast<u64>(4) + proto_size > body.size()) {
            LOG_ERROR(NpHandler, "invalid proto size {}", proto_size);
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        shadnet::GetScoreResponse resp;
        if (!resp.ParseFromArray(body.data() + 4, static_cast<s32>(proto_size))) {
            LOG_ERROR(NpHandler, "failed to parse reply");
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 found = FillRankArrayFromProto(resp, pending.requested_np_ids, pending.rank_array,
                                                 pending.comment_array, pending.info_array);
        if (pending.last_sort_date != nullptr) {
            pending.last_sort_date->tick = resp.lastsortdate();
        }
        if (pending.total_record != nullptr) {
            *pending.total_record = resp.totalrecord();
        }
        LOG_INFO(NpHandler, "GetScoreNpid user_id={} pkt_id={} found={}/{} total_record={}",
                 user_id, pkt_id, found, pending.array_num, resp.totalrecord());
        s32 result = found != 0 ? found : ORBIS_NP_COMMUNITY_SERVER_ERROR_GAME_RANKING_NOT_FOUND;
        req->SetResult(result);
        break;
    }
    case ShadNet::CommandType::GetScoreAccountId: {
        if (body.size() < 4) {
            LOG_ERROR(NpHandler, "invalid body size {}", body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 proto_size = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                               (static_cast<u32>(body[2]) << 16) |
                               (static_cast<u32>(body[3]) << 24);
        if (static_cast<u64>(4) + proto_size > body.size()) {
            LOG_ERROR(NpHandler, "invalid proto size {}", proto_size);
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        shadnet::GetScoreResponse resp;
        if (!resp.ParseFromArray(body.data() + 4, static_cast<s32>(proto_size))) {
            LOG_ERROR(NpHandler, "failed to parse reply");
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 found =
            FillPlayerRankArrayAFromProto(resp, pending.array_num, pending.a_player_rank_array,
                                          pending.comment_array, pending.info_array);
        if (pending.last_sort_date != nullptr) {
            pending.last_sort_date->tick = resp.lastsortdate();
        }
        if (pending.total_record != nullptr) {
            *pending.total_record = resp.totalrecord();
        }
        LOG_INFO(NpHandler, "GetScoreAccountId user_id={} pkt_id={} found={}/{} total_record={}",
                 user_id, pkt_id, found, pending.array_num, resp.totalrecord());
        s32 result = found != 0 ? found : ORBIS_NP_COMMUNITY_SERVER_ERROR_GAME_RANKING_NOT_FOUND;
        req->SetResult(result);
        break;
    }
    case ShadNet::CommandType::GetScoreRange:
    case ShadNet::CommandType::GetScoreFriends: {
        const char* cmd_name =
            (cmd == ShadNet::CommandType::GetScoreRange) ? "GetScoreRange" : "GetScoreFriends";
        if (body.size() < 4) {
            LOG_ERROR(NpHandler, "invalid body size {}", body.size());
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        const u32 proto_size = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                               (static_cast<u32>(body[2]) << 16) |
                               (static_cast<u32>(body[3]) << 24);
        if (static_cast<u64>(4) + proto_size > body.size()) {
            LOG_ERROR(NpHandler, "invalid proto size {}", proto_size);
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        shadnet::GetScoreResponse resp;
        if (!resp.ParseFromArray(body.data() + 4, static_cast<s32>(proto_size))) {
            LOG_ERROR(NpHandler, "failed to parse reply");
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            break;
        }
        u32 found = 0;
        if (pending.a_rank_array != nullptr) {
            found = FillPlainRankArrayAFromProto(resp, pending.array_num, pending.a_rank_array,
                                                 pending.comment_array, pending.info_array);
        } else {
            found = FillPlainRankArrayFromProto(resp, pending.array_num, pending.plain_rank_array,
                                                pending.comment_array, pending.info_array);
        }
        if (pending.last_sort_date != nullptr) {
            pending.last_sort_date->tick = resp.lastsortdate();
        }
        if (pending.total_record != nullptr) {
            *pending.total_record = resp.totalrecord();
        }
        LOG_INFO(NpHandler, "{} user_id={} pkt_id={} found={}/{} total_record={}{}", cmd_name,
                 user_id, pkt_id, found, pending.array_num, resp.totalrecord(),
                 pending.a_rank_array != nullptr ? " (A-variant)" : "");
        s32 result = found != 0 || cmd == ShadNet::CommandType::GetScoreFriends
                         ? found
                         : ORBIS_NP_COMMUNITY_SERVER_ERROR_GAME_RANKING_NOT_FOUND;
        req->SetResult(result);
        break;
    }

    default:
        LOG_WARNING(NpHandler, "unexpected cmd={} pkt_id={}", static_cast<s32>(cmd), pkt_id);
        req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
        break;
    }
}

} // namespace Libraries::Np
