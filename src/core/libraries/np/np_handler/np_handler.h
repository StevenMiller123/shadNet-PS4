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
#include "core/libraries/np/np_tus/np_tus.h"
#include "core/libraries/np/np_tus/np_tus_ctx.h"
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
        std::scoped_lock lk{m_mutex_clients};
        if (!m_clients.contains(user_id)) {
            return false;
        }
        return m_initialized && m_clients[user_id]->IsAuthenticated();
    };

    // For compatibility
    bool IsPsnSignedIn(s32 user_id) {
        return IsSignedIn(user_id);
    }

    OrbisNpId GetNpId(s32 user_id) {
        std::scoped_lock lk{m_mutex_clients};
        if (m_np_ids.contains(user_id)) {
            return m_np_ids[user_id];
        }
        return OrbisNpId{};
    };

    std::string GetBearerToken(s32 user_id) const;
    s32 RequestAuthorizationCode(s32 user_id, const std::string& client_id,
                                 const std::string& scope,
                                 std::shared_ptr<NpAuth::AuthRequestCtx> ctx);
    u64 GetAccountId(s32 user_id) const;
    s32 GetUserIdByAccountId(u64 account_id) const;
    s32 GetUserIdByOnlineId(const OrbisNpOnlineId& online_id) const;

    // NpScore Requests
    s32 RecordScore(s32 user_id, s32 service_label, u32 board_id, s32 pc_id, s64 score,
                    const char* comment, u64 comment_len, const u8* game_info_data,
                    u64 game_info_size, std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 RecordGameData(s32 user_id, s32 service_label, u32 board_id, s32 pc_id, s64 score,
                       const u8* data, u64 size, std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 GetGameData(s32 user_id, s32 service_label, u32 board_id, const std::string& np_id,
                    s32 pc_id, void* data_out, u64 recv_size, u64* total_size_out,
                    std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 GetGameDataByAccountId(s32 user_id, s32 service_label, u32 board_id, u64 account_id,
                               s32 pc_id, void* data_out, u64 recv_size, u64* total_size_out,
                               std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 GetBoardInfo(s32 user_id, s32 service_label, u32 board_id,
                     NpScore::OrbisNpScoreBoardInfo* board_info,
                     std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 GetRankingByNpId(s32 user_id, s32 service_label, u32 board_id,
                         const std::vector<std::string>& np_ids, const std::vector<s32>& pc_ids,
                         NpScore::OrbisNpScorePlayerRankData* rank_array,
                         NpScore::OrbisNpScoreComment* comment_array,
                         NpScore::OrbisNpScoreGameInfo* info_array,
                         Rtc::OrbisRtcTick* last_sort_date, u32* total_record,
                         std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 GetRankingByRange(s32 user_id, s32 service_label, u32 board_id, u32 start_serial_rank,
                          u32 array_num, NpScore::OrbisNpScoreRankData* rank_array,
                          NpScore::OrbisNpScoreComment* comment_array,
                          NpScore::OrbisNpScoreGameInfo* info_array,
                          Rtc::OrbisRtcTick* last_sort_date, u32* total_record,
                          std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 GetRankingByRangeA(s32 user_id, s32 service_label, u32 board_id, u32 start_serial_rank,
                           u32 array_num, NpScore::OrbisNpScoreRankDataA* rank_array,
                           NpScore::OrbisNpScoreComment* comment_array,
                           NpScore::OrbisNpScoreGameInfo* info_array,
                           Rtc::OrbisRtcTick* last_sort_date, u32* total_record,
                           std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 GetRankingByAccountId(s32 user_id, s32 service_label, u32 board_id,
                              const std::vector<u64>& account_ids, const std::vector<s32>& pc_ids,
                              NpScore::OrbisNpScorePlayerRankDataA* rank_array,
                              NpScore::OrbisNpScoreComment* comment_array,
                              NpScore::OrbisNpScoreGameInfo* info_array,
                              Rtc::OrbisRtcTick* last_sort_date, u32* total_record,
                              std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 GetFriendsRanking(s32 user_id, s32 service_label, u32 board_id, bool include_self,
                          u32 array_num, NpScore::OrbisNpScoreRankData* rank_array,
                          NpScore::OrbisNpScoreComment* comment_array,
                          NpScore::OrbisNpScoreGameInfo* info_array,
                          Rtc::OrbisRtcTick* last_sort_date, u32* total_record,
                          std::shared_ptr<NpScore::ScoreRequestCtx> req);
    s32 GetFriendsRankingA(s32 user_id, s32 service_label, u32 board_id, bool include_self,
                           u32 array_num, NpScore::OrbisNpScoreRankDataA* rank_array,
                           NpScore::OrbisNpScoreComment* comment_array,
                           NpScore::OrbisNpScoreGameInfo* info_array,
                           Rtc::OrbisRtcTick* last_sort_date, u32* total_record,
                           std::shared_ptr<NpScore::ScoreRequestCtx> req);

    // NpTus Requests
    s32 TusGetMultiSlotVariable(s32 user_id, s32 service_label, const std::string& owner_np_id,
                                const std::string& virtual_user, const std::vector<s32>& slot_ids,
                                NpTus::OrbisNpTusVariable* variables_out, u64 array_num,
                                std::shared_ptr<NpTus::TusRequestCtx> ctx,
                                NpTus::OrbisNpTusVariableA* variables_a_out = nullptr,
                                s64 owner_account_id = 0,
                                NpTus::OrbisNpTusVariableForCrossSave* variables_cs_out = nullptr);
    s32 TusSetMultiSlotVariable(s32 user_id, s32 service_label, const std::string& owner_np_id,
                                const std::string& virtual_user, const std::vector<s32>& slot_ids,
                                const std::vector<s64>& values,
                                std::shared_ptr<NpTus::TusRequestCtx> ctx,
                                s64 owner_account_id = 0);
    s32 TssGetData(s32 user_id, s32 service_label, s32 slot_id, bool has_offset, u64 offset,
                   bool has_last_byte, u64 last_byte, bool has_if_param, s32 if_type,
                   u64 if_last_modified, NpTus::OrbisNpTssDataStatus* status_out, void* data_out,
                   u64 data_cap, std::shared_ptr<NpTus::TusRequestCtx> ctx,
                   u64* content_length_out = nullptr);
    s32 TusGetData(s32 user_id, s32 service_label, const std::string& owner_np_id,
                   const std::string& virtual_user, s64 owner_account_id, s32 slot_id,
                   NpTus::OrbisNpTusDataStatusA* status_a_out, u64 status_cap, void* data_out,
                   u64 data_cap, u64 data_offset, std::shared_ptr<NpTus::TusRequestCtx> ctx,
                   NpTus::OrbisNpTusDataStatus* status_out = nullptr,
                   NpTus::OrbisNpTusDataStatusForCrossSave* status_cs_out = nullptr);
    s32 TusDeleteMultiSlotData(s32 user_id, s32 service_label, const std::string& owner_np_id,
                               const std::string& virtual_user, s64 owner_account_id,
                               const std::vector<s32>& slot_ids,
                               std::shared_ptr<NpTus::TusRequestCtx> ctx);
    s32 TusDeleteMultiSlotVariable(s32 user_id, s32 service_label, const std::string& owner_np_id,
                                   const std::string& virtual_user, s64 owner_account_id,
                                   const std::vector<s32>& slot_ids,
                                   std::shared_ptr<NpTus::TusRequestCtx> ctx);
    s32 TusGetMultiUserDataStatus(s32 user_id, s32 service_label, s32 slot_id,
                                  const std::vector<std::string>& owner_np_ids,
                                  const std::vector<std::string>& virtual_users,
                                  const std::vector<s64>& owner_account_ids,
                                  NpTus::OrbisNpTusDataStatus* status_out,
                                  NpTus::OrbisNpTusDataStatusA* status_a_out, u64 array_num,
                                  std::shared_ptr<NpTus::TusRequestCtx> ctx,
                                  NpTus::OrbisNpTusDataStatusForCrossSave* status_cs_out = nullptr);
    s32 TusGetMultiUserVariable(s32 user_id, s32 service_label, s32 slot_id,
                                const std::vector<std::string>& owner_np_ids,
                                const std::vector<std::string>& virtual_users,
                                const std::vector<s64>& owner_account_ids,
                                NpTus::OrbisNpTusVariable* variables_out,
                                NpTus::OrbisNpTusVariableA* variables_a_out, u64 array_num,
                                std::shared_ptr<NpTus::TusRequestCtx> ctx,
                                NpTus::OrbisNpTusVariableForCrossSave* variables_cs_out = nullptr);
    s32 TusTryAndSetVariable(s32 user_id, s32 service_label, const std::string& owner_np_id,
                             const std::string& virtual_user, s64 owner_account_id, s32 slot_id,
                             s32 ope_type, s64 value, bool has_compare, s64 compare_value,
                             bool has_author_check, s64 is_last_changed_author,
                             const std::string& is_last_changed_author_np_id, bool has_date_check,
                             u64 is_last_changed_date, NpTus::OrbisNpTusVariable* variable_out,
                             NpTus::OrbisNpTusVariableA* variable_a_out,
                             std::shared_ptr<NpTus::TusRequestCtx> ctx,
                             NpTus::OrbisNpTusVariableForCrossSave* variable_cs_out = nullptr);
    s32 TusAddAndGetVariable(s32 user_id, s32 service_label, const std::string& owner_np_id,
                             const std::string& virtual_user, s64 owner_account_id, s32 slot_id,
                             s64 in_variable, bool has_author_check, s64 is_last_changed_author,
                             const std::string& is_last_changed_author_np_id, bool has_date_check,
                             u64 is_last_changed_date, NpTus::OrbisNpTusVariable* variable_out,
                             NpTus::OrbisNpTusVariableA* variable_a_out,
                             std::shared_ptr<NpTus::TusRequestCtx> ctx,
                             NpTus::OrbisNpTusVariableForCrossSave* variable_cs_out = nullptr);
    s32 TusGetFriendsDataStatus(s32 user_id, s32 service_label, s32 slot_id, bool include_self,
                                s32 sort_type, u32 max, NpTus::OrbisNpTusDataStatus* status_out,
                                NpTus::OrbisNpTusDataStatusA* status_a_out, u64 array_num,
                                std::shared_ptr<NpTus::TusRequestCtx> ctx, u32 start_offset = 0,
                                u32* hits_out = nullptr,
                                NpTus::OrbisNpTusDataStatusForCrossSave* status_cs_out = nullptr);
    s32 TusGetFriendsVariable(s32 user_id, s32 service_label, s32 slot_id, bool include_self,
                              s32 sort_type, u32 max, NpTus::OrbisNpTusVariable* variables_out,
                              NpTus::OrbisNpTusVariableA* variables_a_out, u64 array_num,
                              std::shared_ptr<NpTus::TusRequestCtx> ctx, u32 start_offset = 0,
                              u32* hits_out = nullptr,
                              NpTus::OrbisNpTusVariableForCrossSave* variables_cs_out = nullptr);
    s32 TusGetMultiSlotDataStatus(s32 user_id, s32 service_label, const std::string& owner_np_id,
                                  const std::string& virtual_user, s64 owner_account_id,
                                  const std::vector<s32>& slot_ids,
                                  NpTus::OrbisNpTusDataStatus* status_out,
                                  NpTus::OrbisNpTusDataStatusA* status_a_out, u64 array_num,
                                  std::shared_ptr<NpTus::TusRequestCtx> ctx,
                                  NpTus::OrbisNpTusDataStatusForCrossSave* status_cs_out = nullptr);
    s32 TusSetData(s32 user_id, s32 service_label, const std::string& owner_np_id,
                   const std::string& virtual_user, s64 owner_account_id, s32 slot_id,
                   const std::vector<u8>& blob, const std::vector<u8>& info, bool has_author_check,
                   s64 is_last_changed_author, const std::string& is_last_changed_author_np_id,
                   bool has_date_check, u64 is_last_changed_date,
                   std::shared_ptr<NpTus::TusRequestCtx> ctx);

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
    void OnTusReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id, ShadNet::ErrorType error,
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
        Rtc::OrbisRtcTick* last_sort_date = nullptr;
        u32* total_record = nullptr;
        u64 array_num = 0;
    };
    mutable std::mutex m_mutex_pending_score;
    std::map<u64, PendingScoreRequest> m_pending_score;

    // TUS state
    struct PendingTusRequest {
        std::shared_ptr<NpTus::TusRequestCtx> req;
        ShadNet::CommandType cmd;
        s32 user_id = -1;
        NpTus::OrbisNpTusVariable* variable_array = nullptr;
        NpTus::OrbisNpTusVariableA* variable_array_a = nullptr;
        NpTus::OrbisNpTusVariableForCrossSave* variable_array_cs = nullptr;
        NpTus::OrbisNpTusDataStatus* status_array = nullptr;
        NpTus::OrbisNpTusDataStatusA* status_array_a = nullptr;
        NpTus::OrbisNpTusDataStatusForCrossSave* status_array_cs = nullptr;
        void* data_out = nullptr;
        u64 data_cap = 0;
        u64 data_offset = 0;
        u64 status_cap = 0;
        NpTus::OrbisNpTssDataStatus* tss_status_out = nullptr;
        u64* tss_content_length_out = nullptr;
        u32* total_out = nullptr;
        u64 array_num = 0;
    };
    mutable std::mutex m_mutex_pending_tus;
    std::map<u64, PendingTusRequest> m_pending_tus;

    // Invitations
    struct PendingInvitation {
        std::string session_id;
        std::string invitation_id;
        std::string from_npid;
        std::string to_npid;
        s64 valid_until = 0;
    };
    mutable std::mutex m_mutex_pending_invites;
    std::map<s32, std::vector<PendingInvitation>> m_pending_invites;
};

} // namespace Libraries::Np
