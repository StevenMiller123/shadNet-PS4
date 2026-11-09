// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/NetCtl.h>

#include "common/logging/log.h"
#include "core/net_util/net_util.h"

namespace NetUtil {

const std::string& NetUtilInternal::GetIp() const {
    return ip;
}

bool NetUtilInternal::RetrieveIp() {
    std::scoped_lock lock{m_mutex};

    OrbisNetCtlInfo info{};
    const s32 ret = sceNetCtlGetInfo(ORBIS_NET_CTL_INFO_IP_ADDRESS, &info);
    if (ret < 0) {
        LOG_ERROR(Lib_Net, "sceNetCtlGetInfo failed with error {:#x}", static_cast<u32>(ret));
        return false;
    }
    ip = info.ip_address;

    return true;
}

u32 NetUtilInternal::GetExternalIp() const {
    return external_ip;
}

void NetUtilInternal::SetExternalIp(u32 addr) {
    external_ip = addr;
}

} // namespace NetUtil
