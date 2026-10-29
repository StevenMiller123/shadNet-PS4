// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <chrono>
#include <deque>
#include <mutex>
#include <span>
#include <vector>

#include "common/types.h"

namespace NetUtil {

class P2PTransport {
public:
    enum Channel : u64 { Signaling = 0, Control = 1, Matching2 = 2, ChannelCount };

    static P2PTransport& Instance();

    P2PTransport(const P2PTransport&) = delete;
    P2PTransport& operator=(const P2PTransport&) = delete;

    void Configure(u16 udp_port, u32 advertised_addr);
    u16 Start(u16 udp_port);
    void Stop();

    bool Ensure();
    bool IsReady();
    u16 GetConfiguredPort();
    u16 GetBoundPort();
    u32 GetAdvertisedAddr();
    u16 GetAdvertisedPort();

    s32 SendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port);
    s32 RecvFrom(Channel channel, void* buf, u32 len, u32* from_addr, u16* from_port);

private:
    struct Packet {
        std::vector<u8> data;
        u32 from_addr = 0;
        u16 from_port = 0;
    };

    P2PTransport() = default;
    ~P2PTransport();

    static Channel Classify(std::span<const u8> packet);

    u16 StartLocked(u16 udp_port, bool allow_fallback);
    void StopLocked();
    s32 BindSocket(u16 udp_port);
    bool RunningLocked();
    void DrainLocked();
    u16 PortToBind() const;

    std::mutex m_mutex;
    s32 m_socket = -1;
    u16 m_bound_port = 0;
    u16 m_configured_port = 0;
    u32 m_advertised_addr = 0;
    std::chrono::steady_clock::time_point m_retry_after{};
    std::array<std::deque<Packet>, ChannelCount> m_queues{};
    std::array<u64, ChannelCount> m_queue_bytes{};
};

void ConfigureP2P(u16 udp_port, u32 advertised_addr);
u16 StartP2P(u16 udp_port);
void StopP2P();

bool EnsureP2PTransport();
bool P2PTransportIsReady();
u16 GetP2PConfiguredPort();
u16 GetP2PBoundPort();
u32 GetP2PAdvertisedAddr();
u16 GetP2PAdvertisedPort();
s32 P2PSignalingSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port);
s32 P2PSignalingRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port);
s32 P2PControlSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port);
s32 P2PControlRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port);
s32 P2PMatching2SendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port);
s32 P2PMatching2RecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port);

} // namespace NetUtil
