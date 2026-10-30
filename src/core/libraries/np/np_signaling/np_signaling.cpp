// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <arpa/inet.h>
#include <orbis/NetCtl.h>
#include <orbis/NpManager.h>
#include <orbis/NpSignaling.h>

#include "common/shadnet_hook.h"
#include "common/singleton.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_signaling/np_signaling_handler.h"
#include "core/libraries/np/np_signaling/np_signaling_helpers.h"
#include "core/libraries/np/np_signaling/np_signaling_state.h"
#include "core/libraries/np/np_signaling/np_signaling_transport.h"
#include "core/net_util/net_util.h"

SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingInitialize);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingTerminate);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingCreateContext);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingCreateContextA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingDeleteContext);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingActivateConnection);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingActivateConnectionA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingDeactivateConnection);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingTerminateConnection);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingGetConnectionStatus);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingGetConnectionInfo);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingGetConnectionInfoA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingGetConnectionFromNpId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingGetConnectionFromPeerAddress);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingGetConnectionFromPeerAddressA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingSetContextOption);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingGetContextOption);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingGetLocalNetInfo);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingGetPeerNetInfo);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingGetPeerNetInfoA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingCancelPeerNetInfo);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingGetPeerNetInfoResult);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingGetMemoryInfo);
SHADNET_HOOK_DECLARE(Libraries::Np::NpSignaling, sceNpSignalingGetConnectionStatistics);

static void RegisterNpSignalingHooks() {
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingInitialize);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingTerminate);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingCreateContext);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingCreateContextA);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingDeleteContext);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingActivateConnection);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingActivateConnectionA);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingDeactivateConnection);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingTerminateConnection);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingGetConnectionStatus);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingGetConnectionInfo);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingGetConnectionInfoA);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingGetConnectionFromNpId);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingGetConnectionFromPeerAddress);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingGetConnectionFromPeerAddressA);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingSetContextOption);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingGetContextOption);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingGetLocalNetInfo);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingGetPeerNetInfo);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingGetPeerNetInfoA);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingCancelPeerNetInfo);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingGetPeerNetInfoResult);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingGetMemoryInfo);
    SHADNET_HOOK(Libraries::Np::NpSignaling, sceNpSignalingGetConnectionStatistics);
}

