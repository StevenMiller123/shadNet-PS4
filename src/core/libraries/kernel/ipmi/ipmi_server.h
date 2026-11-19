// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <span>
#include <string_view>
#include <vector>

#include "common/types.h"
#include "core/libraries/kernel/kernel_ipmi.h"

namespace Libraries::Kernel::Ipmi {

// Base class for our emulated IPMI servers.
class Server {
public:
    static constexpr u32 EventFlagCount = 32;
    static constexpr u32 MessageQueueCount = 32;

    Server(std::string name, void* user_data);
    virtual ~Server() = default;

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    const std::string& GetName() const {
        return m_name;
    }

    // Called when emulating a client that initialized before us.
    virtual void OnAdopted(u32 kid) {}

    virtual s32 Connect(const void* user_data, u64 user_data_size, s32* status);
    virtual s32 Disconnect(s32* status);
    virtual s32 TerminateConnection();
    virtual s32 InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                                 std::span<IpmiBufferInfo> out, s32* method_result);
    virtual s32 InvokeAsyncMethod(u32 method, std::span<const IpmiDataInfo> in, s32* method_result);

    s32 TryGetResult(u32 method, s32* method_result, std::span<IpmiBufferInfo> out);
    s32 GetMessage(u32 queue, void* message, u64 max_size, u64* size, u32* timeout);
    s32 TryGetMessage(u32 queue, void* message, u64 max_size, u64* size);
    s32 WaitEventFlag(u32 index, u64 pattern, u32 mode, u64* result_pattern, u32* timeout);
    s32 PollEventFlag(u32 index, u64 pattern, u32 mode, u64* result_pattern);
    s32 GetUserData(void** user_data);

protected:
    void SetEventFlag(u32 index, u64 pattern);
    void PushMessage(u32 queue, std::vector<u8> message);
    void PushAsyncResult(u32 method, s32 method_result, std::vector<std::vector<u8>> out);
    static s32 WriteOutput(IpmiBufferInfo& out, const void* data, u64 size);

private:
    struct AsyncResult {
        u32 method;
        s32 method_result;
        std::vector<std::vector<u8>> out;
    };

    bool TryConsumeEventFlag(u32 index, u64 pattern, u32 mode, u64* result_pattern);

    std::string m_name;
    void* m_user_data;

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::array<u64, EventFlagCount> m_event_flags{};
    std::array<std::deque<std::vector<u8>>, MessageQueueCount> m_message_queues;
    std::deque<AsyncResult> m_async_results;
};

// Returns a Server object if there is one for the inputted name. Otherwise returns nullptr.
std::shared_ptr<Server> CreateServer(std::string_view name, void* user_data);

} // namespace Libraries::Kernel::Ipmi
