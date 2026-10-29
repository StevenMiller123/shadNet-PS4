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

s32 sceNpSignalingInitialize(s64 memorySize, s32 threadPriority, s32 cpuAffinityMask,
                             s64 threadStackSize) {
    if (GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_ALREADY_INITIALIZED;
    }
    SignalingMutexGuard lock;

    if (memorySize == 0) {
        memorySize = 0x20000;
    }
    if (threadPriority == 0) {
        threadPriority = 700;
    }
    if (threadStackSize == 0) {
        threadStackSize = 0x4000;
    }

    LOG_INFO(Lib_NpSignaling,
             "memorySize={} threadPriority={} cpuAffinityMask={} threadStackSize={}", memorySize,
             threadPriority, cpuAffinityMask, threadStackSize);

    const s32 heap_rc = InitSignalingHeap(memorySize);
    if (heap_rc < 0) {
        return heap_rc;
    }

    RegisterRuntimeHooks();

    GetState()->initialized = true;

    const s32 rc = StartMainRuntime(threadPriority, cpuAffinityMask, threadStackSize);
    if (rc < 0) {
        GetState()->initialized = false;
        ShutdownSignalingHeap();
        return rc;
    }

    const s32 echo_rc = StartEchoRuntime(threadPriority, cpuAffinityMask);
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
        NpSignaling::ReleaseContextRef();
    }

    LOG_INFO(Lib_NpSignaling, "called");
    return ORBIS_OK;
}

s32 sceNpSignalingCreateContext(const void* npId, void* callback, void* callbackArg,
                                OrbisNpSignalingContextId* outContextId) {
    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
    }
    if (!npId || !outContextId) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    OrbisNpId owner_npid{};
    OrbisNpOnlineId owner_online_id{};
    if (NormalizeNpId(npId, &owner_npid, &owner_online_id) != ORBIS_OK) {
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
        ctx.callback_arg = callbackArg;
        ctx.active = true;
        ctx.owner_npid = owner_npid;
        ctx.owner_online_id = owner_online_id;
        ctx.flags = 0x2;
        ctx.compiled_sdk_version = CaptureCompiledSdkVersion();
        ctx.activate_budget_us = kActivateBudgetMaxUs;
        ctx.activate_last_update_us = NowUs();
        ctx.bound_port = ConfiguredPort();
        *outContextId = ctx_id;

        LOG_INFO(Lib_NpSignaling, "ctxId={} owner='{}' callback={} arg={} sdk={:#x} p2p_port={}",
                 ctx_id, OnlineIdToString(owner_online_id), fmt::ptr(callback),
                 fmt::ptr(callbackArg), ctx.compiled_sdk_version, ctx.bound_port);
    }

    NpSignaling::AddContextRef();
    return ORBIS_OK;
}

s32 sceNpSignalingCreateContextA(s32 userId, void* callback, void* callbackArg,
                                 OrbisNpSignalingContextId* outContextId) {
    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
    }
    if (userId == -1 || !outContextId) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    OrbisNpId owner_npid{};
    const s32 npid_rc = sceNpGetNpId(userId, reinterpret_cast<::OrbisNpId*>(&owner_npid));
    if (npid_rc < 0) {
        return npid_rc;
    }
    OrbisNpAccountId account_id = 0;
    const s32 account_rc = sceNpGetAccountIdA(userId, &account_id);
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
        ctx.callback_arg = callbackArg;
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
        *outContextId = ctx_id;

        LOG_INFO(Lib_NpSignaling, "ctxId={} userId={} owner='{}' accountId={:#x}", ctx_id, userId,
                 OnlineIdToString(ctx.owner_online_id), account_id);
    }

    NpSignaling::AddContextRef();
    return ORBIS_OK;
}

s32 sceNpSignalingDeleteContext(OrbisNpSignalingContextId ctxId) {
    std::vector<s32> active_conns;
    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }

        LOG_INFO(Lib_NpSignaling, "ctxId={}", ctxId);

        for (const auto& [cid, ci] : GetState()->connections) {
            if (ci.ctx_id == ctxId && ci.status != ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE) {
                active_conns.push_back(cid);
            }
        }
    }

    for (const s32 cid : active_conns) {
        NpSignaling::TerminateSig1(cid);
    }

    {
        SignalingMutexGuard lock;
        RemoveContextConnectionsLocked(ctxId);
        for (auto it = GetState()->peer_netinfo_results.begin();
             it != GetState()->peer_netinfo_results.end();) {
            it = (it->second.ctx_id == ctxId) ? GetState()->peer_netinfo_results.erase(it)
                                              : std::next(it);
        }
        GetState()->contexts.erase(ctxId);
    }

    NpSignaling::ReleaseContextRef();
    return ORBIS_OK;
}