namespace Libraries::Np::NpSignaling {

s32 sceNpSignalingInitialize(s64 memory_size, s32 thread_priority, s32 cpu_affinity_mask,
                             s64 thread_stack_size) {
    if (GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_ALREADY_INITIALIZED;
    }
    SignalingMutexGuard lock;

    if (memory_size == 0) {
        memory_size = 0x20000;
    }
    if (thread_priority == 0) {
        thread_priority = 700;
    }
    if (thread_stack_size == 0) {
        thread_stack_size = 0x4000;
    }

    LOG_INFO(Lib_NpSignaling,
             "memory_size={} thread_priority={} cpu_affinity_mask={} thread_stack_size={}",
             memory_size, thread_priority, cpu_affinity_mask, thread_stack_size);

    const s32 heap_rc = InitSignalingHeap(memory_size);
    if (heap_rc < 0) {
        return heap_rc;
    }

    RegisterRuntimeHooks();

    GetState()->initialized = true;

    const s32 rc = StartMainRuntime(thread_priority, cpu_affinity_mask, thread_stack_size);
    if (rc < 0) {
        GetState()->initialized = false;
        ShutdownSignalingHeap();
        return rc;
    }

    const s32 echo_rc = StartEchoRuntime(thread_priority, cpu_affinity_mask);
    if (echo_rc < 0) {
        GetState()->initialized = false;
        ShutdownRuntime();
        ShutdownSignalingHeap();
        return echo_rc;
    }
    return ORBIS_OK;
}

s32 sceNpSignalingTerminate() {
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    {
        SignalingMutexGuard lock;
        GetState()->initialized = false;
    }

    ShutdownRuntime();
    ShutdownSignalingHeap();

    u64 context_count = 0;
    {
        SignalingMutexGuard lock;
        NpSignalingState* state = GetState();
        context_count = state->contexts.size();
        state->ResetSig1Data();
    }
    for (u64 i = 0; i < context_count; ++i) {
        ReleaseContextRef();
    }

    LOG_INFO(Lib_NpSignaling, "called");
    return ORBIS_OK;
}

s32 sceNpSignalingCreateContext(const void* np_id, void* callback, void* callback_arg,
                                OrbisNpSignalingContextId* out_context_id) {
    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
    }
    if (!np_id || !out_context_id) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    OrbisNpId owner_npid{};
    OrbisNpOnlineId owner_online_id{};
    if (NormalizeNpId(np_id, &owner_npid, &owner_online_id) != ORBIS_OK) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    EnsureTransport();
    s32 ctx_id = 0;
    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }

        ctx_id = AllocateContextIdLocked();
        if (ctx_id < 0) {
            return ORBIS_NP_SIGNALING_ERROR_CTXID_NOT_AVAILABLE;
        }

        NpSignalingContext& ctx = GetState()->contexts[ctx_id];
        ctx.callback = reinterpret_cast<OrbisNpSignalingHandler>(callback);
        ctx.callback_arg = callback_arg;
        ctx.active = true;
        ctx.owner_npid = owner_npid;
        ctx.owner_online_id = owner_online_id;
        ctx.flags = 0x2;
        ctx.compiled_sdk_version = CaptureCompiledSdkVersion();
        ctx.activate_budget_us = kActivateBudgetMaxUs;
        ctx.activate_last_update_us = NowUs();
        ctx.bound_port = ConfiguredPort();
        *out_context_id = ctx_id;

        LOG_INFO(Lib_NpSignaling, "ctx_id={} owner='{}' callback={} arg={} sdk={:#x} p2p_port={}",
                 ctx_id, OnlineIdToString(owner_online_id), fmt::ptr(callback),
                 fmt::ptr(callback_arg), ctx.compiled_sdk_version, ctx.bound_port);
    }

    AddContextRef();
    return ORBIS_OK;
}

s32 sceNpSignalingCreateContextA(s32 user_id, void* callback, void* callback_arg,
                                 OrbisNpSignalingContextId* out_context_id) {
    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
    }
    if (user_id == -1 || !out_context_id) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    OrbisNpId owner_npid{};
    const s32 npid_rc = sceNpGetNpId(user_id, reinterpret_cast<::OrbisNpId*>(&owner_npid));
    if (npid_rc < 0) {
        return npid_rc;
    }
    OrbisNpAccountId account_id = 0;
    const s32 account_rc = sceNpGetAccountIdA(user_id, &account_id);
    if (account_rc < 0) {
        return account_rc;
    }
    if (account_id == 0) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }

    EnsureTransport();
    s32 ctx_id = 0;
    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }

        ctx_id = AllocateContextIdLocked();
        if (ctx_id < 0) {
            return ORBIS_NP_SIGNALING_ERROR_CTXID_NOT_AVAILABLE;
        }

        NpSignalingContext& ctx = GetState()->contexts[ctx_id];
        ctx.callback = reinterpret_cast<OrbisNpSignalingHandler>(callback);
        ctx.callback_arg = callback_arg;
        ctx.active = true;
        ctx.owner_npid = owner_npid;
        ctx.owner_online_id = owner_npid.handle;
        ctx.flags = 0x4 | 0x2;
        ctx.account_id = account_id;
        ctx.platform_type = 0;
        ctx.compiled_sdk_version = CaptureCompiledSdkVersion();
        ctx.activate_budget_us = kActivateBudgetMaxUs;
        ctx.activate_last_update_us = NowUs();
        ctx.bound_port = ConfiguredPort();
        *out_context_id = ctx_id;

        LOG_INFO(Lib_NpSignaling, "ctx_id={} user_id={} owner='{}' account_id={:#x}", ctx_id,
                 user_id, OnlineIdToString(ctx.owner_online_id), account_id);
    }

    AddContextRef();
    return ORBIS_OK;
}

