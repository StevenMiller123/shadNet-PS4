// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string_view>
#include <vector>
#include <orbis/_types/net.h>

#include "common/recursive_mutex.h"
#include "common/types.h"
#include "core/libraries/network/net/net.h"
#include "core/libraries/np/np_matching2/np_matching2_internal.h"

namespace ShadNet {
struct NotifyRoomEvent;
}

namespace Libraries::Network {

// A class for emulating Sony's Matching2 servers with shadNet.
// This relies on socket, epoll, and ssl hooks to function.
class FakeMatching2 {
public:
    static FakeMatching2& Instance();

    void OnSocketCreate(s32 socket, const char* name, s32 type);
    void OnSocketClose(s32 socket);
    std::optional<s32> OnConnect(s32 socket, const OrbisNetSockaddr* addr, u32 addr_size);
    std::optional<s32> OnSend(s32 socket, const void* data, u64 size, s32 flags);
    std::optional<s32> OnRecv(s32 socket, void* buf, u64 size, s32 flags);

    std::optional<s32> OnEpollControl(s32 epoll, s32 op, s32 socket,
                                      const Net::OrbisNetEpollEvent* event);
    std::optional<s32> OnEpollWait(s32 epoll, Net::OrbisNetEpollEvent* events, s32 max_events,
                                   s32 timeout);
    void OnEpollDestroy(s32 epoll);

    void OnSslCreate(s32 ssl_id, s32 socket);
    void OnSslDelete(s32 ssl_id);
    std::optional<s32> OnSslConnect(s32 ssl_id);
    std::optional<s32> OnSslClose(s32 ssl_id);
    std::optional<s32> OnSslSend(s32 ssl_id, const void* data, u64 size);
    std::optional<s32> OnSslRecv(s32 ssl_id, void* buf, u64 size);
    std::optional<s32> OnSslCheckRecvPending(s32 ssl_id, u8* pending);

    // Stores CreateJoinRoom parameters for our requests to use.
    void OnCreateJoinRoom(std::shared_ptr<Np::NpMatching2::CreateJoinRoomParams> params);
    void CancelCreateJoinRoom(const Np::NpMatching2::CreateJoinRoomParams* params);

    // Forwards a shadNet room event to the room server connection for the room.
    void OnRoomEvent(s32 user_id, const ShadNet::NotifyRoomEvent& event);

private:
    struct RequestHeader;

    struct Connection {
        s32 socket = -1;
        u16 server_id = 0;
        s32 user_id = -1;
        u64 room_id = 0;
        u16 member_id = 0;
        std::map<u16, u16> members;
        std::vector<u8> sent;
        std::vector<u8> received;
    };

    using Handler = void (FakeMatching2::*)(Connection& connection, const RequestHeader& header,
                                            std::span<const u8> payload);

    struct OpcodeHandler {
        u16 opcode;
        std::string_view name;
        Handler handler;
    };

    static const OpcodeHandler* FindOpcode(u16 opcode);

    Connection* FindSslConnection(s32 ssl_id);
    void Send(Connection& connection, const void* data, u64 size);
    u64 Receive(Connection& connection, void* buf, u64 size);

    void HandleMessage(Connection& connection, std::span<const u8> message);
    void HandleRequest(Connection& connection, std::span<const u8> message);

    void QueueReply(Connection& connection, const RequestHeader& header, s32 error,
                    std::span<const u8> payload);
    template <typename T>
    void QueueReply(Connection& connection, const RequestHeader& header, s32 error,
                    const T& payload) {
        QueueReply(connection, header, error,
                   std::span<const u8>{reinterpret_cast<const u8*>(&payload), sizeof(payload)});
    }
    void QueueLateReply(s32 socket, const RequestHeader& header, s32 error,
                        std::span<const u8> payload);
    void QueueEvent(Connection& connection, u16 opcode, std::span<const u8> payload,
                    bool internal = false);

    // Opcode handlers
    void Connect(Connection& connection, const RequestHeader& header, std::span<const u8> payload);
    void SignIn(Connection& connection, const RequestHeader& header, std::span<const u8> payload);
    void RenewSession(Connection& connection, const RequestHeader& header,
                      std::span<const u8> payload);
    void GetWorldInfoList(Connection& connection, const RequestHeader& header,
                          std::span<const u8> payload);
    void SearchRoom(Connection& connection, const RequestHeader& header,
                    std::span<const u8> payload);
    void GetRoomDataExternalList(Connection& connection, const RequestHeader& header,
                                 std::span<const u8> payload);
    void GetRoomServer(Connection& connection, const RequestHeader& header,
                       std::span<const u8> payload);
    void RoomServerSignIn(Connection& connection, const RequestHeader& header,
                          std::span<const u8> payload);
    void CreateJoinRoom(Connection& connection, const RequestHeader& header,
                        std::span<const u8> payload);
    void JoinRoom(Connection& connection, const RequestHeader& header, std::span<const u8> payload);
    void JoinedRoom(Connection& connection, const RequestHeader& header,
                    std::span<const u8> payload);
    void PublishRoom(Connection& connection, const RequestHeader& header,
                     std::span<const u8> payload);
    void SetRoomDataExternal(Connection& connection, const RequestHeader& header,
                             std::span<const u8> payload);
    void SetRoomDataInternal(Connection& connection, const RequestHeader& header,
                             std::span<const u8> payload);
    void LeaveRoom(Connection& connection, const RequestHeader& header,
                   std::span<const u8> payload);
    void SendRoomMessage(Connection& connection, const RequestHeader& header,
                         std::span<const u8> payload);
    void ReleaseRoomServer(Connection& connection, const RequestHeader& header,
                           std::span<const u8> payload);

    u32 ReadyEvents(s32 socket) const;
    s32 CollectEvents(s32 epoll, Net::OrbisNetEpollEvent* events, s32 max_events) const;

    Common::RecursiveMutex m_mutex;
    std::condition_variable_any m_cv;
    std::set<s32> m_sockets;
    std::map<s32, Connection> m_connections;
    std::map<s32, std::map<s32, Net::OrbisNetEpollEvent>> m_epolls;
    std::map<s32, s32> m_ssl_sockets;
    std::deque<std::shared_ptr<Np::NpMatching2::CreateJoinRoomParams>> m_pending_creates;
    struct CreatedRoom {
        u64 room_id;
        std::vector<u8> reply;
        u16 member_id = 0;
        std::map<u16, u16> members;
    };
    std::map<u32, CreatedRoom> m_created_rooms;
};

} // namespace Libraries::Network