s32 sceNpSignalingActivateConnection(OrbisNpSignalingContextId ctxId, const void* peerNpId,
                                     OrbisNpSignalingConnectionId* outConnId) {
    const s64 call_time = NowMs();
    LOG_INFO(Lib_NpSignaling, "(STUBBED) t={} ctxId={} peerNpId={:p} outConnId={:p}", call_time,
             ctxId, peerNpId, fmt::ptr(outConnId));

    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            LOG_INFO(Lib_NpSignaling,
                     "result: t={} ctxId={} peer='<unresolved>' "
                     "rc={:#x}",
                     call_time, ctxId, ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED);
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
    }
    if (!peerNpId || !outConnId) {
        LOG_INFO(Lib_NpSignaling,
                 "result: t={} ctxId={} peer='<unresolved>' "
                 "rc={:#x}",
                 call_time, ctxId, ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT);
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    OrbisNpId peer_npid{};
    OrbisNpOnlineId peer_online_id{};
    const s32 normalize_rc = NormalizeNpId(peerNpId, &peer_npid, &peer_online_id);
    if (normalize_rc != ORBIS_OK) {
        return normalize_rc;
    }

    const std::string peer_online_id_str = OnlineIdToString(peer_online_id);
    const s32 rc = NpSignaling::ActivateSig1(ctxId, peer_npid, peer_online_id, outConnId);
    if (rc == ORBIS_OK) {
        LOG_INFO(Lib_NpSignaling,
                 "result: t={} ctxId={} peer='{}' rc={:#x} "
                 "connId={}",
                 call_time, ctxId, peer_online_id_str, rc, *outConnId);
    } else {
        LOG_INFO(Lib_NpSignaling, "result: t={} ctxId={} peer='{}' rc={:#x}", call_time, ctxId,
                 peer_online_id_str, rc);
    }
    return rc;
}

s32 sceNpSignalingActivateConnectionA(OrbisNpSignalingContextId ctxId,
                                      const OrbisNpSignalingAccountPlatformPair* peerAddr,
                                      OrbisNpSignalingConnectionId* outConnId) {
    OrbisNpId peer_npid{};
    if (peerAddr && peerAddr->accountId != 0 && peerAddr->platformType != 0 && outConnId &&
        NpSignaling::FindMatching2MemberNpId(peerAddr->accountId, &peer_npid)) {
        s32 rc = NpSignaling::ActivateSig1(ctxId, peer_npid, peer_npid.handle, outConnId);
        if (rc == ORBIS_NP_SIGNALING_ERROR_OWN_NP_ID) {
            rc = ORBIS_NP_SIGNALING_ERROR_OWN_PEER_ADDRESS;
        }
        LOG_INFO(Lib_NpSignaling,
                 "ctxId={} accountId={:#x} platform={} peer='{}' rc={:#x} connId={}", ctxId,
                 peerAddr->accountId, peerAddr->platformType, OnlineIdToString(peer_npid.handle),
                 rc, rc == ORBIS_OK ? *outConnId : 0);
        return rc;
    }
    LOG_INFO(Lib_NpSignaling, "(STUBBED) ctxId={}", ctxId);

    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!peerAddr || peerAddr->accountId == 0 || !outConnId || peerAddr->platformType == 0) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    const auto ctx_it = GetState()->contexts.find(ctxId);
    if (ctx_it == GetState()->contexts.end() || !ctx_it->second.active) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if ((ctx_it->second.flags & 0x4) != 0 && ctx_it->second.account_id == peerAddr->accountId &&
        ctx_it->second.platform_type == peerAddr->platformType) {
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
    ci.ctx_id = ctxId;
    ci.status = ORBIS_NP_SIGNALING_CONN_STATUS_PENDING;
    GetState()->connections[cid] = std::move(ci);
    *outConnId = cid;

    LOG_INFO(Lib_NpSignaling, "ctxId={} accountId={:#x} platform={} connId={}", ctxId,
             peerAddr->accountId, peerAddr->platformType, cid);
    return ORBIS_OK;
}

