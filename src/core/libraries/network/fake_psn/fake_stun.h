// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <map>
#include <optional>
#include <span>
#include <string_view>
#include <vector>
#include <orbis/_types/net.h>

#include "common/recursive_mutex.h"
#include "common/types.h"

namespace Libraries::Network {

// A class for emulating Sony's STUN server (<country>.np.stun.playstation.net) with shadNet.
class FakeStun {
public:
    static FakeStun& Instance();
    static bool IsStunHost(std::string_view host);
    u32 GetServerAddr();

    std::optional<s32> OnSendTo(s32 socket, const void* data, u64 size, const OrbisNetSockaddr* to,
                                u32 to_size);
    s32 OnRecvFrom(s32 socket, void* buf, u64 capacity, s32 size, OrbisNetSockaddr* from,
                   u32* from_size);

    void OnSocketClose(s32 socket);

    // Handles shadNet's answer to a STUN ping, returns false if it isn't one.
    bool OnShadNetEcho(std::span<const u8> data, const OrbisNetSockaddrIn& from);

private:
    // Sony's STUN port, and the port responses asking for a port change come from.
    static constexpr u16 StunPort = 3478;
    static constexpr u16 ChangedPort = 3479;

    using TransactionId = std::array<u8, 16>;

    struct PendingRequest {
        TransactionId transaction_id;
        u16 source_port;
        std::vector<u8> sony_attribute;
        bool forwarded;
    };

    bool ResolveServers();
    s32 LoopbackSocket();
    static bool PassesNat(u32 change_flags);

    Common::RecursiveMutex m_mutex;
    bool m_resolved = false;
    u32 m_server_addr = 0;
    u32 m_echo_addr = 0;
    u16 m_echo_port = 0;
    std::map<s32, PendingRequest> m_pending;
    s32 m_loopback_socket = -1;
    u16 m_loopback_port = 0;
};

} // namespace Libraries::Network