s32 sceNpSignalingDeleteContext(OrbisNpSignalingContextId ctx_id) {
    std::vector<s32> active_conns;
    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }

        LOG_INFO(Lib_NpSignaling, "ctx_id={}", ctx_id);

        for (const auto& [cid, ci] : GetState()->connections) {
            if (ci.ctx_id == ctx_id && ci.status != ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE) {
                active_conns.push_back(cid);
            }
        }
    }

    for (const s32 cid : active_conns) {
        TerminateSig1(cid);
    }

    {
        SignalingMutexGuard lock;
        RemoveContextConnectionsLocked(ctx_id);
        for (auto it = GetState()->peer_netinfo_results.begin();
             it != GetState()->peer_netinfo_results.end();) {
            it = (it->second.ctx_id == ctx_id) ? GetState()->peer_netinfo_results.erase(it)
                                               : std::next(it);
        }
        GetState()->contexts.erase(ctx_id);
    }

    ReleaseContextRef();
    return ORBIS_OK;
}

s32 sceNpSignalingActivateConnection(OrbisNpSignalingContextId ctx_id, const void* peer_np_id,
                                     OrbisNpSignalingConnectionId* out_conn_id) {
    const s64 call_time = NowMs();
    LOG_INFO(Lib_NpSignaling, "(STUBBED) t={} ctx_id={} peer_np_id={:p} out_conn_id={:p}",
             call_time, ctx_id, peer_np_id, fmt::ptr(out_conn_id));

    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            LOG_INFO(Lib_NpSignaling,
                     "result: t={} ctx_id={} peer='<unresolved>' "
                     "rc={:#x}",
                     call_time, ctx_id, ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED);
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
    }
    if (!peer_np_id || !out_conn_id) {
        LOG_INFO(Lib_NpSignaling,
                 "result: t={} ctx_id={} peer='<unresolved>' "
                 "rc={:#x}",
                 call_time, ctx_id, ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT);
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    OrbisNpId peer_npid{};
    OrbisNpOnlineId peer_online_id{};
    const s32 normalize_rc = NormalizeNpId(peer_np_id, &peer_npid, &peer_online_id);
    if (normalize_rc != ORBIS_OK) {
        return normalize_rc;
    }

    const std::string peer_online_id_str = OnlineIdToString(peer_online_id);
    const s32 rc = ActivateSig1(ctx_id, peer_npid, peer_online_id, out_conn_id);
    if (rc == ORBIS_OK) {
        LOG_INFO(Lib_NpSignaling,
                 "result: t={} ctx_id={} peer='{}' rc={:#x} "
                 "conn_id={}",
                 call_time, ctx_id, peer_online_id_str, rc, *out_conn_id);
    } else {
        LOG_INFO(Lib_NpSignaling, "result: t={} ctx_id={} peer='{}' rc={:#x}", call_time, ctx_id,
                 peer_online_id_str, rc);
    }
    return rc;
}