s32 sceNpSignalingDeactivateConnection(OrbisNpSignalingContextId ctxId,
                                       OrbisNpSignalingConnectionId connId) {
    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
        if (!IsContextValidLocked(ctxId)) {
            return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
        }

        const auto it = GetState()->connections.find(connId);
        if (it == GetState()->connections.end() || it->second.ctx_id != ctxId) {
            return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
        }

        LOG_INFO(Lib_NpSignaling, "t={} ctxId={} connId={} peer='{}' status={}", NowMs(), ctxId,
                 connId, OnlineIdToString(it->second.online_id), it->second.status);
    }

    NpSignaling::DeactivateSig1(connId);
    return ORBIS_OK;
}

s32 sceNpSignalingTerminateConnection(OrbisNpSignalingContextId ctxId,
                                      OrbisNpSignalingConnectionId connId) {
    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
        if (!IsContextValidLocked(ctxId)) {
            return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
        }
        const auto it = GetState()->connections.find(connId);
        if (it == GetState()->connections.end() || it->second.ctx_id != ctxId) {
            return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
        }
        LOG_INFO(Lib_NpSignaling, "ctxId={} connId={}", ctxId, connId);
    }

    NpSignaling::TerminateSig1(connId);
    {
        SignalingMutexGuard lock;
        RemoveConnectionLocked(connId);
    }
    return ORBIS_OK;
}

s32 sceNpSignalingGetConnectionStatus(OrbisNpSignalingContextId ctxId,
                                      OrbisNpSignalingConnectionId connId, u32* outStatus,
                                      u32* outPeerAddr, u16* outPeerPort) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!outStatus) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    if (!IsContextValidLocked(ctxId)) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }

    const auto it = GetState()->connections.find(connId);
    if (it == GetState()->connections.end() || it->second.ctx_id != ctxId) {
        *outStatus = ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE;
        return ORBIS_OK;
    }

    const auto transport_it = GetState()->peer_transports.find(it->second.transport_id);
    const auto* transport =
        transport_it != GetState()->peer_transports.end() ? &transport_it->second : nullptr;
    const s32 state = ConnectionStateFromStatus(it->second.status);
    if (state == 10) {
        *outStatus = ORBIS_NP_SIGNALING_CONN_STATUS_ACTIVE;
        if (outPeerAddr && transport) {
            *outPeerAddr = transport->addr;
        }
        if (outPeerPort && transport) {
            *outPeerPort = transport->port;
        }
    } else if (state != 0) {
        *outStatus = ORBIS_NP_SIGNALING_CONN_STATUS_PENDING;
    } else {
        *outStatus = ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE;
    }

    LOG_INFO(Lib_NpSignaling, "t={} ctxId={} connId={} peer='{}' status={}", NowMs(), ctxId, connId,
             OnlineIdToString(it->second.online_id), *outStatus);
    return ORBIS_OK;
}

s32 sceNpSignalingGetConnectionInfo(OrbisNpSignalingContextId ctxId,
                                    OrbisNpSignalingConnectionId connId, s32 infoCode,
                                    void* outInfo) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!outInfo) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const s32 rc = GetConnectionInfoInternal(ctxId, connId, infoCode, outInfo, nullptr);
    LOG_INFO(Lib_NpSignaling, "ctxId={} connId={} infoCode={} rc={:#x}", ctxId, connId, infoCode,
             static_cast<u32>(rc));
    return rc;
}

s32 sceNpSignalingGetConnectionInfoA(OrbisNpSignalingContextId ctxId,
                                     OrbisNpSignalingConnectionId connId, s32 infoCode,
                                     void* outInfo) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!outInfo) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const s32 rc = GetConnectionInfoInternal(ctxId, connId, infoCode, nullptr, outInfo);
    LOG_INFO(Lib_NpSignaling, "ctxId={} connId={} infoCode={} rc={:#x}", ctxId, connId, infoCode,
             static_cast<u32>(rc));
    return rc;
}

