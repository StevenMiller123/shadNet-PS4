// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <cstring>
#include <upnperrors.h>
#include "common/logging/log.h"
#include "core/net_util/net_upnp.h"

#include <arpa/inet.h>
#include <upnpcommands.h>

namespace NetUtil {

UPnPClient& UPnPClient::Instance() {
    static UPnPClient instance;
    return instance;
}

UPnPClient::~UPnPClient() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
    {
        std::scoped_lock lock{m_map_mutex};
        m_map_stop = true;
    }
    m_map_cv.notify_all();
    if (m_map_thread.joinable()) {
        m_map_thread.join();
    }
    for (const u16 port : m_mapped_ports) {
        RemoveMapping(port);
    }
    m_mapped_ports.clear();
    if (m_available.load()) {
        FreeUPNPUrls(&m_urls);
    }
}

void UPnPClient::Start() {
    if (!m_p2p_features_enabled.load()) {
        LOG_INFO(Lib_Net, "UPnP skipped because Matching2 is disabled");
        return;
    }

    bool expected = false;
    if (!m_started.compare_exchange_strong(expected, true)) {
        return;
    }
    LOG_INFO(Lib_Net, "starting UPnP discovery thread");
    m_thread = std::thread(&UPnPClient::DiscoverThread, this);
}

void UPnPClient::SetP2PFeaturesEnabled(bool enabled) {
    m_p2p_features_enabled.store(enabled);
}

bool UPnPClient::WaitReady(int timeout_ms) {
    std::unique_lock lock(m_mutex);
    m_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return m_done.load(); });
    return m_available.load();
}

bool UPnPClient::IsAvailable() const {
    return m_available.load();
}

u32 UPnPClient::GetExternalIp() const {
    return m_external_ip_nbo.load();
}

std::string UPnPClient::GetExternalIpString() const {
    return m_available.load() ? std::string(m_external_ip_str) : std::string{};
}

u16 UPnPClient::GetExternalPort() const {
    return m_external_port.load();
}

void UPnPClient::DiscoverThread() {
    int error = 0;
    UPNPDev* devlist = upnpDiscover(2000, nullptr, nullptr, UPNP_LOCAL_PORT_ANY, 0, 2, &error);

    if (!devlist) {
        LOG_INFO(Lib_Net, "UPnP: no IGD found (error={})", error);
        m_done = true;
        m_cv.notify_all();
        return;
    }

    char wan_addr[40]{};
    const int ret = UPNP_GetValidIGD(devlist, &m_urls, &m_data, m_lan_addr, sizeof(m_lan_addr),
                                     wan_addr, sizeof(wan_addr));
    freeUPNPDevlist(devlist);

    if (ret != 1) {
        LOG_INFO(Lib_Net, "UPnP: no valid IGD (ret={})", ret);
        m_done = true;
        m_cv.notify_all();
        return;
    }

    char external_ip[40]{};
    if (UPNP_GetExternalIPAddress(m_urls.controlURL, m_data.first.servicetype, external_ip) !=
        UPNPCOMMAND_SUCCESS) {
        LOG_WARNING(Lib_Net, "UPnP: IGD found but failed to get external IP");
        FreeUPNPUrls(&m_urls);
        m_done = true;
        m_cv.notify_all();
        return;
    }

    u32 ip_nbo = 0;
    inet_pton(AF_INET, external_ip, &ip_nbo);
    m_external_ip_nbo.store(ip_nbo);
    std::strncpy(m_external_ip_str, external_ip, sizeof(m_external_ip_str) - 1);
    m_available = true;

    LOG_INFO(Lib_Net, "UPnP: IGD found - external IP={} lan={}", external_ip, m_lan_addr);

    m_done = true;
    m_cv.notify_all();
}

bool UPnPClient::AddMapping(u16 port) {
    if (!m_p2p_features_enabled.load()) {
        return false;
    }

    if (!m_available.load()) {
        return false;
    }

    const std::string port_str = std::to_string(port);
    const s32 ret =
        UPNP_AddPortMapping(m_urls.controlURL, m_data.first.servicetype, port_str.c_str(),
                            port_str.c_str(), m_lan_addr, "shadPS4", "UDP", nullptr, "0");

    m_external_port.store(port);
    if (ret == UPNPCOMMAND_SUCCESS) {
        LOG_INFO(Lib_Net, "UPnP: mapped UDP port {}", port);
        return true;
    }
    LOG_WARNING(Lib_Net, "UPnP: failed to map port {} ({})", port, strupnperror(ret));
    return false;
}

void UPnPClient::MapPortAsync(u16 port) {
    if (port == 0) {
        return;
    }
    {
        std::scoped_lock lock{m_map_mutex};
        if (m_map_stop || m_port_refs[port]++ != 0) {
            return;
        }
        m_map_queue.emplace_back(port, true);
        if (!m_map_thread.joinable()) {
            m_map_thread = std::thread(&UPnPClient::MapThread, this);
        }
    }
    m_map_cv.notify_one();
}

void UPnPClient::UnmapPort(u16 port) {
    {
        std::scoped_lock lock{m_map_mutex};
        const auto it = m_port_refs.find(port);
        if (it == m_port_refs.end()) {
            return;
        }
        if (--it->second != 0) {
            return;
        }
        m_port_refs.erase(it);
        m_map_queue.emplace_back(port, false);
    }
    m_map_cv.notify_one();
}

void UPnPClient::MapThread() {
    for (;;) {
        std::pair<u16, bool> op;
        {
            std::unique_lock lock{m_map_mutex};
            m_map_cv.wait(lock, [this] { return m_map_stop || !m_map_queue.empty(); });
            if (m_map_stop) {
                return;
            }
            op = m_map_queue.front();
            m_map_queue.pop_front();
        }
        const auto [port, add] = op;
        if (!add) {
            if (m_mapped_ports.erase(port) != 0) {
                RemoveMapping(port);
            }
            continue;
        }
        if (m_mapped_ports.contains(port)) {
            continue;
        }
        if (!WaitReady(10000)) {
            LOG_INFO(Lib_Net, "UPnP: no IGD, UDP port {} is not forwarded", port);
            continue;
        }
        if (AddMapping(port)) {
            m_mapped_ports.insert(port);
        }
    }
}

void UPnPClient::RemoveMapping(u16 port) {
    if (!m_available.load()) {
        return;
    }

    const std::string port_str = std::to_string(port);
    UPNP_DeletePortMapping(m_urls.controlURL, m_data.first.servicetype, port_str.c_str(), "UDP",
                           nullptr);

    LOG_INFO(Lib_Net, "UPnP: unmapped UDP port {}", port);
}

} // namespace NetUtil