s32 sceNpSignalingActivateConnectionA(OrbisNpSignalingContextId ctx_id,
                                      const OrbisNpSignalingAccountPlatformPair* peer_addr,
                                      OrbisNpSignalingConnectionId* out_conn_id) {
    OrbisNpId peer_npid{};
    if (peer_addr && peer_addr->account_id != 0 && peer_addr->platform_type != 0 && out_conn_id &&
        FindMatching2MemberNpId(peer_addr->account_id, &peer_npid)) {
        s32 rc = ActivateSig1(ctx_id, peer_npid, peer_npid.handle, out_conn_id);
        if (rc == ORBIS_NP_SIGNALING_ERROR_OWN_NP_ID) {
            rc = ORBIS_NP_SIGNALING_ERROR_OWN_PEER_ADDRESS;
        }
        LOG_INFO(Lib_NpSignaling,
                 "ctx_id={} account_id={:#x} platform={} peer='{}' rc={:#x} conn_id={}", ctx_id,
                 peer_addr->account_id, peer_addr->platform_type,
                 OnlineIdToString(peer_npid.handle), rc, rc == ORBIS_OK ? *out_conn_id : 0);
        return rc;
    }
    LOG_INFO(Lib_NpSignaling, "(STUBBED) ctx_id={}", ctx_id);

    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!peer_addr || peer_addr->account_id == 0 || !out_conn_id || peer_addr->platform_type == 0) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    const auto ctx_it = GetState()->contexts.find(ctx_id);
    if (ctx_it == GetState()->contexts.end() || !ctx_it->second.active) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if ((ctx_it->second.flags & 0x4) != 0 && ctx_it->second.account_id == peer_addr->account_id &&
        ctx_it->second.platform_type == peer_addr->platform_type) {
        return ORBIS_NP_SIGNALING_ERROR_OWN_PEER_ADDRESS;
    }

    if (!ConsumeActivationBudgetGatedLocked(ctx_it->second)) {
        return ORBIS_NP_SIGNALING_ERROR_EXCEED_RATE_LIMIT;
    }
    const s32 cid = AllocateConnectionIdLocked();
    if (cid < 0) {
        return ORBIS_NP_SIGNALING_ERROR_OUT_OF_MEMORY;
    }
    ConnectionInfo ci{};
    ci.conn_id = cid;
    ci.ctx_id = ctx_id;
    ci.status = ORBIS_NP_SIGNALING_CONN_STATUS_PENDING;
    GetState()->connections[cid] = std::move(ci);
    *out_conn_id = cid;

    LOG_INFO(Lib_NpSignaling, "ctx_id={} account_id={:#x} platform={} conn_id={}", ctx_id,
             peer_addr->account_id, peer_addr->platform_type, cid);
    return ORBIS_OK;
}

s32 sceNpSignalingDeactivateConnection(OrbisNpSignalingContextId ctx_id,
                                       OrbisNpSignalingConnectionId conn_id) {
    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
        if (!IsContextValidLocked(ctx_id)) {
            return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
        }

        const auto it = GetState()->connections.find(conn_id);
        if (it == GetState()->connections.end() || it->second.ctx_id != ctx_id) {
            return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
        }

        LOG_INFO(Lib_NpSignaling, "t={} ctx_id={} conn_id={} peer='{}' status={}", NowMs(), ctx_id,
                 conn_id, OnlineIdToString(it->second.online_id), it->second.status);
    }

    DeactivateSig1(conn_id);
    return ORBIS_OK;
}

s32 sceNpSignalingTerminateConnection(OrbisNpSignalingContextId ctx_id,
                                      OrbisNpSignalingConnectionId conn_id) {
    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
        if (!IsContextValidLocked(ctx_id)) {
            return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
        }
        const auto it = GetState()->connections.find(conn_id);
        if (it == GetState()->connections.end() || it->second.ctx_id != ctx_id) {
            return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
        }
        LOG_INFO(Lib_NpSignaling, "ctx_id={} conn_id={}", ctx_id, conn_id);
    }

    TerminateSig1(conn_id);
    {
        SignalingMutexGuard lock;
        RemoveConnectionLocked(conn_id);
    }
    return ORBIS_OK;
}

