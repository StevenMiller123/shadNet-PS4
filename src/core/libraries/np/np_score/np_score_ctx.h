// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "common/types.h"
#include "core/libraries/np/np_error.h"

namespace Libraries::Np::NpScore {

struct ScoreRequestCtx {
    std::mutex mutex;
    std::condition_variable cv;
    std::optional<s32> result;

    s32 title_ctx_id = 0;
    s32 user_id = -1;
    s32 pc_id = 0;
    u32* tmp_rank_out = nullptr;
    u64 game_data_recv_offset = 0;
    std::vector<u8> game_data_recv_cache;
    std::string game_data_recv_key;
    std::vector<u8> game_data_send_buf;
    u64 game_data_send_total = 0;

    bool BeginOp() {
        std::lock_guard lock(mutex);
        if (result.has_value() && *result == ORBIS_NP_COMMUNITY_ERROR_ABORTED) {
            return false;
        }
        result.reset();
        return true;
    }

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
