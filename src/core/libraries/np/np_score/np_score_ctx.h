// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>

#include "common/types.h"

namespace Libraries::Np::NpScore {

struct ScoreRequestCtx {
    std::mutex mutex;
    std::condition_variable cv;
    std::optional<s32> result;

    s32 titleCtxId = 0;
    s32 userId = -1;
    s32 pcId = 0;
    u32* tmpRankOut = nullptr;

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

    s32 Wait() {
        std::unique_lock lock(mutex);
        cv.wait(lock, [this] { return result.has_value(); });
        return *result;
    }
};

} // namespace Libraries::Np::NpScore
