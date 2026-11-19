// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <mutex>
#include <optional>

#include "common/types.h"

namespace Libraries::Kernel::Ipmi::NpService {

// A generic class for handling the request logic frequently used by the PS4 firmware.
class RequestTable {
public:
    enum class State : u32 {
        Dormant = 0,
        Created = 1,
        Started = 2,
        Aborting = 3,
        Finished = 4,
    };

    // Requests are signaled through an event flag, resulting in a max request count of 64.
    static constexpr u64 MaxOutstanding = 64;

    // The event flag bit a finished request's client waits on.
    struct Signal {
        u32 index;
        u64 pattern;
    };

    // Takes a free slot for a request with the given id.
    s32 Create(s32 request_id);
    // Records the IPMI slot the client waits on, the request must be newly created.
    s32 Start(s32 request_id, s32 ipc_slot);
    // Marks a started request as being aborted. Created requests can't be aborted.
    s32 Abort(s32 request_id);
    // Frees the request's slot.
    s32 Destroy(s32 request_id);
    // Marks a started or aborting request finished, returning the event flag bit to signal.
    s32 Finish(s32 request_id, Signal* signal);

    std::optional<State> GetState(s32 request_id);

private:
    static constexpr s32 InvalidId = -1;

    struct Request {
        State state = State::Dormant;
        s32 id = InvalidId;
        s32 ipc_slot = InvalidId;
    };

    Request* Find(s32 request_id);

    std::mutex m_mutex;
    std::array<Request, MaxOutstanding> m_requests{};
};

} // namespace Libraries::Kernel::Ipmi::NpService
