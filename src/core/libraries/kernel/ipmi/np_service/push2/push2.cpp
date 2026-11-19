// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/kernel/ipmi/np_service/push2/push2.h"
#include "core/libraries/np/np_error.h"

namespace Libraries::Kernel::Ipmi::NpService {

namespace {
enum class Method : u32 {
    RegisterNotificationUser = 0xe0000,
    UnregisterNotificationUser = 0xe0001,
    ConnectContext = 0xe0008,
    CreateServerContext = 0xe0009,
    DeleteServerContext = 0xe000a,
};

struct NotificationInput {
    s32 user_id;
    s32 lib_ctx_id;
};

struct ConnectContextInput {
    s32 lib_ctx_id;
    s32 param_2;
    s32 param_3;
    s32 param_4;
};

struct ServerContextInput {
    s32 lib_ctx_id;
};

} // namespace

Push2Service::Push2Service() : Service("SceNpPush2", Id, 0xe0000, 0xeffff) {}

s32 Push2Service::InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                                   std::span<IpmiBufferInfo> out) {
    switch (static_cast<Method>(method)) {
    case Method::RegisterNotificationUser:
        return RegisterNotificationUser(in);
    case Method::UnregisterNotificationUser:
        return UnregisterNotificationUser(in);
    case Method::ConnectContext:
        return ConnectContext(in);
    case Method::CreateServerContext:
        return CreateServerContext(in);
    case Method::DeleteServerContext:
        return DeleteServerContext(in);
    default:
        return Service::InvokeSyncMethod(method, in, out);
    }
}

// sceNpPush2CreateUserContext
s32 Push2Service::RegisterNotificationUser(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<NotificationInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{m_mutex};
    if (!m_proc_contexts.contains(input->lib_ctx_id)) {
        return ORBIS_NP_PUSH2_ERROR_IPC_PROCESS_CONTEXT_NOT_FOUND;
    }
    auto& proc_ctx = m_proc_contexts[input->lib_ctx_id];
    if (!proc_ctx.user_contexts.contains(input->user_id)) {
        proc_ctx.user_contexts.emplace(input->user_id, UserContext(input->user_id));
    }
    return 0;
}

// sceNpPush2CreateUserContext
s32 Push2Service::UnregisterNotificationUser(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<NotificationInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{m_mutex};
    if (!m_proc_contexts.contains(input->lib_ctx_id)) {
        return ORBIS_NP_PUSH2_ERROR_IPC_PROCESS_CONTEXT_NOT_FOUND;
    }
    auto& proc_ctx = m_proc_contexts[input->lib_ctx_id];
    if (!proc_ctx.user_contexts.contains(input->user_id)) {
        return ORBIS_NP_PUSH2_ERROR_IPC_USER_CONTEXT_NOT_FOUND;
    }
    proc_ctx.user_contexts.erase(input->user_id);
    return 0;
}

// sceNpPush2CreateUserContext
s32 Push2Service::ConnectContext(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<ConnectContextInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (input->lib_ctx_id == 0) {
        return ORBIS_NP_PUSH2_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{m_mutex};
    if (!m_proc_contexts.contains(input->lib_ctx_id)) {
        return ORBIS_NP_PUSH2_ERROR_IPC_PROCESS_CONTEXT_NOT_FOUND;
    }

    // Stores parameters into the process context
    return 0;
}

// sceNpPush2CreateUserContext
s32 Push2Service::CreateServerContext(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<ServerContextInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{m_mutex};
    if (!m_proc_contexts.contains(input->lib_ctx_id)) {
        m_proc_contexts.emplace(input->lib_ctx_id, ProcessContext());
    }
    return 0;
}

// sceNpPush2Term
s32 Push2Service::DeleteServerContext(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<ServerContextInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{m_mutex};
    if (m_proc_contexts.contains(input->lib_ctx_id)) {
        m_proc_contexts.erase(input->lib_ctx_id);
    }
    return 0;
}

} // namespace Libraries::Kernel::Ipmi::NpService