s32 sceNpSignalingGetConnectionFromNpId(OrbisNpSignalingContextId ctxId, const void* peerNpId,
                                        OrbisNpSignalingConnectionId* outConnId) {
    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
    }
    if (!peerNpId || !outConnId) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    OrbisNpId peer_npid{};
    OrbisNpOnlineId peer_online_id{};
    if (NormalizeNpId(peerNpId, &peer_npid, &peer_online_id) != ORBIS_OK) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!IsContextValidLocked(ctxId)) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }

    const auto it = GetState()->npid_to_conn.find(MakeCtxNpIdKey(ctxId, peer_npid));
    if (it == GetState()->npid_to_conn.end()) {
        return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
    }
    const auto conn_it = GetState()->connections.find(it->second);
    if (conn_it == GetState()->connections.end()) {
        return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
    }

    *outConnId = it->second;
    LOG_INFO(Lib_NpSignaling, "ctxId={} npid='{}' connId={}", ctxId,
             OnlineIdToString(peer_online_id), it->second);
    return ORBIS_OK;
}

s32 sceNpSignalingGetConnectionFromPeerAddress(OrbisNpSignalingContextId ctxId, u32 peerAddr,
                                               u16 peerPort,
                                               OrbisNpSignalingConnectionId* outConnId) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!outConnId) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    if (!IsContextValidLocked(ctxId)) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }

    for (const auto& [id, ci] : GetState()->connections) {
        const auto transport_it = GetState()->peer_transports.find(ci.transport_id);
        if (transport_it != GetState()->peer_transports.end() && ci.ctx_id == ctxId &&
            transport_it->second.addr == peerAddr && transport_it->second.port == peerPort &&
            ConnectionStateFromStatus(ci.status) == 10) {
            *outConnId = id;
            LOG_INFO(Lib_NpSignaling, "ctxId={} addr={:#x} port={} connId={}", ctxId, peerAddr,
                     ntohs(peerPort), id);
            return ORBIS_OK;
        }
    }
    return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
}

s32 sceNpSignalingGetConnectionFromPeerAddressA(OrbisNpSignalingContextId ctxId,
                                                const OrbisNpSignalingAccountPlatformPair* peerAddr,
                                                OrbisNpSignalingConnectionId* outConnId) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!peerAddr || !outConnId) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const auto ctx_it = GetState()->contexts.find(ctxId);
    if (ctx_it == GetState()->contexts.end() || !ctx_it->second.active) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if ((ctx_it->second.flags & 0x4) == 0) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpSignaling, "ctxId={} accountId={:#x}", ctxId, peerAddr->accountId);
    return ORBIS_NP_SIGNALING_ERROR_CONN_NOT_FOUND;
}

s32 sceNpSignalingSetContextOption(OrbisNpSignalingContextId ctxId, s32 optionId, s32 optionValue) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    const auto it = GetState()->contexts.find(ctxId);
    if (it == GetState()->contexts.end() || !it->second.active) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if (optionId != ORBIS_NP_SIGNALING_CONTEXT_OPTION_FLAG) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    if (optionValue == 0) {
        it->second.flags &= ~0x2u;
    } else if (optionValue == 1) {
        it->second.flags |= 0x2u;
    } else {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    return ORBIS_OK;
}

s32 sceNpSignalingGetContextOption(OrbisNpSignalingContextId ctxId, s32 optionId,
                                   s32* outOptionValue) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!outOptionValue) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const auto it = GetState()->contexts.find(ctxId);
    if (it == GetState()->contexts.end() || !it->second.active) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if (optionId != ORBIS_NP_SIGNALING_CONTEXT_OPTION_FLAG) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    *outOptionValue = (it->second.flags & 0x2u) != 0 ? 1 : 0;
    return ORBIS_OK;
}

s32 sceNpSignalingGetLocalNetInfo(OrbisNpSignalingContextId ctxId, OrbisNpSignalingNetInfo* info) {
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
    info->localAddr = ParseIpv4Nbo(netinfo->GetIp());
    if (info->localAddr == 0) {
        info->localAddr = ParseIpv4Nbo("127.0.0.1");
    }

    OrbisNetCtlNatInfo nat_info{};
    nat_info.size = sizeof(nat_info);
    info->mappedAddr = 0;
    info->natStatus = 0;
    if (sceNetCtlGetNatInfo(&nat_info) >= 0) {
        info->mappedAddr = nat_info.mapped_addr;
        info->natStatus = nat_info.nat_type;
    }
    if (info->mappedAddr == 0) {
        const u32 external = netinfo->GetExternalIp();
        info->mappedAddr = external != 0 ? external : info->localAddr;
    }
    info->_pad_14 = 0;

    LOG_INFO(Lib_NpSignaling, "localAddr={:#x} mappedAddr={:#x} natStatus={}", info->localAddr,
             info->mappedAddr, info->natStatus);
    return ORBIS_OK;
}

