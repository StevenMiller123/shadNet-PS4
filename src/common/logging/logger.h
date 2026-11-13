// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

class Logger {
public:
    static Logger& GetInstance();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void Initialize(std::string& serial);
    void Log(const char* log_string);

private:
    Logger() = default;
    ~Logger() = default;
};
