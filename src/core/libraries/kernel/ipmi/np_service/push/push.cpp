// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/ipmi/np_service/push/push.h"
#include "core/libraries/np/np_error.h"

namespace Libraries::Kernel::Ipmi::NpService {

namespace {

enum class Method : u32 {
    RegisterNotificationUser = 0x40000,
    UnregisterNotificationUser = 0x40001,
};

struct UserIdInput {
    s32 user_id;
};

} // namespace

PushService::PushService() : Service("SceNpPush", Id, 0x40000, 0x4ffff) {}

s32 PushService::InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                                  std::span<IpmiBufferInfo> out) {
    switch (static_cast<Method>(method)) {
    case Method::RegisterNotificationUser:
        return RegisterNotificationUser(in);
    case Method::UnregisterNotificationUser:
        return UnregisterNotificationUser(in);
    default:
        return Service::InvokeSyncMethod(method, in, out);
    }
}

// sceNpPushStartNotificationA, used by libSceNpWebApi to receive push events for a user.
s32 PushService::RegisterNotificationUser(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<UserIdInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{m_mutex};
    m_notification_users.insert(input->user_id);
    return 0;
}

// sceNpPushStopNotificationA
s32 PushService::UnregisterNotificationUser(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<UserIdInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{m_mutex};
    m_notification_users.erase(input->user_id);
    return 0;
}

} // namespace Libraries::Kernel::Ipmi::NpService
