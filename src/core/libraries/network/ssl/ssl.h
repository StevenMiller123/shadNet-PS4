// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::Network::Ssl {

s32 sceSslCreateSslConnection(s32 libssl_ctx_id, s32 socket, const char* server_name);
s32 sceSslDeleteSslConnection(s32 ssl_id);
s32 sceSslConnect(s32 ssl_id);
s32 sceSslClose(s32 ssl_id);
s32 sceSslSend(s32 ssl_id, const void* data, u64 size);
s32 sceSslRecv(s32 ssl_id, void* buf, u64 size);
s32 sceSslCheckRecvPending(s32 ssl_id, u8* pending);

void RegisterHooks();
} // namespace Libraries::Network::Ssl
