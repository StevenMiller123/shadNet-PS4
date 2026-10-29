// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <map>
#include "common/singleton.h"
#include "common/types.h"
#include "core/libraries/np/np_auth/np_auth_ctx.h"
#include "core/libraries/np/np_manager/np_manager.h"
#include "core/libraries/np/np_score/np_score.h"
#include "core/libraries/np/np_score/np_score_ctx.h"
#include "core/libraries/np/np_types.h"
#include "shadnet/client.h"

namespace Libraries::Np {

class NpHandler {
public:
    static NpHandler& Instance() {
        return *Common::Singleton<NpHandler>::Instance();
    }

    // For compatibility
    static NpHandler& GetInstance() {
        return Instance();
    }

    void Initialize();
    void OnUserLoggedIn(s32 user_id);
    void OnUserLoggedOut(s32 user_id);

    bool IsActive() {
        return m_initialized;
    };

    bool IsSignedIn(s32 user_id) {
        if (!m_clients.contains(user_id)) {
            return false;
        }
        return m_initialized && m_clients[user_id]->IsAuthenticated();
    };

    // For compatibility
    bool IsPsnSignedIn(s32 user_id) {
        return IsSignedIn(user_id);
    }

    OrbisNpId& GetNpId(s32 user_id) {
        return m_np_ids[user_id];
    };

    std::string GetBearerToken(s32 user_id) const;
    s32 RequestAuthorizationCode(s32 user_id, const std::string& client_id,
                                 const std::string& scope,
                                 std::shared_ptr<NpAuth::AuthRequestCtx> ctx);
    u64 GetAccountId(s32 user_id) const;
    s32 GetUserIdByAccountId(u64 account_id) const;
    s32 GetUserIdByOnlineId(const OrbisNpOnlineId& online_id) const;

