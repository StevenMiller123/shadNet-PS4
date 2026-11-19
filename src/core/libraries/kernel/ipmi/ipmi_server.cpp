// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <orbis/libkernel.h>

#include "common/logging/log.h"
#include "core/libraries/kernel/ipmi/np_mgr_ipc.h"
#include "core/libraries/kernel/ipmi/np_service_server.h"

namespace Libraries::Kernel::Ipmi {

Server::Server(std::string name, void* user_data)
    : m_name(std::move(name)), m_user_data(user_data) {}

s32 Server::Connect(const void* user_data, u64 user_data_size, s32* status) {
    LOG_INFO(Kernel_Ipmi, "{} connect, user data size {:#x}", m_name, user_data_size);
    if (status != nullptr) {
        *status = 0;
    }
    return 0;
}

s32 Server::Disconnect(s32* status) {
    LOG_INFO(Kernel_Ipmi, "{} disconnect", m_name);
    if (status != nullptr) {
        *status = 0;
    }
    return 0;
}

s32 Server::TerminateConnection() {
    LOG_INFO(Kernel_Ipmi, "{} terminate connection", m_name);
    return 0;
}

s32 Server::InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                             std::span<IpmiBufferInfo> out, s32* method_result) {
    LOG_ERROR(Kernel_Ipmi, "{} sync method {:#x} not implemented ({} in, {} out)", m_name, method,
              in.size(), out.size());
    if (method_result != nullptr) {
        *method_result = ORBIS_KERNEL_ERROR_ENOSYS;
    }
    return 0;
}

s32 Server::InvokeAsyncMethod(u32 method, std::span<const IpmiDataInfo> in, s32* method_result) {
    LOG_ERROR(Kernel_Ipmi, "{} async method {:#x} not implemented ({} in)", m_name, method,
              in.size());
    PushAsyncResult(method, ORBIS_KERNEL_ERROR_ENOSYS, {});
    if (method_result != nullptr) {
        *method_result = 0;
    }
    return 0;
}

s32 Server::TryGetResult(u32 method, s32* method_result, std::span<IpmiBufferInfo> out) {
    AsyncResult result;
    {
        std::scoped_lock lk{m_mutex};
        auto it = m_async_results.begin();
        while (it != m_async_results.end() && it->method != method) {
            ++it;
        }
        if (it == m_async_results.end()) {
            return ORBIS_KERNEL_ERROR_EAGAIN;
        }
        result = std::move(*it);
        m_async_results.erase(it);
    }

    if (method_result != nullptr) {
        *method_result = result.method_result;
    }
    if (result.out.size() > out.size()) {
        LOG_ERROR(Kernel_Ipmi, "{} async method {:#x} has {} outputs, caller expects {}", m_name,
                  method, result.out.size(), out.size());
    }
    for (u64 i = 0; i < result.out.size() && i < out.size(); ++i) {
        if (const s32 ret = WriteOutput(out[i], result.out[i].data(), result.out[i].size());
            ret != 0) {
            return ret;
        }
    }
    return 0;
}

s32 Server::GetMessage(u32 queue, void* message, u64 max_size, u64* size, u32* timeout) {
    if (queue >= MessageQueueCount) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    std::unique_lock lk{m_mutex};
    auto& messages = m_message_queues[queue];
    const auto ready = [&] { return !messages.empty(); };
    if (timeout == nullptr) {
        m_cv.wait(lk, ready);
    } else {
        const auto start = std::chrono::steady_clock::now();
        const auto limit = std::chrono::microseconds{*timeout};
        const bool got = m_cv.wait_for(lk, limit, ready);
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start);
        *timeout = elapsed >= limit ? 0 : static_cast<u32>((limit - elapsed).count());
        if (!got) {
            return ORBIS_KERNEL_ERROR_ETIMEDOUT;
        }
    }

    auto& front = messages.front();
    if (front.size() > max_size) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    std::memcpy(message, front.data(), front.size());
    *size = front.size();
    messages.pop_front();
    return 0;
}