s32 sceNpSignalingGetConnectionStatus(OrbisNpSignalingContextId ctx_id,
                                      OrbisNpSignalingConnectionId conn_id, u32* out_status,
                                      u32* out_peer_addr, u16* out_peer_port) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!out_status) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    if (!IsContextValidLocked(ctx_id)) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }

    const auto it = GetState()->connections.find(conn_id);
    if (it == GetState()->connections.end() || it->second.ctx_id != ctx_id) {
        *out_status = ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE;
        return ORBIS_OK;
    }

    const auto transport_it = GetState()->peer_transports.find(it->second.transport_id);
    const auto* transport =
        transport_it != GetState()->peer_transports.end() ? &transport_it->second : nullptr;
    const s32 state = ConnectionStateFromStatus(it->second.status);
    if (state == 10) {
        *out_status = ORBIS_NP_SIGNALING_CONN_STATUS_ACTIVE;
        if (out_peer_addr && transport) {
            *out_peer_addr = transport->addr;
        }
        if (out_peer_port && transport) {
            *out_peer_port = transport->port;
        }
    } else if (state != 0) {
        *out_status = ORBIS_NP_SIGNALING_CONN_STATUS_PENDING;
    } else {
        *out_status = ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE;
    }

    LOG_INFO(Lib_NpSignaling, "t={} ctx_id={} conn_id={} peer='{}' status={}", NowMs(), ctx_id,
             conn_id, OnlineIdToString(it->second.online_id), *out_status);
    return ORBIS_OK;
}

s32 sceNpSignalingGetConnectionInfo(OrbisNpSignalingContextId ctx_id,
                                    OrbisNpSignalingConnectionId conn_id, s32 info_code,
                                    void* out_info) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!out_info) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const s32 rc = GetConnectionInfoInternal(ctx_id, conn_id, info_code, out_info, nullptr);
    LOG_INFO(Lib_NpSignaling, "ctx_id={} conn_id={} info_code={} rc={:#x}", ctx_id, conn_id,
             info_code, static_cast<u32>(rc));
    return rc;
}

s32 sceNpSignalingGetConnectionInfoA(OrbisNpSignalingContextId ctx_id,
                                     OrbisNpSignalingConnectionId conn_id, s32 info_code,
                                     void* out_info) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!out_info) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const s32 rc = GetConnectionInfoInternal(ctx_id, conn_id, info_code, nullptr, out_info);
    LOG_INFO(Lib_NpSignaling, "ctx_id={} conn_id={} info_code={} rc={:#x}", ctx_id, conn_id,
             info_code, static_cast<u32>(rc));
    return rc;
}

s32 sceNpSignalingGetConnectionFromNpId(OrbisNpSignalingContextId ctx_id, const void* peer_np_id,
                                        OrbisNpSignalingConnectionId* out_conn_id) {
    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
    }
    if (!peer_np_id || !out_conn_id) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    OrbisNpId peer_npid{};
    OrbisNpOnlineId peer_online_id{};
    if (NormalizeNpId(peer_np_id, &peer_npid, &peer_online_id) != ORBIS_OK) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!IsContextValidLocked(ctx_id)) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }

    const auto it = GetState()->npid_to_conn.find(MakeCtxNpIdKey(ctx_id, peer_npid));
    if (it == GetState()->npid_to_conn.end()) {
        return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
    }
    const auto conn_it = GetState()->connections.find(it->second);
    if (conn_it == GetState()->connections.end()) {
        return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
    }

    *out_conn_id = it->second;
    LOG_INFO(Lib_NpSignaling, "ctx_id={} npid='{}' conn_id={}", ctx_id,
             OnlineIdToString(peer_online_id), it->second);
    return ORBIS_OK;
}

s32 sceNpSignalingGetConnectionFromPeerAddress(OrbisNpSignalingContextId ctx_id, u32 peer_addr,
                                               u16 peer_port,
                                               OrbisNpSignalingConnectionId* out_conn_id) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!out_conn_id) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    if (!IsContextValidLocked(ctx_id)) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }

    for (const auto& [id, ci] : GetState()->connections) {
        const auto transport_it = GetState()->peer_transports.find(ci.transport_id);
        if (transport_it != GetState()->peer_transports.end() && ci.ctx_id == ctx_id &&
            transport_it->second.addr == peer_addr && transport_it->second.port == peer_port &&
            ConnectionStateFromStatus(ci.status) == 10) {
            *out_conn_id = id;
            LOG_INFO(Lib_NpSignaling, "ctx_id={} addr={:#x} port={} conn_id={}", ctx_id, peer_addr,
                     ntohs(peer_port), id);
            return ORBIS_OK;
        }
    }
    return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
}

