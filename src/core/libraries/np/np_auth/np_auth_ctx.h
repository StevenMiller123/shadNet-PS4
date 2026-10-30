// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>

#include "core/libraries/np/np_types.h"

namespace Libraries::Np::NpAuth {

struct AuthRequestCtx {
    std::mutex mutex;
    std::condition_variable cv;
    std::optional<s32> result;

    OrbisNpAuthorizationCode* auth_code = nullptr;
    s32* issuer_id = nullptr;

    void SetResult(s32 r) {
        {
            std::lock_guard lock(mutex);
            if (result.has_value()) {
                return;
            }
            result = r;
        }
        cv.notify_all();
    }
    void SetAuthorizationCode(const std::string& code, s32 issuer) {
        {
            std::lock_guard lock(mutex);
            if (result.has_value()) {
                return;
            }
            std::memset(auth_code, 0, sizeof(OrbisNpAuthorizationCode));
            std::memcpy(auth_code->code, code.data(), code.size());
            if (issuer_id) {
                *issuer_id = issuer;
            }
            result = ORBIS_OK;
        }
        cv.notify_all();
    }

    bool HasResult() {
        std::lock_guard lock(mutex);
        return result.has_value();
    }

    s32 Wait() {
        std::unique_lock lock(mutex);
        cv.wait(lock, [this] { return result.has_value(); });
        return *result;
    }
};

} // namespace Libraries::Np::NpAuth
