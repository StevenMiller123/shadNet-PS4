// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <semaphore>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <unistd.h>
#include "common/types.h"

namespace ShadNet {

// Protocol constants
static constexpr u32 SHAD_HEADER_SIZE = 15;
static constexpr u32 SHAD_CONNECT_TIMEOUT_MS = 10000;
static constexpr u32 SHAD_CONNECT_MAX_ATTEMPTS = 4;
static constexpr u32 SHAD_CONNECT_RETRY_BACKOFF_MS = 1000;
static constexpr u32 SHAD_PROTOCOL_VERSION = 1;
static constexpr u32 SHAD_MAX_PACKET_SIZE = 0x800000;

enum class PacketType : u8 {
    Request = 0,
    Reply = 1,
    Notification = 2,
    ServerInfo = 3,
};

enum class CommandType : u16 {
    Login = 0,
    Terminate = 1,
    Create = 2,
    Delete = 3,
    SendToken = 4,
    SendResetToken = 5,
    ResetPassword = 6,
    ResetState = 7,
    AddFriend = 8,
    RemoveFriend = 9,
    AddBlock = 10,
    RemoveBlock = 11,
    GetServerFeatures = 12,
    SetClientVersion = 13,
    // Score
    GetBoardInfos = 30,
    RecordScore = 31,
    RecordScoreData = 32,
    GetScoreData = 33,
    GetScoreRange = 34,
    GetScoreFriends = 35,
    GetScoreNpId = 36,
    GetScoreAccountId = 37,
    GetScoreGameDataByAccId = 38,
    GetToken = 39,
    SetAppearOffline = 40,
    LookupOnlineId = 41,
    GetAuthorizationCode = 42,
    // Matchmaking
    ContextStart = 100,
    CreateRoom = 101,
    JoinRoom = 102,
    LeaveRoom = 103,
    SearchRoom = 104,
    RequestSignalingInfos = 105,
    ContextStop = 106,
    SetUserInfo = 107,
    SetRoomDataInternal = 108,
    SetRoomDataExternal = 109,
    KickoutRoomMember = 110,
    GetWorldInfoList = 111,
    GetRoomDataExternalList = 112,
    GetUserInfoList = 113,
    GetRoomMemberDataExternalList = 114,
    SendRoomMessage = 115,
    SetRoomMemberDataInternal = 116,
    // Title User Storage
    TusSetData = 201,
    TusGetData = 202,
    TusSetMultiSlotVariable = 203,
    TusGetMultiSlotVariable = 204,
    TusAddAndGetVariable = 205,
    TusGetMultiSlotDataStatus = 206,
    TusGetMultiUserDataStatus = 207,
    TusGetFriendsDataStatus = 208,
    TusDeleteMultiSlotData = 209,
    TusGetMultiUserVariable = 210,
    TusTryAndSetVariable = 211,
    TusGetFriendsVariable = 212,
    TusDeleteMultiSlotVariable = 213,
    TssGetData = 214,
    // Trophies
    UnlockTrophy = 301,
    SyncTrophies = 302,
};

enum class NotificationType : u16 {
    FriendQuery = 5,
    FriendNew = 6,
    FriendLost = 7,
    FriendStatus = 8,
    RoomEvent = 10,
    RoomMessage = 11,
    WebApiPushEvent = 17,
    ServiceUnavailable = 18,
    ClientRejected = 19,
};

enum class ErrorType : uint8_t {
    NoError = 0,
    Malformed = 1,
    Invalid = 2,
    InvalidInput = 3,
    TooSoon = 4,
    LoginError = 5,
    LoginAlreadyLoggedIn = 6,
    LoginInvalidUsername = 7,
    LoginInvalidPassword = 8,
    LoginInvalidToken = 9,
    CreationError = 10,
    CreationExistingUsername = 11,
    CreationBannedEmailProvider = 12,
    CreationExistingEmail = 13,
    RoomMissing = 14,
    RoomAlreadyJoined = 15,
    RoomFull = 16,
    RoomPasswordMismatch = 17,
    RoomPasswordMissing = 18,
    RoomGroupNoJoinLabel = 19,
    RoomGroupFull = 20,
    RoomGroupJoinLabelNotFound = 21,
    RoomGroupMaxSlotMismatch = 22,
    Unauthorized = 23,
    DbFail = 24,
    EmailFail = 25,
    NotFound = 26,
    Blocked = 27,
    AlreadyFriend = 28,
    ScoreNotBest = 29,
    ScoreInvalid = 30,
    ScoreHasData = 31,
    CondFail = 32,
    Unsupported = 33,
    TooLarge = 34,
};

enum class ShadNetState {
    Ok,
    FailureInput,
    FailureResolve,
    FailureConnect,
    FailureServerInfo,
    FailureAuth,
    FailureAlreadyIn,
    FailureUsername,
    FailurePassword,
    FailureToken,
    FailureProtocol,
    FailureOther,
};

// Callback data structures
struct FriendEntry {
    std::string npid;
    bool online = false;
};

struct LoginResult {
    ErrorType error = ErrorType::Malformed;
    std::string avatar_url;
    u64 user_id = 0;
    std::vector<FriendEntry> friends;
    std::vector<std::string> requests_sent;
    std::vector<std::string> requests_received;
    std::vector<std::string> blocked;
};

struct NotifyFriendQuery {
    std::string from_npid;
};

struct NotifyFriendNew {
    std::string npid;
    bool online = false;
};

struct NotifyFriendLost {
    std::string npid;
};

struct NotifyFriendStatus {
    std::string npid;
    bool online = false;
    u64 timestamp = 0;
};

struct NotifyServiceUnavailable {
    enum : u32 { ServiceMatching2 = 1 };
    enum : u32 { ReasonServerFull = 1, ReasonNotAllowed = 2, ReasonDisabled = 3 };
    u32 service = 0;
    u32 reason = 0;
    u32 in_use = 0;
    u32 limit = 0;
};

struct NotifyClientRejected {
    enum : u32 { ReasonVersion = 1 };
    u32 reason = 0;
    std::string message;
    std::string version;
};

struct NotifyWebApiPushEvent {
    std::string np_service_name;
    u32 np_service_label = 0;
    std::string data_type;
    std::string data;
    std::string from_npid;
    std::string to_npid;
    std::vector<std::pair<std::string, std::string>> extd_data;
    u64 from_account_id = 0;
    u64 to_account_id = 0;
};

struct MatchingBinAttr {
    u32 attr_id = 0;
    u64 update_date = 0;
    u32 update_member_id = 0;
    std::vector<u8> data;
};

struct NotifyRoomEvent {
    u32 ctx_id = 0;
    u64 room_id = 0;
    u32 event = 0;
    u32 event_cause = 0;
    s32 error_code = 0;
    u32 flags = 0;
    bool has_passwd_mask = false;
    u64 passwd_slot_mask = 0;