s32 sceNpSignalingGetConnectionFromPeerAddressA(
    OrbisNpSignalingContextId ctx_id, const OrbisNpSignalingAccountPlatformPair* peer_addr,
    OrbisNpSignalingConnectionId* out_conn_id) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!peer_addr || !out_conn_id) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const auto ctx_it = GetState()->contexts.find(ctx_id);
    if (ctx_it == GetState()->contexts.end() || !ctx_it->second.active) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if ((ctx_it->second.flags & 0x4) == 0) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpSignaling, "ctx_id={} account_id={:#x}", ctx_id, peer_addr->account_id);
    return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
}

s32 sceNpSignalingSetContextOption(OrbisNpSignalingContextId ctx_id, s32 option_id,
                                   s32 option_value) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    const auto it = GetState()->contexts.find(ctx_id);
    if (it == GetState()->contexts.end() || !it->second.active) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if (option_id != ORBIS_NP_SIGNALING_CONTEXT_OPTION_FLAG) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    if (option_value == 0) {
        it->second.flags &= ~0x2u;
    } else if (option_value == 1) {
        it->second.flags |= 0x2u;
    } else {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    return ORBIS_OK;
}

s32 sceNpSignalingGetContextOption(OrbisNpSignalingContextId ctx_id, s32 option_id,
                                   s32* out_option_value) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!out_option_value) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const auto it = GetState()->contexts.find(ctx_id);
    if (it == GetState()->contexts.end() || !it->second.active) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if (option_id != ORBIS_NP_SIGNALING_CONTEXT_OPTION_FLAG) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    *out_option_value = (it->second.flags & 0x2u) != 0 ? 1 : 0;
    return ORBIS_OK;
}

s32 sceNpSignalingGetLocalNetInfo(OrbisNpSignalingContextId ctx_id, OrbisNpSignalingNetInfo* info) {
    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
    }
    if (!info) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    if (info->size != sizeof(OrbisNpSignalingNetInfo)) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    auto* netinfo = Common::Singleton<NetUtil::NetUtilInternal>::Instance();
    if (netinfo->GetIp().empty()) {
        netinfo->RetrieveIp();
    }
    info->local_addr = ParseIpv4Nbo(netinfo->GetIp());
    if (info->local_addr == 0) {
        info->local_addr = ParseIpv4Nbo("127.0.0.1");
    }

    OrbisNetCtlNatInfo nat_info{};
    nat_info.size = sizeof(nat_info);
    info->mapped_addr = 0;
    info->nat_status = 0;
    if (sceNetCtlGetNatInfo(&nat_info) >= 0) {
        info->mapped_addr = nat_info.mapped_addr;
        info->nat_status = nat_info.nat_type;
    }
    if (info->mapped_addr == 0) {
        const u32 external = netinfo->GetExternalIp();
        info->mapped_addr = external != 0 ? external : info->local_addr;
    }
    info->pad_14 = 0;

    LOG_INFO(Lib_NpSignaling, "local_addr={:#x} mapped_addr={:#x} nat_status={}", info->local_addr,
             info->mapped_addr, info->nat_status);
    return ORBIS_OK;
}

