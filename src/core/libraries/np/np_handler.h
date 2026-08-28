// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>

namespace Libraries::Np {

class NpHandler {
public:
    static NpHandler& GetInstance();

    NpHandler(const NpHandler&) = delete;
    NpHandler& operator=(const NpHandler&) = delete;

    void Initialize();
private:
    NpHandler() = default;
    ~NpHandler() = default;

    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_worker_running{false};
};

} // namespace Libraries::Np