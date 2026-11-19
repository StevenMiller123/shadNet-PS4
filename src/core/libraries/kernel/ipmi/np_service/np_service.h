// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <functional>
#include <span>
#include <string>
#include <string_view>

#include "common/types.h"
#include "core/libraries/kernel/ipmi/np_service/request_table.h"
#include "core/libraries/kernel/kernel_ipmi.h"

namespace Libraries::Kernel::Ipmi::NpService {

// Base class for the services the SceNpService IPMI server hosts
class Service {
public:
    Service(std::string_view name, u32 id, u32 first_method, u32 last_method);
    virtual ~Service() = default;

    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;

    const std::string& GetName() const {
        return m_name;
    }
    u32 GetId() const {
        return m_id;
    }
    bool HandlesMethod(u32 method) const {
        return method >= m_first_method && method <= m_last_method;
    }

    virtual void OnClientConnected() {}
    virtual void OnClientDisconnected() {}

    virtual void OnOpened() {}
    virtual void OnClosed() {}

    virtual s32 InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                                 std::span<IpmiBufferInfo> out);
    virtual s32 InvokeAsyncMethod(u32 method, std::span<const IpmiDataInfo> in);

    // Sets the event flag bits of the server's client, used to signal finished requests.
    using EventFlagSetter = std::function<void(u32 index, u64 pattern)>;
    void SetEventFlagSetter(EventFlagSetter setter) {
        m_set_event_flag = std::move(setter);
    }

    s32 CreateRequest(std::span<const u8> params, bool blocking, s32* request_id);
    s32 StartRequest(s32 request_id, s32 ipc_slot);
    s32 AbortRequest(s32 request_id, bool blocking);
    s32 DestroyRequest(s32 request_id, bool blocking);

protected:
    // Returns the method's input at index as a T, nullptr if it's missing or too small.
    template <typename T>
    static const T* InputAs(std::span<const IpmiDataInfo> in, u64 index) {
        if (index >= in.size() || in[index].data == nullptr || in[index].size < sizeof(T)) {
            return nullptr;
        }
        return static_cast<const T*>(in[index].data);
    }

    // Copies a method output into the caller's buffer.
    static s32 WriteOutput(IpmiBufferInfo& out, const void* data, u64 size);

    // Assigns an id to a new request.
    virtual s32 OnCreateRequest(std::span<const u8> params, s32* request_id);

    // Called before a request is aborted or destroyed, to stop any work on it.
    virtual s32 OnAbortRequest(s32 request_id) {
        return 0;
    }
    virtual s32 OnDestroyRequest(s32 request_id) {
        return 0;
    }

    // Finishes a started request, waking the client waiting on it.
    s32 FinishRequest(s32 request_id);

    RequestTable& GetRequests() {
        return m_requests;
    }

private:
    std::string m_name;
    u32 m_id;
    u32 m_first_method;
    u32 m_last_method;

    RequestTable m_requests;
    EventFlagSetter m_set_event_flag;
    std::atomic<s32> m_next_request_id{1};
};

} // namespace Libraries::Kernel::Ipmi::NpService