s32 sceNpSignalingGetPeerNetInfo(OrbisNpSignalingContextId ctx_id, const void* peer_np_id,
                                 OrbisNpSignalingRequestId* out_req_id) {
    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
    }
    if (!peer_np_id || !out_req_id) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    OrbisNpId peer_npid{};
    if (NormalizeNpId(peer_np_id, &peer_npid, nullptr) != ORBIS_OK) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    s32 req_id = 0;
    {
        SignalingMutexGuard lock;
        if (!IsContextValidLocked(ctx_id)) {
            return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
        }
        req_id = StagePeerNetInfoResultLocked(ctx_id);
        if (req_id == 0) {
            return ORBIS_NP_MATCHING2_SIGNALING_ERROR_OUT_OF_MEMORY;
        }
    }
    *out_req_id = static_cast<u32>(req_id);
    LOG_INFO(Lib_NpSignaling, "ctx_id={} req_id={:#x}", ctx_id, *out_req_id);
    return ORBIS_OK;
}

s32 sceNpSignalingGetPeerNetInfoA(OrbisNpSignalingContextId ctx_id,
                                  const void* peer_account_payload,
                                  OrbisNpSignalingRequestId* out_req_id) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!peer_account_payload || !out_req_id) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const auto ctx_it = GetState()->contexts.find(ctx_id);
    if (ctx_it == GetState()->contexts.end() || !ctx_it->second.active) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if ((ctx_it->second.flags & 0x4) == 0) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const s32 req_id = StagePeerNetInfoResultLocked(ctx_id);
    if (req_id == 0) {
        return ORBIS_NP_MATCHING2_SIGNALING_ERROR_OUT_OF_MEMORY;
    }
    *out_req_id = static_cast<u32>(req_id);
    LOG_INFO(Lib_NpSignaling, "ctx_id={} req_id={:#x}", ctx_id, *out_req_id);
    return ORBIS_OK;
}

s32 sceNpSignalingCancelPeerNetInfo(OrbisNpSignalingContextId ctx_id, s32 req_or_conn_id) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!IsContextValidLocked(ctx_id)) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if (!DropPeerNetInfoResultLocked(ctx_id, req_or_conn_id)) {
        return ORBIS_NP_SIGNALING_ERROR_REQ_NOT_FOUND;
    }
    LOG_INFO(Lib_NpSignaling, "ctx_id={} req_or_conn_id={:#x}", ctx_id, req_or_conn_id);
    return ORBIS_OK;
}

s32 sceNpSignalingGetPeerNetInfoResult(OrbisNpSignalingContextId ctx_id,
                                       OrbisNpSignalingConnectionId conn_id,
                                       OrbisNpSignalingNetInfo* peer_net_info) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!peer_net_info) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    if (!IsContextValidLocked(ctx_id)) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if (peer_net_info->size != sizeof(OrbisNpSignalingNetInfo)) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    PeerNetInfoResult result{};
    if (!TakePeerNetInfoResultLocked(ctx_id, conn_id, &result)) {
        return ORBIS_NP_SIGNALING_ERROR_RESULT_NOT_FOUND;
    }
    peer_net_info->local_addr = result.local_ipv4;
    peer_net_info->mapped_addr = result.external_ipv4;
    peer_net_info->nat_status = static_cast<s32>(result.nat_route_kind);
    peer_net_info->pad_14 = 0;
    LOG_INFO(Lib_NpSignaling, "ctx_id={} conn_id={:#x} ext={:#x}", ctx_id, conn_id,
             result.external_ipv4);
    return ORBIS_OK;
}

s32 sceNpSignalingGetMemoryInfo(OrbisNpSignalingMemoryInfo* info) {
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!info) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    info->current_in_use = 0;
    info->peak_in_use = 0;
    info->max_system_size = 0x20000;
    return ORBIS_OK;
}

s32 sceNpSignalingGetConnectionStatistics(OrbisNpSignalingConnectionStatistics* stats) {
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!stats) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    SignalingMutexGuard lock;
    SnapshotConnectionStatisticsLocked(
        &stats->peak_connection_count, &stats->active_connection_count,
        &stats->transient_connection_count, &stats->established_connection_count);
    return ORBIS_OK;
}

void RegisterHooks() {
    return RegisterNpSignalingHooks();
}

} // namespace Libraries::Np::NpSignaling
