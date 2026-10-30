// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>

#include "common/types.h"
#include "core/libraries/np/np_types.h"

namespace Libraries::Np::NpUtility {

struct LookupTimeouts {
    s32 resolve_retry = 0;
    u32 resolve_timeout = 0;
    u32 conn_timeout = 0;
    u32 send_timeout = 0;
    u32 recv_timeout = 0;
};

struct LookupRequestCtx {
    std::mutex mutex;
    std::condition_variable cv;
    std::optional<s32> result;

    s32 title_ctx_id = 0;
    bool is_async = false;
    bool used = false;
    bool aborted = false;
    std::optional<LookupTimeouts> timeouts;

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

struct WordFilterTitleCtx {
    s32 user_id = -1;
    OrbisNpId self_np_id{};
    std::optional<LookupTimeouts> timeouts;
};

using WordFilterRequestCtx = LookupRequestCtx;

} // namespace Libraries::Np::NpUtility
