// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <mutex>
#include <unistd.h>

#include "common/logging/log.h"
#include "common/types.h"

Logger& Logger::GetInstance() {
    static Logger s_instance;
    return s_instance;
}

// /data/shadnet/CUSAXXXXX.log
static const char* log_fmt = "/data/shadnet/%s";
static constexpr u64 MAX_LOG_FILE_LENGTH = 28;

static std::mutex mtx{};
static std::ofstream file{};
static bool is_init = false;

void Logger::Initialize(std::string& serial) {
    std::scoped_lock lk{mtx};
    if (is_init) {
        file.flush();
        file.close();
    } else {
        is_init = true;
    }

    // Filename is some form of CUSAXXXXX.log
    char filename_buf[14]{};
    snprintf(filename_buf, sizeof(filename_buf), "%s%s", serial.c_str(), ".log");

    // Create the actual file stream
    char log_path[MAX_LOG_FILE_LENGTH]{};
    snprintf(log_path, MAX_LOG_FILE_LENGTH, log_fmt, filename_buf);
    file = std::ofstream(log_path);
    file.clear();
}

void Logger::Log(const char* log_string) {
    std::scoped_lock lk{mtx};
    if (!is_init) {
        return;
    }

    // LOG_ macros append a newline, so we don't need to add any here.
    file << log_string;
    file.flush();
}

void TtyLogThread(int fd, char const* name) {
    pthread_set_name_np(pthread_self(), name);
    LOG_DEBUG(shadNet, "Tty log thread started");
    std::string line{};
    char buf{};
    while (read(fd, &buf, sizeof buf)) {
        if (buf == '\n') {
            LOG_INFO(Tty, "{}", line);
            line = "";
            continue;
        }
        line += buf;
    }
    if (line.size() > 1) {
        LOG_DEBUG(Tty, "{}", line);
    }
    LOG_DEBUG(shadNet, "Tty log thread exited");
}
