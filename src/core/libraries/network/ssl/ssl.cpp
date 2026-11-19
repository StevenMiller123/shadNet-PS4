// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstddef>
#include <orbis/Ssl.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/network/fake_psn/fake_matching2.h"
#include "core/libraries/network/ssl/ssl.h"

extern "C" void sceSslCheckRecvPending();

SHADNET_HOOK_DECLARE(Libraries::Network::Ssl, sceSslCreateSslConnection);
SHADNET_HOOK_DECLARE(Libraries::Network::Ssl, sceSslDeleteSslConnection);
SHADNET_HOOK_DECLARE(Libraries::Network::Ssl, sceSslConnect);
SHADNET_HOOK_DECLARE(Libraries::Network::Ssl, sceSslClose);
SHADNET_HOOK_DECLARE(Libraries::Network::Ssl, sceSslSend);
SHADNET_HOOK_DECLARE(Libraries::Network::Ssl, sceSslRecv);
SHADNET_HOOK_DECLARE(Libraries::Network::Ssl, sceSslCheckRecvPending);

static void RegisterSslHooks() {
    SHADNET_HOOK16(Libraries::Network::Ssl, sceSslCreateSslConnection);
    SHADNET_HOOK(Libraries::Network::Ssl, sceSslDeleteSslConnection);
    SHADNET_HOOK(Libraries::Network::Ssl, sceSslConnect);
    SHADNET_HOOK(Libraries::Network::Ssl, sceSslClose);
    SHADNET_HOOK(Libraries::Network::Ssl, sceSslSend);
    SHADNET_HOOK(Libraries::Network::Ssl, sceSslRecv);
    SHADNET_HOOK(Libraries::Network::Ssl, sceSslCheckRecvPending);
}

namespace Libraries::Network::Ssl {

s32 sceSslCreateSslConnection(s32 libssl_ctx_id, s32 socket, const char* server_name) {
    const s32 ssl_id =
        SHADNET_HOOK_CONTINUE(sceSslCreateSslConnection, libssl_ctx_id, socket, server_name);
    FakeMatching2::Instance().OnSslCreate(ssl_id, socket);
    return ssl_id;
}

s32 sceSslDeleteSslConnection(s32 ssl_id) {
    FakeMatching2::Instance().OnSslDelete(ssl_id);
    return SHADNET_HOOK_CONTINUE(sceSslDeleteSslConnection, ssl_id);
}

s32 sceSslConnect(s32 ssl_id) {
    if (const auto ret = FakeMatching2::Instance().OnSslConnect(ssl_id)) {
        return *ret;
    }
    return SHADNET_HOOK_CONTINUE(sceSslConnect, ssl_id);
}

s32 sceSslClose(s32 ssl_id) {
    if (const auto ret = FakeMatching2::Instance().OnSslClose(ssl_id)) {
        return *ret;
    }
    return SHADNET_HOOK_CONTINUE(sceSslClose, ssl_id);
}

s32 sceSslSend(s32 ssl_id, const void* data, u64 size) {
    if (const auto ret = FakeMatching2::Instance().OnSslSend(ssl_id, data, size)) {
        return *ret;
    }
    return SHADNET_HOOK_CONTINUE(sceSslSend, ssl_id, data, size);
}

s32 sceSslRecv(s32 ssl_id, void* buf, u64 size) {
    if (const auto ret = FakeMatching2::Instance().OnSslRecv(ssl_id, buf, size)) {
        return *ret;
    }
    return SHADNET_HOOK_CONTINUE(sceSslRecv, ssl_id, buf, size);
}

s32 sceSslCheckRecvPending(s32 ssl_id, u8* pending) {
    if (const auto ret = FakeMatching2::Instance().OnSslCheckRecvPending(ssl_id, pending)) {
        return *ret;
    }
    return SHADNET_HOOK_CONTINUE(sceSslCheckRecvPending, ssl_id, pending);
}

void RegisterHooks() {
    return RegisterSslHooks();
}
} // namespace Libraries::Network::Ssl
