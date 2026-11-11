// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include "common/types.h"

#include <miniupnpc.h>

namespace NetUtil {

class UPnPClient {
public:
    static UPnPClient& Instance();

    void Start();
    void SetP2PFeaturesEnabled(bool enabled);
    bool WaitReady(int timeout_ms);

    bool IsAvailable() const;
    u32 GetExternalIp() const;
    std::string GetExternalIpString() const;
    u16 GetExternalPort() const;

    void AddMapping(u16 port);
    void RemoveMapping(u16 port);
    void MapPortAsync(u16 port);
    void UnmapPort(u16 port);

private:
    UPnPClient() = default;
    ~UPnPClient();
    UPnPClient(const UPnPClient&) = delete;
    UPnPClient& operator=(const UPnPClient&) = delete;

    void DiscoverThread();

    std::atomic<bool> m_started{false};
    std::atomic<bool> m_p2p_features_enabled{true};
    std::atomic<bool> m_done{false};
    std::atomic<bool> m_available{false};
    std::atomic<u32> m_external_ip_nbo{0};
    std::atomic<u16> m_external_port{0};
    std::atomic<u16> m_mapped_port{0};
    char m_external_ip_str[16]{};

    char m_lan_addr[64]{};
    UPNPUrls m_urls{};
    IGDdatas m_data{};

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::thread m_thread;
    std::mutex m_map_mutex;
    std::thread m_map_thread;
};

} // namespace NetUtil
