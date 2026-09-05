// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <pthread_np.h>
#include "fmt/format.h"
#include "orbis/SysUtil.h"
#include "orbis/libkernel.h"

template <typename... Args>
std::string FormatLog(char const* format, Args const&... args) {
    std::string message;
    if constexpr (sizeof...(args) == 0) {
        message = format;
    } else {
        message = fmt::vformat(format, fmt::make_format_args(args...));
    }
    return message;
}

template <typename... Args>
void PrintLog(char const* lib, char const* log_level, char const* file, unsigned int line_num, char const* function,
              char const* format, Args const&... args) {
    std::string message = FormatLog(format, args...);
    char thr_name[256];
    std::memset(thr_name, 0, sizeof(thr_name));
    std::string file_name{file};
    size_t last_separator = file_name.find_last_of('/');
    if (last_separator != std::string::npos && last_separator < file_name.size() - 1) {
        file_name = file_name.substr(last_separator + 1);
    }
    std::string full_log;
    if (pthread_getname_np(pthread_self(), thr_name) == 0) {
        full_log = fmt::format("[{}] <{}> ({}) {}:{} {}: {}\n", lib, log_level, thr_name,
                               file_name, line_num, function, message);
    } else {
        full_log = fmt::format("[{}] <{}> {}:{} {}: {}\n", lib, log_level, file_name, line_num,
                               function, message);
    }
    sceKernelDebugOutText(0, full_log.c_str());
}

template <typename... Args>
void PrintLogN(char const* lib, char const* log_level, char const* file, unsigned int line_num, char const* function,
               char const* format, Args const&... args) {
    PrintLog(lib, log_level, file, line_num, function, format, args...);
    std::string message = FormatLog(format, args...);
    sceSysUtilSendSystemNotificationWithText(222, message.c_str());
}

template <typename... Args>
void PrintLogR(char const* format, Args const&... args) {
    std::string message = FormatLog(format, args...);
    sceKernelDebugOutText(0, message.c_str());
}

#define LOG_TRACE(lib, ...) PrintLog(#lib, "Trace", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_DEBUG(lib, ...) PrintLog(#lib, "Debug", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_INFO(lib, ...) PrintLog(#lib, "Info", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARNING(lib, ...) PrintLog(#lib, "Warning", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(lib, ...) PrintLog(#lib, "Error", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_CRITICAL(lib, ...) PrintLog(#lib, "Critical", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_NOTIFICATION(lib, ...) PrintLogN(#lib, "Info", __FILE__, __LINE__, __func__, __VA_ARGS__)