    std::string member_npid;
    u64 member_account_id = 0;
    u32 member_platform = 0;
    u32 member_id = 0;
    u32 member_team_id = 0;
    bool member_is_owner = false;
    u64 member_join_date = 0;
    u32 member_nat_type = 0;
    u32 member_flag_attr = 0;
    u32 member_group_id = 0;
    std::string member_addr;
    u32 member_port = 0;
    std::vector<MatchingBinAttr> member_bin_attrs;

    std::vector<MatchingBinAttr> bin_attrs;
};

struct NotifyRoomMessage {
    u32 ctx_id = 0;
    u64 room_id = 0;
    u32 src_member_id = 0;
    u32 event = 0;
    u32 cast_type = 0;
    std::vector<u32> dst_member_ids;
    std::string src_npid;
    u64 src_account_id = 0;
    u32 src_platform = 0;
    std::vector<u8> msg;
};

class ShadNetClient {
public:
    ShadNetClient();
    ~ShadNetClient();
    ShadNetClient(const ShadNetClient&) = delete;
    ShadNetClient& operator=(const ShadNetClient&) = delete;

    void Start(const std::string& host, u16 port, const std::string& npid,
               const std::string& password, const std::string& token = {});
    void Stop();

    ShadNetState WaitForConnection();
    ShadNetState WaitForAuthenticated();

    bool IsConnected() const;
    bool IsAuthenticated() const;
    ShadNetState GetState() const;

    const std::string& GetAvatarUrl() const;
    u64 GetUserId() const;
    const std::string& GetNpid() const;
    u32 GetServerProtocolVersion() const {
        return m_server_protocol_version.load();
    }
    u32 GetAddrLocal() const;
    u32 GetAddrServer() const;
    bool IsMatching2Enabled() const;
    static bool OnReaderThread();
    u32 GetNumFriends() const;
    std::optional<std::string> GetFriendNpid(u32 index) const;
    u64 ReportClientVersion();
    static std::string BuildVersionString();

