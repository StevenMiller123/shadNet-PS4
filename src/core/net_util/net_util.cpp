// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <arpa/inet.h>
#include <orbis/Net.h>
#include <sys/socket.h>
#include <unistd.h>

#include "core/net_util/net_util.h"

namespace NetUtil {

const std::string& NetUtilInternal::GetIp() const {
    return ip;
}

bool NetUtilInternal::RetrieveIp() {
    std::scoped_lock lock{m_mutex};

    auto sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        return false;
    }

    sockaddr_in sa{};
    socklen_t sa_len{sizeof(sa)};
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr("1.1.1.1");
    sa.sin_port = htons(80);

    if (connect(sockfd, (sockaddr*)&sa, sa_len) == -1) {
        close(sockfd);
        return false;
    }

    if (getsockname(sockfd, (struct sockaddr*)&sa, &sa_len) == -1) {
        close(sockfd);
        return false;
    }

    char netmaskStr[INET_ADDRSTRLEN];
    sceNetInetNtop(AF_INET, &sa.sin_addr, netmaskStr, INET_ADDRSTRLEN);
    ip = netmaskStr;

    close(sockfd);

    return true;
}

u32 NetUtilInternal::GetExternalIp() const {
    return external_ip;
}

void NetUtilInternal::SetExternalIp(u32 addr) {
    external_ip = addr;
}

} // namespace NetUtil