s32 Server::TryGetMessage(u32 queue, void* message, u64 max_size, u64* size) {
    if (queue >= MessageQueueCount) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    std::scoped_lock lk{m_mutex};
    auto& messages = m_message_queues[queue];
    if (messages.empty()) {
        return ORBIS_KERNEL_ERROR_EAGAIN;
    }
    auto& front = messages.front();
    if (front.size() > max_size) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    std::memcpy(message, front.data(), front.size());
    *size = front.size();
    messages.pop_front();
    return 0;
}

bool Server::TryConsumeEventFlag(u32 index, u64 pattern, u32 mode, u64* result_pattern) {
    u64& bits = m_event_flags[index];
    const bool matched =
        (mode & IpmiEventFlagWaitAnd) != 0 ? (bits & pattern) == pattern : (bits & pattern) != 0;
    if (!matched) {
        return false;
    }
    if (result_pattern != nullptr) {
        *result_pattern = bits;
    }
    if ((mode & IpmiEventFlagClearAll) != 0) {
        bits = 0;
    } else if ((mode & IpmiEventFlagClearPattern) != 0) {
        bits &= ~pattern;
    }
    return true;
}

s32 Server::WaitEventFlag(u32 index, u64 pattern, u32 mode, u64* result_pattern, u32* timeout) {
    if (index >= EventFlagCount || pattern == 0) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    std::unique_lock lk{m_mutex};
    const auto ready = [&] { return TryConsumeEventFlag(index, pattern, mode, result_pattern); };
    if (timeout == nullptr) {
        m_cv.wait(lk, ready);
        return 0;
    }

    const auto start = std::chrono::steady_clock::now();
    const auto limit = std::chrono::microseconds{*timeout};
    const bool got = m_cv.wait_for(lk, limit, ready);
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start);
    *timeout = elapsed >= limit ? 0 : static_cast<u32>((limit - elapsed).count());
    if (!got) {
        if (result_pattern != nullptr) {
            *result_pattern = m_event_flags[index];
        }
        return ORBIS_KERNEL_ERROR_ETIMEDOUT;
    }
    return 0;
}

s32 Server::PollEventFlag(u32 index, u64 pattern, u32 mode, u64* result_pattern) {
    if (index >= EventFlagCount || pattern == 0) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }

    std::scoped_lock lk{m_mutex};
    if (!TryConsumeEventFlag(index, pattern, mode, result_pattern)) {
        if (result_pattern != nullptr) {
            *result_pattern = m_event_flags[index];
        }
        return ORBIS_KERNEL_ERROR_EBUSY;
    }
    return 0;
}

s32 Server::GetUserData(void** user_data) {
    if (user_data == nullptr) {
        return ORBIS_KERNEL_ERROR_EFAULT;
    }
    *user_data = m_user_data;
    return 0;
}

void Server::SetEventFlag(u32 index, u64 pattern) {
    if (index >= EventFlagCount) {
        LOG_ERROR(Kernel_Ipmi, "{} event flag {} out of range", m_name, index);
        return;
    }
    {
        std::scoped_lock lk{m_mutex};
        m_event_flags[index] |= pattern;
    }
    m_cv.notify_all();
}

void Server::PushMessage(u32 queue, std::vector<u8> message) {
    if (queue >= MessageQueueCount) {
        LOG_ERROR(Kernel_Ipmi, "{} message queue {} out of range", m_name, queue);
        return;
    }
    {
        std::scoped_lock lk{m_mutex};
        m_message_queues[queue].push_back(std::move(message));
    }
    m_cv.notify_all();
}

void Server::PushAsyncResult(u32 method, s32 method_result, std::vector<std::vector<u8>> out) {
    {
        std::scoped_lock lk{m_mutex};
        m_async_results.push_back({method, method_result, std::move(out)});
    }
    m_cv.notify_all();
}

s32 Server::WriteOutput(IpmiBufferInfo& out, const void* data, u64 size) {
    if (size > out.capacity) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (size != 0) {
        std::memcpy(out.data, data, size);
    }
    out.size = size;
    return 0;
}

std::shared_ptr<Server> CreateServer(std::string_view name, void* user_data) {
    if (name == NpMgrIpcServer::ServiceName) {
        return std::make_shared<NpMgrIpcServer>(user_data);
    }
    if (name == NpServiceServer::ServiceName) {
        return std::make_shared<NpServiceServer>(user_data);
    }
    return nullptr;
}

} // namespace Libraries::Kernel::Ipmi