    // NpScore Requests
    s32 RecordScore(s32 user_id, s32 service_label, u32 boardId, s32 pcId, s64 score,
                    const char* comment, u64 commentLen, const u8* gameInfoData, u64 gameInfoSize,
                    std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 RecordGameData(s32 user_id, s32 service_label, u32 boardId, s32 pcId, s64 score,
                       const u8* data, u64 size, std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 GetGameData(s32 user_id, s32 service_label, u32 boardId, const std::string& npId, s32 pcId,
                    void* dataOut, u64 recvSize, u64* totalSizeOut,
                    std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 GetGameDataByAccountId(s32 user_id, s32 service_label, u32 boardId, u64 accountId, s32 pcId,
                               void* dataOut, u64 recvSize, u64* totalSizeOut,
                               std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 GetBoardInfo(s32 user_id, s32 service_label, u32 boardId,
                     NpScore::OrbisNpScoreBoardInfo* boardInfo,
                     std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 GetRankingByNpId(s32 user_id, s32 service_label, u32 boardId,
                         const std::vector<std::string>& npIds, const std::vector<s32>& pcIds,
                         NpScore::OrbisNpScorePlayerRankData* rankArray,
                         NpScore::OrbisNpScoreComment* commentArray,
                         NpScore::OrbisNpScoreGameInfo* infoArray,
                         Libraries::Rtc::OrbisRtcTick* lastSortDate, u32* totalRecord,
                         std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 GetRankingByRange(s32 user_id, s32 service_label, u32 boardId, u32 startSerialRank,
                          u32 arrayNum, NpScore::OrbisNpScoreRankData* rankArray,
                          NpScore::OrbisNpScoreComment* commentArray,
                          NpScore::OrbisNpScoreGameInfo* infoArray,
                          Libraries::Rtc::OrbisRtcTick* lastSortDate, u32* totalRecord,
                          std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 GetRankingByRangeA(s32 user_id, s32 service_label, u32 boardId, u32 startSerialRank,
                           u32 arrayNum, NpScore::OrbisNpScoreRankDataA* rankArray,
                           NpScore::OrbisNpScoreComment* commentArray,
                           NpScore::OrbisNpScoreGameInfo* infoArray,
                           Libraries::Rtc::OrbisRtcTick* lastSortDate, u32* totalRecord,
                           std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 GetRankingByAccountId(s32 user_id, s32 service_label, u32 boardId,
                              const std::vector<u64>& accountIds, const std::vector<s32>& pcIds,
                              NpScore::OrbisNpScorePlayerRankDataA* rankArray,
                              NpScore::OrbisNpScoreComment* commentArray,
                              NpScore::OrbisNpScoreGameInfo* infoArray,
                              Libraries::Rtc::OrbisRtcTick* lastSortDate, u32* totalRecord,
                              std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 GetFriendsRanking(s32 user_id, s32 service_label, u32 boardId, bool includeSelf,
                          u32 arrayNum, NpScore::OrbisNpScoreRankData* rankArray,
                          NpScore::OrbisNpScoreComment* commentArray,
                          NpScore::OrbisNpScoreGameInfo* infoArray,
                          Libraries::Rtc::OrbisRtcTick* lastSortDate, u32* totalRecord,
                          std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 GetFriendsRankingA(s32 user_id, s32 service_label, u32 boardId, bool includeSelf,
                           u32 arrayNum, NpScore::OrbisNpScoreRankDataA* rankArray,
                           NpScore::OrbisNpScoreComment* commentArray,
                           NpScore::OrbisNpScoreGameInfo* infoArray,
                           Libraries::Rtc::OrbisRtcTick* lastSortDate, u32* totalRecord,
                           std::shared_ptr<NpScore::ScoreRequestCtx> req);

    // State callbacks
    using StateCallback = void (*)(s32 user_id, OrbisNpState state);
    s32 RegisterStateCallback(StateCallback cb, void* userdata);
    void UnregisterStateCallback(s32 handle);

private:
    // User management
    bool ConnectUser(s32 user_id, const std::string& host, u16 port, const std::string& npid,
                     const std::string& password);
    bool ConnectUserById(s32 user_id);
    void MarkForReconnect(s32 user_id);
    void TryReconnect();
    void DisconnectUser(s32 user_id);
    void FailPendingRequests(s32 user_id, s32 error_code);

    // NP Communication ID validation
    bool IsValidNpCommId(const std::string& id);
    std::string GetNpCommId(s32 service_label) const;

    // Workers
    void StartWorker();
    void WorkerThread();

    // Per-user worker threads
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_worker_running{false};
    std::thread m_worker_thread;

    struct ReconnectState {
        std::chrono::steady_clock::time_point next_attempt{};
        std::chrono::milliseconds backoff{0};
    };
    std::unordered_map<s32, ReconnectState> m_reconnects;

    // Client
    mutable std::mutex m_mutex_clients;
    std::map<s32, std::shared_ptr<ShadNet::ShadNetClient>> m_clients;
    std::map<s32, OrbisNpId> m_np_ids;

    // Client callbacks
    void OnFriendQuery(s32 user_id, const ShadNet::NotifyFriendQuery& n);
    void OnFriendNew(s32 user_id, const ShadNet::NotifyFriendNew& n);
    void OnFriendLost(s32 user_id, const ShadNet::NotifyFriendLost& n);
    void OnFriendStatus(s32 user_id, const ShadNet::NotifyFriendStatus& n);
    void OnWebApiPushEvent(s32 user_id, const ShadNet::NotifyWebApiPushEvent& n);
    void OnAsyncReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id, ShadNet::ErrorType error,
                      const std::vector<u8>& body);
    void OnScoreReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id, ShadNet::ErrorType error,
                      const std::vector<u8>& body);
    void OnServiceUnavailable(s32 user_id, const ShadNet::NotifyServiceUnavailable& n);
    void OnClientRejected(s32 user_id, const ShadNet::NotifyClientRejected& n);
    void OnLoginResult(s32 user_id, const ShadNet::LoginResult& res);

    // Auth callbacks
    struct PendingAuthRequest {
        std::shared_ptr<NpAuth::AuthRequestCtx> req;
        s32 user_id = -1;
    };
    mutable std::mutex m_mutex_pending_auth;
    std::map<u64, PendingAuthRequest> m_pending_auth;
    void OnAuthReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id, ShadNet::ErrorType error,
                     const std::vector<u8>& body);

    // State callbacks
    struct CbEntry {
        s32 handle;
        StateCallback cb;
        void* userdata;
    };
    mutable std::mutex m_mutex_cbs;
    std::map<s32, CbEntry> m_state_cbs;
    std::atomic<s32> m_next_handle;

    void FireStateCallback(s32 user_id, OrbisNpState state);

    // Friend state
    struct FriendInfo {
        std::string npid;
        bool online = false;
    };
    struct FriendListSnapshot {
        std::vector<FriendInfo> friends;
        std::vector<std::string> requests_received;
        std::vector<std::string> requests_sent;
        std::vector<std::string> blocked;
    };
    mutable std::mutex m_mutex_friend_state;
    std::map<s32, FriendListSnapshot> m_friend_states;

    // Score state
    struct PendingScoreRequest {
        std::shared_ptr<NpScore::ScoreRequestCtx> req;
        ShadNet::CommandType cmd;
        s32 user_id = -1;
        std::vector<std::string> requested_np_ids;
        NpScore::OrbisNpScorePlayerRankData* rank_array = nullptr;
        NpScore::OrbisNpScoreRankData* plain_rank_array = nullptr;
        NpScore::OrbisNpScoreRankDataA* a_rank_array = nullptr;
        NpScore::OrbisNpScorePlayerRankDataA* a_player_rank_array = nullptr;
        NpScore::OrbisNpScoreComment* comment_array = nullptr;
        NpScore::OrbisNpScoreGameInfo* info_array = nullptr;
        NpScore::OrbisNpScoreBoardInfo* board_info = nullptr;
        void* data_out = nullptr;
        u64 recv_size = 0;
        u64* total_size_out = nullptr;
        Libraries::Rtc::OrbisRtcTick* last_sort_date = nullptr;
        u32* total_record = nullptr;
        u64 array_num = 0;
    };
    mutable std::mutex m_mutex_pending_score;
    std::map<u64, PendingScoreRequest> m_pending_score;

    // Invitations
    struct PendingInvitation {
        std::string session_id;
        std::string invitation_id;
        std::string from_npid;
        std::string to_npid;
        int64_t valid_until = 0;
    };
    mutable std::mutex m_mutex_pending_invites;
    std::map<s32, std::vector<PendingInvitation>> m_pending_invites;
};

} // namespace Libraries::Np