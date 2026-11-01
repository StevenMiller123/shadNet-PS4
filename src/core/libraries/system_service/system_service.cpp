// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include <orbis/SystemService.h>
#include <queue>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/system_service/system_service.h"
#include "core/libraries/system_service/system_service_error.h"

SHADNET_HOOK_DECLARE(Libraries::SystemService, sceSystemServiceGetStatus);
SHADNET_HOOK_DECLARE(Libraries::SystemService, sceSystemServiceReceiveEvent);

static void RegisterSystemServiceHooks() {
    SHADNET_HOOK(Libraries::SystemService, sceSystemServiceGetStatus);
    SHADNET_HOOK(Libraries::SystemService, sceSystemServiceReceiveEvent);
}

namespace Libraries::SystemService {

static std::mutex g_event_queue_mutex;
static std::queue<OrbisSystemServiceEvent> g_event_queue;

s32 sceSystemServiceGetStatus(OrbisSystemServiceStatus* status) {
    s32 result = SHADNET_HOOK_CONTINUE(sceSystemServiceGetStatus, status);
    if (result == 0 && status != nullptr) {
        std::scoped_lock lk{g_event_queue_mutex};
        status->event_num += static_cast<s32>(g_event_queue.size());
    }
    return result;
}

s32 sceSystemServiceReceiveEvent(OrbisSystemServiceEvent* event) {
    if (event == nullptr) {
        return ORBIS_SYSTEM_SERVICE_ERROR_PARAMETER;
    }

    {
        std::scoped_lock lk{g_event_queue_mutex};
        if (!g_event_queue.empty()) {
            *event = g_event_queue.front();
            g_event_queue.pop();
            LOG_DEBUG(Lib_SystemService, "delivered event type={:#x} ({} still queued)",
                      static_cast<u32>(event->event_type), g_event_queue.size());
            return 0;
        }
    }
    return SHADNET_HOOK_CONTINUE(sceSystemServiceReceiveEvent, event);
}

void PushSystemServiceEvent(const OrbisSystemServiceEvent& event) {
    std::scoped_lock lk{g_event_queue_mutex};
    g_event_queue.push(event);
}

void RegisterHooks() {
    return RegisterSystemServiceHooks();
}
} // namespace Libraries::SystemService
