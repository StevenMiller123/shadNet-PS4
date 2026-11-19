// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <span>
#include <stop_token>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>
#include <orbis/_types/net.h>

#include "common/types.h"
#include "core/libraries/network/net/net.h"

namespace Libraries::Network {

// A class for bridging the PS4's forms of signaling with shadNet.
class FakeSignaling {
public:
    // A signaling message to deliver to a room member.
    struct Message {
        u16 src_member_id;
        u16 dst_member_id;
        std::vector<u8> data;
    };

    static FakeSignaling& Instance();

    // Tracks the room members shadNet reports, with the address they signal from.
    void OnMemberJoined(u64 room_id, u16 member_id, std::string_view addr, u16 port);
    void OnMemberLeft(u64 room_id, u16 member_id);
    void OnRoomLeft(u64 room_id);

    // Handles a signaling message the console sent to another room member.
    // Returns the messages to deliver back to the console.
    std::vector<Message> OnMessage(u64 room_id, u16 src_member_id, u16 dst_member_id,
                                   std::span<const u8> data);

    // Sends shadNet signaling from the console's P2P port
    s32 SendToShadNet(std::span<const u8> data, const OrbisNetSockaddrIn& to);

    void OnSocketCreate(s32 socket, const char* name);
    void OnSocketClose(s32 socket);

    // Craft signaling sends and responses
    std::optional<s32> OnSendTo(s32 socket, std::span<const u8> data, const OrbisNetSockaddr* to,
                                u32 to_size);
    std::optional<s32> OnRecvFrom(s32 socket, void* buf, u64 size, OrbisNetSockaddr* from,
                                  u32* from_size);

    void OnReceived(s32 socket, std::span<const u8> data, const OrbisNetSockaddr* from,
                    u32 from_size);

    // Emulates epoll behavior
    void OnEpollControl(s32 epoll, s32 op, s32 socket, const Net::OrbisNetEpollEvent* event);
    void OnEpollDestroy(s32 epoll);
    std::optional<s32> OnEpollWait(s32 epoll, Net::OrbisNetEpollEvent* events, s32 max_events,
                                   s32 timeout, const std::function<s32(s32 timeout)>& wait);

private:
    struct Peer {
        u32 addr = 0;
        u16 port = 0;
        u16 console_connection = 0;
        u16 peer_connection = 0;
        std::array<u8, 0x10> nonce{};
        u32 console_addr = 0;
        u16 console_port = 0;
    };

    struct Answer {
        std::vector<u8> data;
        OrbisNetSockaddrIn from;
    };

    const Peer* FindPeerLocked(const OrbisNetSockaddr* addr, u32 addr_size) const;
    u32 NextTransactionId();
    void LogPeerTrafficLocked(std::string_view direction, s32 socket, std::span<const u8> data,
                              const OrbisNetSockaddr* addr, u32 addr_size);
    s32 CollectAnswersLocked(s32 epoll, Net::OrbisNetEpollEvent* events, s32 max_events) const;

    // Answers shadNet signaling separate from the library's sockets
    s32 StartResponder();
    void StopResponder();
    void ResponderThread(std::stop_token stop, s32 socket);
    void HandleShadNetPacket(s32 socket, std::span<const u8> data, OrbisNetSockaddrIn from);

    std::mutex m_mutex;
    std::map<std::pair<u64, u16>, Peer> m_peers;
    u16 m_transaction = 0;

    s32 m_echo_socket = -1;
    std::map<s32, std::map<s32, Net::OrbisNetEpollEvent>> m_epolls;
    std::map<s32, std::deque<Answer>> m_answers;

    std::map<s32, u32> m_traffic_logged;

    std::mutex m_responder_mutex;
    s32 m_responder_socket = -1;
    std::jthread m_responder;
};

} // namespace Libraries::Network
