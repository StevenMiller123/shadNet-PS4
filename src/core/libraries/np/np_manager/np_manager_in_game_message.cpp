// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "common/types.h"
#include "core/libraries/np/np_manager/np_manager.h"

namespace Libraries::Np::NpManager {

s32 sceNpInGameMessageInitialize(u64 pool_size, void* option) {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return 0;
}

s32 sceNpInGameMessageTerminate(s32 lib_ctx_id) {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return 0;
}

s32 sceNpInGameMessageCreateHandle(s32 lib_ctx_id) {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return 0;
}

s32 sceNpInGameMessageDeleteHandle(s32 lib_ctx_id, s32 handle_id) {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return 0;
}

s32 sceNpInGameMessageAbortHandle(s32 lib_ctx_id, s32 handle_id) {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return 0;
}

s32 sceNpInGameMessagePrepare(s32 lib_ctx_id, s32 handle_id, const void* reserved,
                              OrbisNpInGameMessageEventCallback cb_func, void* user_arg) {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return 0;
}

s32 sceNpInGameMessagePrepareA(s32 lib_ctx_id, s32 handle_id, const void* reserved,
                               OrbisNpInGameMessageEventCallbackA cb_func, void* user_arg) {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return 0;
}

s32 sceNpInGameMessageSendData(s32 lib_ctx_id, const OrbisNpPeerAddress* to,
                               const OrbisNpPeerAddress* from,
                               const OrbisNpInGameMessageData* message) {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return 0;
}

s32 sceNpInGameMessageSendDataA(s32 lib_ctx_id, const OrbisNpPeerAddressA* to,
                                const OrbisNpPeerAddressA* from,
                                const OrbisNpInGameMessageData* message) {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return 0;
}

s32 sceNpInGameMessageGetMemoryPoolStatistics(s32 lib_ctx_id,
                                              OrbisNpInGameMessageMemoryPoolStatistics* mem_stats) {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return 0;
}

} // namespace Libraries::Np::NpManager