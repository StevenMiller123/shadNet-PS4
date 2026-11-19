// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <orbis/libkernel.h>

#include "common/logging/log.h"
#include "core/libraries/kernel/ipmi/np_service/np_service.h"
#include "core/libraries/np/np_error.h"

namespace Libraries::Kernel::Ipmi::NpService {

Service::Service(std::string_view name, u32 id, u32 first_method, u32 last_method)
    : m_name(name), m_id(id), m_first_method(first_method), m_last_method(last_method) {}

s32 Service::InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                              std::span<IpmiBufferInfo> out) {
    LOG_ERROR(Kernel_Ipmi, "{} sync method {:#x} not implemented ({} in, {} out)", m_name, method,
              in.size(), out.size());
    return ORBIS_KERNEL_ERROR_ENOSYS;
}

s32 Service::InvokeAsyncMethod(u32 method, std::span<const IpmiDataInfo> in) {
    LOG_ERROR(Kernel_Ipmi, "{} async method {:#x} not implemented ({} in)", m_name, method,
              in.size());
    return ORBIS_KERNEL_ERROR_ENOSYS;
}

s32 Service::WriteOutput(IpmiBufferInfo& out, const void* data, u64 size) {
    if (size > out.capacity) {
        return ORBIS_KERNEL_ERROR_EINVAL;
    }
    if (size != 0) {
        std::memcpy(out.data, data, size);
    }
    out.size = size;
    return 0;
}

s32 Service::OnCreateRequest(std::span<const u8> params, s32* request_id) {
    *request_id = m_next_request_id++;
    return 0;
}

s32 Service::CreateRequest(std::span<const u8> params, bool blocking, s32* request_id) {
    s32 id = -1;
    if (const s32 ret = OnCreateRequest(params, &id); ret < 0) {
        return ret;
    }
    if (blocking) {
        if (const s32 ret = m_requests.Create(id); ret < 0) {
            OnDestroyRequest(id);
            return ret;
        }
    }
    *request_id = id;
    return 0;
}

s32 Service::StartRequest(s32 request_id, s32 ipc_slot) {
    return m_requests.Start(request_id, ipc_slot);
}

s32 Service::AbortRequest(s32 request_id, bool blocking) {
    if (blocking) {
        if (const s32 ret = m_requests.Abort(request_id); ret < 0) {
            return ret;
        }
    }
    return OnAbortRequest(request_id);
}

s32 Service::DestroyRequest(s32 request_id, bool blocking) {
    if (blocking && !m_requests.GetState(request_id)) {
        return ORBIS_NP_IPC_ERROR_INVALID_REQUEST_ID;
    }
    if (const s32 ret = OnDestroyRequest(request_id); ret < 0) {
        return ret;
    }
    if (blocking) {
        m_requests.Destroy(request_id);
    }
    return 0;
}

s32 Service::FinishRequest(s32 request_id) {
    RequestTable::Signal signal{};
    if (const s32 ret = m_requests.Finish(request_id, &signal); ret < 0) {
        return ret;
    }
    if (m_set_event_flag) {
        m_set_event_flag(signal.index, signal.pattern);
    }
    return 0;
}

} // namespace Libraries::Kernel::Ipmi::NpService
