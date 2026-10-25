// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <pthread.h>

namespace Common {
class RecursiveMutex {
public:
    RecursiveMutex() {
        pthread_mutexattr_t mtx_attr{};
        pthread_mutexattr_init(&mtx_attr);
        pthread_mutexattr_settype(&mtx_attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mutex, &mtx_attr);
        pthread_mutexattr_destroy(&mtx_attr);
    }
    ~RecursiveMutex() {
        pthread_mutex_destroy(&mutex);
    }
    RecursiveMutex(const RecursiveMutex&) = delete;
    RecursiveMutex& operator=(const RecursiveMutex&) = delete;

    void lock() {
        pthread_mutex_lock(&mutex);
    }
    void unlock() {
        pthread_mutex_unlock(&mutex);
    }
    [[nodiscard]] bool try_lock() {
        return pthread_mutex_trylock(&mutex) == 0;
    }

private:
    pthread_mutex_t mutex{};
};
} // namespace Common