    std::string GetBearerToken() const;

    // Callbacks
    std::function<void(const LoginResult&)> on_login_result;
    std::function<void(const NotifyFriendQuery&)> on_friend_query;
    std::function<void(const NotifyFriendNew&)> on_friend_new;
    std::function<void(const NotifyFriendLost&)> on_friend_lost;
    std::function<void(const NotifyFriendStatus&)> on_friend_status;
    std::function<void(const NotifyRoomEvent&)> on_room_event;
    std::function<void(const NotifyRoomMessage&)> on_room_message;
    std::function<void(const NotifyWebApiPushEvent&)> on_web_api_push_event;
    std::function<void(const NotifyServiceUnavailable&)> on_service_unavailable;
    std::function<void(const NotifyClientRejected&)> on_client_rejected;
    std::function<void(CommandType cmd, u64 pkt_id, ErrorType error, const std::vector<u8>& body)>
        on_async_reply;

    u64 SubmitRequest(CommandType cmd, const std::vector<u8>& payload);

    // Friend / block commands.
    u64 AddFriend(const std::string& npid);
    u64 RemoveFriend(const std::string& npid);
    u64 AddBlock(const std::string& npid);
    u64 RemoveBlock(const std::string& npid);
    u64 SetAppearOffline(bool enable);
    u64 LookupOnlineId(const std::string& npid);
    u64 GetAuthorizationCode(const std::string& client_id, const std::string& scope);

private:
    void ConnectThread();
    void ReaderThread();
    void WriterThread();
    bool DoConnect();
    void DoDisconnect();
    bool RecvN(u8* buf, u32 n);
    bool SendAll(const std::vector<u8>& data);
    std::vector<u8> BuildPacket(CommandType cmd, u64 id, const std::vector<u8>& payload) const;
    void DispatchPacket(PacketType type, u16 cmd_raw, u64 pkt_id, const std::vector<u8>& payload);
    void HandleLoginReply(const std::vector<u8>& payload);
    void HandleGetTokenReply(const std::vector<u8>& payload);
    void HandleServerFeaturesReply(const std::vector<u8>& payload);
    void HandleNotification(u16 cmd_raw, const std::vector<u8>& payload);
    bool RequestServerFeatures();

    static std::string ExtractBlob(const std::vector<u8>& p, s32 pos);

    static void PutLE16(std::vector<u8>& b, u64 off, u16 v);
    static void PutLE32(std::vector<u8>& b, u64 off, u32 v);
    static void PutLE64(std::vector<u8>& b, u64 off, u64 v);
    static u16 GetLE16(const u8* p);
    static u32 GetLE32(const u8* p);
    static u64 GetLE64(const u8* p);

    s32 m_sock = -1;
    std::string m_host;
    u16 m_port = 31313;
    std::string m_npid;
    std::string m_password;
    std::string m_token;
    bool m_appear_offline = false;

    std::atomic<bool> m_terminate{false};
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_authenticated{false};
    std::atomic<ShadNetState> m_state{ShadNetState::Ok};

    std::binary_semaphore m_sem_connected{0};
    std::binary_semaphore m_sem_authenticated{0};
    std::mutex m_mutex_connected;
    std::mutex m_mutex_authenticated;

    std::thread m_thread_connect;
    std::thread m_thread_reader;
    std::thread m_thread_writer;

    std::mutex m_mutex_send_direct;
    std::mutex m_mutex_send_queue;
    std::condition_variable m_cv_send_queue;
    std::vector<std::vector<u8>> m_send_queue;

    std::string m_avatar_url;
    u64 m_user_id = 0;
    mutable std::mutex m_mutex_bearer;
    std::string m_bearer_token;
    std::atomic<u32> m_addr_local{0};
    std::atomic<u32> m_addr_server{0};
    std::atomic<u32> m_server_protocol_version{0};
    std::atomic<bool> m_matching2_enabled{false};
    std::atomic<bool> m_server_features_received{false};

    mutable std::mutex m_mutex_friends;
    std::vector<FriendEntry> m_friends;

    std::atomic<u64> m_pkt_counter{1};
};

} // namespace ShadNet