s32 sceNpSignalingGetPeerNetInfo(OrbisNpSignalingContextId ctxId, const void* peerNpId,
                                 OrbisNpSignalingRequestId* outReqId) {
    {
        SignalingMutexGuard lock;
        if (!GetState()->initialized) {
            return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
        }
    }
    if (!peerNpId || !outReqId) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    OrbisNpId peer_npid{};
    if (NormalizeNpId(peerNpId, &peer_npid, nullptr) != ORBIS_OK) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    s32 req_id = 0;
    {
        SignalingMutexGuard lock;
        if (!IsContextValidLocked(ctxId)) {
            return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
        }
        req_id = StagePeerNetInfoResultLocked(ctxId);
        if (req_id == 0) {
            return ORBIS_NP_MATCHING2_SIGNALING_ERROR_OUT_OF_MEMORY;
        }
    }
    *outReqId = static_cast<u32>(req_id);
    LOG_INFO(Lib_NpSignaling, "ctxId={} reqId={:#x}", ctxId, *outReqId);
    return ORBIS_OK;
}

s32 sceNpSignalingGetPeerNetInfoA(OrbisNpSignalingContextId ctxId, const void* peerAccountPayload,
                                  OrbisNpSignalingRequestId* outReqId) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!peerAccountPayload || !outReqId) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const auto ctx_it = GetState()->contexts.find(ctxId);
    if (ctx_it == GetState()->contexts.end() || !ctx_it->second.active) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if ((ctx_it->second.flags & 0x4) == 0) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    const s32 req_id = StagePeerNetInfoResultLocked(ctxId);
    if (req_id == 0) {
        return ORBIS_NP_MATCHING2_SIGNALING_ERROR_OUT_OF_MEMORY;
    }
    *outReqId = static_cast<u32>(req_id);
    LOG_INFO(Lib_NpSignaling, "ctxId={} reqId={:#x}", ctxId, *outReqId);
    return ORBIS_OK;
}

s32 sceNpSignalingCancelPeerNetInfo(OrbisNpSignalingContextId ctxId, s32 reqOrConnId) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!IsContextValidLocked(ctxId)) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if (!DropPeerNetInfoResultLocked(ctxId, reqOrConnId)) {
        return ORBIS_NP_SIGNALING_ERROR_REQ_NOT_FOUND;
    }
    LOG_INFO(Lib_NpSignaling, "ctxId={} reqOrConnId={:#x}", ctxId, reqOrConnId);
    return ORBIS_OK;
}

s32 sceNpSignalingGetPeerNetInfoResult(OrbisNpSignalingContextId ctxId,
                                       OrbisNpSignalingConnectionId connId,
                                       OrbisNpSignalingNetInfo* peerNetInfo) {
    SignalingMutexGuard lock;
    if (!GetState()->initialized) {
        return ORBIS_NP_SIGNALING_ERROR_NOT_INITIALIZED;
    }
    if (!peerNetInfo) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }
    if (!IsContextValidLocked(ctxId)) {
        return ORBIS_NP_SIGNALING_ERROR_CTX_NOT_FOUND;
    }
    if (peerNetInfo->size != sizeof(OrbisNpSignalingNetInfo)) {
        return ORBIS_NP_SIGNALING_ERROR_INVALID_ARGUMENT;
    }

    PeerNetInfoResult result{};
    if (!TakePeerNetInfoResultLocked(ctxId, connId, &result)) {
        return ORBIS_NP_SIGNALING_ERROR_RESULT_NOT_FOUND;
    }
    peerNetInfo->localAddr = result.local_ipv4;
    peerNetInfo->mappedAddr = result.external_ipv4;
    peerNetInfo->natStatus = static_cast<s32>(result.nat_route_kind);
    peerNetInfo->_pad_14 = 0;
    LOG_INFO(Lib_NpSignaling, "ctxId={} connId={:#x} ext={:#x}", ctxId, connId,
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
    info->currentInUse = 0;
    info->peakInUse = 0;
    info->maxSystemSize = 0x20000;
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
    SnapshotConnectionStatisticsLocked(&stats->peakConnectionCount, &stats->activeConnectionCount,
                                       &stats->transientConnectionCount,
                                       &stats->establishedConnectionCount);
    return ORBIS_OK;
}

void RegisterHooks() {
    return RegisterNpSignalingHooks();
}

} // namespace Libraries::Np::NpSignaling