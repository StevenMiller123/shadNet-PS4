// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <string>

#include "common/types.h"

namespace NetUtil {

class NetUtilInternal {
public:
    explicit NetUtilInternal() = default;
    ~NetUtilInternal() = default;

private:
    std::string ip{};
    u32 external_ip{0};
    std::mutex m_mutex;

public:
    const std::string& GetIp() const;
    bool RetrieveIp();

    u32 GetExternalIp() const;
    void SetExternalIp(u32 addr);
};

} // namespace NetUtil