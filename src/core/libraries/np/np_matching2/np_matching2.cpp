// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <arpa/inet.h>
#include <fmt/format.h>
#include <orbis/NpMatching2.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/network/net/net_error.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_matching2/np_matching2.h"
#include "core/libraries/np/np_matching2/np_matching2_internal.h"
#include "core/libraries/np/np_matching2/np_matching2_mm.h"
#include "core/libraries/np/np_matching2/np_matching2_signaling.h"
#include "core/libraries/np/np_signaling/np_signaling_handler.h"
#include "core/libraries/np/np_types.h"

SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2Initialize);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2Terminate);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2CreateContext);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2CreateContextA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2CreateJoinRoomA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2CreateJoinRoom);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2JoinRoom);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2RegisterContextCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2RegisterLobbyEventCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2RegisterRoomEventCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2RegisterSignalingCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2ContextStart);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2ContextStop);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2GetServerId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2GetWorldInfoList);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2LeaveRoom);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SetDefaultRequestOptParam);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SearchRoom);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SendRoomMessage);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SetUserInfo);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SetRoomDataExternal);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SetRoomDataInternal);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SignalingGetConnectionStatus);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2AbortContextStart);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2DestroyContext);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2RegisterLobbyMessageCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2RegisterRoomMessageCallback);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2GetMemoryInfo);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2GetSslMemoryInfo);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2GetRoomMemberIdListLocal);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2GetRoomPasswordLocal);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2GetSignalingOptParamLocal);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2GetLobbyInfoList);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2GetLobbyMemberDataInternal);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2GetLobbyMemberDataInternalList);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2GetRoomDataExternalList);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2GetRoomDataInternal);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2GetRoomMemberDataExternalList);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2GetRoomMemberDataInternal);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2GetUserInfoListA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2GetUserInfoList);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2GrantRoomOwner);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2JoinLobby);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2JoinRoomA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2KickoutRoomMember);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2LeaveLobby);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SendLobbyChatMessage);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SendRoomChatMessage);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SetLobbyMemberDataInternal);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SetRoomMemberDataInternal);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SetSignalingOptParam);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SignalingGetPeerNetInfo);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SignalingGetPingInfo);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SignalingCancelPeerNetInfo);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SignalingGetConnectionInfoA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SignalingGetConnectionInfo);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SignalingGetLocalNetInfo);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2SignalingGetPeerNetInfoResult);

static void RegisterNpMatching2Hooks() {
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2Initialize);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2Terminate);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2CreateContext);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2CreateContextA);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2CreateJoinRoomA);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2CreateJoinRoom);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2JoinRoom);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2RegisterContextCallback);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2RegisterLobbyEventCallback);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2RegisterRoomEventCallback);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2RegisterSignalingCallback);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2ContextStart);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2ContextStop);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2GetServerId);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2GetWorldInfoList);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2LeaveRoom);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SetDefaultRequestOptParam);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SearchRoom);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SendRoomMessage);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SetUserInfo);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SetRoomDataExternal);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SetRoomDataInternal);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SignalingGetConnectionStatus);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2AbortContextStart);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2DestroyContext);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2RegisterLobbyMessageCallback);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2RegisterRoomMessageCallback);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2GetMemoryInfo);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2GetSslMemoryInfo);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2GetRoomMemberIdListLocal);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2GetRoomPasswordLocal);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2GetSignalingOptParamLocal);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2GetLobbyInfoList);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2GetLobbyMemberDataInternal);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2GetLobbyMemberDataInternalList);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2GetRoomDataExternalList);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2GetRoomDataInternal);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2GetRoomMemberDataExternalList);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2GetRoomMemberDataInternal);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2GetUserInfoListA);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2GetUserInfoList);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2GrantRoomOwner);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2JoinLobby);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2JoinRoomA);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2KickoutRoomMember);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2LeaveLobby);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SendLobbyChatMessage);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SendRoomChatMessage);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SetLobbyMemberDataInternal);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SetRoomMemberDataInternal);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SetSignalingOptParam);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SignalingGetPeerNetInfo);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SignalingGetPingInfo);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SignalingCancelPeerNetInfo);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SignalingGetConnectionInfoA);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SignalingGetConnectionInfo);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SignalingGetLocalNetInfo);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2SignalingGetPeerNetInfoResult);
}

namespace Libraries::Np::NpMatching2 {
s32 sceNpMatching2CreateContext(const OrbisNpMatching2CreateContextParameter* param,
                                OrbisNpMatching2ContextId* ctxId) {
    LOG_INFO(Lib_NpMatching2, "called, npId = {}, serviceLabel = {}, size = {}",
             param->npId->handle.data, param->serviceLabel, param->size);

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!param || param->size != 0x28 || !ctxId) {
        LOG_ERROR(Lib_NpMatching2, "param null, invalid size, or ctxId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    const s32 rc = GetContextManager()->CreateContext(param->npId, param->serviceLabel, ctxId);
    if (rc == ORBIS_OK) {
        NpSignaling::AddContextRef();
    }
    return rc;
}

s32 sceNpMatching2CreateContextA(const OrbisNpMatching2CreateContextParameterA* param,
                                 OrbisNpMatching2ContextId* ctxId) {
    LOG_INFO(Lib_NpMatching2, "called, userId = {}, serviceLabel = {}, size = {}", param->userId,
             param->serviceLabel, param->size);

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!param || param->size != 0x10 || !ctxId) {
        LOG_ERROR(Lib_NpMatching2, "param null, invalid size, or ctxId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    const OrbisNpId np_id = NpHandler::GetInstance().GetNpId(param->userId);

    const s32 rc = GetContextManager()->CreateContext(&np_id, param->serviceLabel, ctxId, true);
    if (rc == ORBIS_OK) {
        NpSignaling::AddContextRef();
    }
    return rc;
}

s32 sceNpMatching2CreateJoinRoom(OrbisNpMatching2ContextId ctxId,
                                 OrbisNpMatching2CreateJoinRoomRequest* request,
                                 OrbisNpMatching2RequestOptParam* requestOpt,
                                 OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);
    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }
    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmCreateJoinRoom(ctxId, reqId, *request);
    return ORBIS_OK;
}

s32 sceNpMatching2JoinRoom(OrbisNpMatching2ContextId ctxId,
                           OrbisNpMatching2JoinRoomRequest* request,
                           OrbisNpMatching2RequestOptParam* requestOpt,
                           OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);
    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }
    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmJoinRoom(ctxId, reqId, *request);
    return ORBIS_OK;
}

s32 sceNpMatching2CreateJoinRoomA(OrbisNpMatching2ContextId ctxId,
                                  OrbisNpMatching2CreateJoinRoomRequestA* request,
                                  OrbisNpMatching2RequestOptParam* requestOpt,
                                  OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmCreateJoinRoomA(ctxId, reqId, *request);
    return ORBIS_OK;
}

s32 sceNpMatching2RegisterContextCallback(OrbisNpMatching2ContextCallback callback,
                                          void* userdata) {
    LOG_INFO(Lib_NpMatching2, "called, userdata = {}", userdata);

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    GetContextManager()->ApplyContextCallback(callback, userdata);
    return ORBIS_OK;
}

s32 sceNpMatching2RegisterLobbyEventCallback(OrbisNpMatching2ContextId ctxId,
                                             OrbisNpMatching2LobbyEventCallback callback,
                                             void* userdata) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, userdata = {}", ctxId, userdata);

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    ctx->lobby_event_callback = callback;
    ctx->lobby_event_callback_arg = userdata;
    return ORBIS_OK;
}

s32 sceNpMatching2RegisterRoomEventCallback(OrbisNpMatching2ContextId ctxId,
                                            OrbisNpMatching2RoomEventCallback callback,
                                            void* userdata) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, userdata = {}", ctxId, userdata);

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    ctx->room_event_callback = callback;
    ctx->room_event_callback_arg = userdata;
    return ORBIS_OK;
}

s32 sceNpMatching2RegisterSignalingCallback(OrbisNpMatching2ContextId ctxId,
                                            OrbisNpMatching2SignalingCallback callback,
                                            void* userdata) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, userdata = {}", ctxId, userdata);

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    ctx->signaling_callback = callback;
    ctx->signaling_callback_arg = userdata;
    NpSignaling::StartMatching2(*ctx);
    return ORBIS_OK;
}

s32 sceNpMatching2ContextStart(OrbisNpMatching2ContextId ctxId, u64 timeout) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, timeout = {}", ctxId, timeout);

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!NpHandler::GetInstance().IsActive()) {
        LOG_ERROR(Lib_NpMatching2, "shadNet disabled");
        return ORBIS_NET_ERROR_RESOLVER_ETIMEDOUT;
    }

    const s32 rc = GetContextManager()->Start(ctxId);
    if (rc != ORBIS_OK) {
        return rc;
    }

    MmContextStart(ctxId);
    return ORBIS_OK;
}

s32 sceNpMatching2ContextStop(OrbisNpMatching2ContextId ctxId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    const s32 rc = GetContextManager()->Stop(ctxId);
    if (rc != ORBIS_OK) {
        return rc;
    }
    auto ctx = GetContextManager()->Get(ctxId);
    if (ctx && ctx->signaling_callback) {
        NpSignaling::StopMatching2(*ctx, ORBIS_NP_MATCHING2_SIGNALING_ERROR_TERMINATED_BY_MYSELF);
    }

    const s32 submit_rc = MmContextStop(ctxId);
    if (submit_rc != ORBIS_OK) {
        GetContextManager()->CompleteStop(ctxId, ctx);
        return submit_rc;
    }
    return ORBIS_OK;
}

s32 sceNpMatching2Initialize(OrbisNpMatching2InitializeParameter* param) {
    LOG_INFO(Lib_NpMatching2, "called");

    if (IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "already initialized");
        return ORBIS_NP_MATCHING2_ERROR_ALREADY_INITIALIZED;
    }

    if (!param || (param->size != 0x30 && param->size != 0x28)) {
        LOG_ERROR(Lib_NpMatching2, "param null or invalid size");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    LOG_INFO(Lib_NpMatching2, "poolSize={:#x} stackSize={:#x} priority={} size={:#x}",
             param->poolSize, param->stackSize, param->priority, param->size);
    if (param->size == 0x30) {
        LOG_INFO(Lib_NpMatching2, "sslPoolSize={:#x}", param->sslPoolSize);
    }

    InitEventDispatcher();

    SetInitialized(true);

    return ORBIS_OK;
}

s32 sceNpMatching2Terminate() {
    LOG_INFO(Lib_NpMatching2, "called");

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    SetInitialized(false);

    TermEventDispatcher();
    GetContextManager()->Reset();
    {
        auto* state = GetState();
        std::lock_guard lock(state->mutex);
        state->next_request_id = 1;
    }
    NpSignaling::ReleaseContextRef();

    return ORBIS_OK;
}

s32 sceNpMatching2SetDefaultRequestOptParam(OrbisNpMatching2ContextId ctxId,
                                            OrbisNpMatching2RequestOptParam* requestOpt) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!requestOpt) {
        LOG_ERROR(Lib_NpMatching2, "requestOpt null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }
    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    LOG_INFO(Lib_NpMatching2, "ctx={} opt={} callback={:#x} arg={} timeout={} appId={}", ctxId,
             fmt::ptr(requestOpt), reinterpret_cast<std::uintptr_t>(requestOpt->callback),
             fmt::ptr(requestOpt->arg), requestOpt->timeout, requestOpt->appId);

    ctx->default_request_callback = requestOpt->callback;
    ctx->default_request_callback_arg = requestOpt->arg;

    return ORBIS_OK;
}

s32 sceNpMatching2GetServerId(OrbisNpMatching2ContextId ctxId, OrbisNpMatching2ServerId* serverId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!serverId) {
        LOG_ERROR(Lib_NpMatching2, "serverId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }
    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    *serverId = 1;

    return ORBIS_OK;
}

s32 sceNpMatching2GetWorldInfoList(OrbisNpMatching2ContextId ctxId,
                                   OrbisNpMatching2GetWorldInfoListRequest* request,
                                   OrbisNpMatching2RequestOptParam* requestOpt,
                                   OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, request.serverId = {}, requestOpt = {}", ctxId,
             request ? request->serverId : 0xFFFF, fmt::ptr(requestOpt));

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmGetWorldInfoList(ctxId, reqId, *request);
    return ORBIS_OK;
}

s32 sceNpMatching2LeaveRoom(OrbisNpMatching2ContextId ctxId,
                            OrbisNpMatching2LeaveRoomRequest* request,
                            OrbisNpMatching2RequestOptParam* requestOpt,
                            OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }
    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmLeaveRoom(ctxId, reqId, *request);
    return ORBIS_OK;
}

s32 sceNpMatching2SearchRoom(OrbisNpMatching2ContextId ctxId,
                             OrbisNpMatching2SearchRoomRequest* request,
                             OrbisNpMatching2RequestOptParam* requestOpt,
                             OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmSearchRoom(ctxId, reqId, *request, ctx->a_variant);
    return ORBIS_OK;
}

s32 sceNpMatching2SetUserInfo(OrbisNpMatching2ContextId ctxId,
                              OrbisNpMatching2SetUserInfoRequest* request,
                              OrbisNpMatching2RequestOptParam* requestOpt,
                              OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmSetUserInfo(ctxId, reqId, *request);
    return ORBIS_OK;
}

s32 sceNpMatching2SendRoomMessage(OrbisNpMatching2ContextId ctxId, void* request,
                                  OrbisNpMatching2RequestOptParam* requestOpt,
                                  OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    return MmSendRoomMessage(ctxId, reqId,
                             *static_cast<OrbisNpMatching2SendRoomMessageRequest*>(request));
}

s32 sceNpMatching2SetRoomDataExternal(OrbisNpMatching2ContextId ctxId,
                                      OrbisNpMatching2SetRoomDataExternalRequest* request,
                                      OrbisNpMatching2RequestOptParam* requestOpt,
                                      OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, request = {}, requestOpt = {}", ctxId,
             fmt::ptr(request), fmt::ptr(requestOpt));

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmSetRoomDataExternal(ctxId, reqId, *request);
    return ORBIS_OK;
}

s32 sceNpMatching2SetRoomDataInternal(OrbisNpMatching2ContextId ctxId,
                                      OrbisNpMatching2SetRoomDataInternalRequest* request,
                                      OrbisNpMatching2RequestOptParam* requestOpt,
                                      OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, request = {}, requestOpt = {}", ctxId,
             fmt::ptr(request), fmt::ptr(requestOpt));

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmSetRoomDataInternal(ctxId, reqId, *request);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpMatching2SignalingGetConnectionStatus(OrbisNpMatching2ContextId ctxId,
                                                            OrbisNpMatching2RoomId roomId,
                                                            OrbisNpMatching2RoomMemberId memberId,
                                                            s32* connStatus, void* peerAddr,
                                                            u16* peerPort) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, roomId = {}, memberId = {}", ctxId, roomId,
             memberId);

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }

    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    constexpr s32 kInactive = 0;
    constexpr s32 kPending = 1;
    constexpr s32 kActive = 2;
    auto cache = LockCache(ctx->ctx_id);
    const auto room_it = cache->rooms.find(roomId);
    const bool in_room = room_it != cache->rooms.end() &&
                         room_it->second.members.find(memberId) != room_it->second.members.end();

    if (!in_room) {
        if (connStatus) {
            *connStatus = kInactive;
        }
        LOG_INFO(Lib_NpMatching2, "member={} is not in room={}, status=inactive", memberId, roomId);
        return ORBIS_OK;
    }

    auto peer_it = cache->peers.find(memberId);
    if (peer_it == cache->peers.end()) {
        PeerInfo pi{};
        pi.member_id = memberId;
        pi.status = kPending;
        const auto member_it = room_it->second.members.find(memberId);
        if (member_it != room_it->second.members.end()) {
            pi.addr = member_it->second.addr;
            pi.port = member_it->second.port;
            pi.online_id = member_it->second.np_id.handle;
        }
        peer_it = cache->peers.emplace(memberId, pi).first;
    }

    const s32 status = peer_it->second.status == kActive ? kActive : kPending;
    if (connStatus) {
        *connStatus = status;
    }
    if (peerAddr) {
        *reinterpret_cast<u32*>(peerAddr) = peer_it->second.addr;
    }
    if (peerPort) {
        *peerPort = peer_it->second.port;
    }

    LOG_INFO(Lib_NpMatching2, "member={} is in room={}, status={} addr={:#x}:{}", memberId, roomId,
             status, peer_it->second.addr, ntohs(peer_it->second.port));
    return ORBIS_OK;
}

s32 sceNpMatching2AbortContextStart(OrbisNpMatching2ContextId ctxId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);
    return ORBIS_OK;
}

s32 sceNpMatching2DestroyContext(OrbisNpMatching2ContextId ctxId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!GetContextManager()->Destroy(ctxId)) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }
    NpSignaling::ReleaseContextRef();
    return ORBIS_OK;
}

s32 sceNpMatching2RegisterLobbyMessageCallback(OrbisNpMatching2ContextId ctxId,
                                               OrbisNpMatching2LobbyMessageCallback callback,
                                               void* userdata) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, userdata = {}", ctxId, userdata);

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    ctx->lobby_message_callback = callback;
    ctx->lobby_message_callback_arg = userdata;
    return ORBIS_OK;
}

s32 sceNpMatching2RegisterRoomMessageCallback(OrbisNpMatching2ContextId ctxId,
                                              OrbisNpMatching2RoomMessageCallback callback,
                                              void* userdata) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, callback = {}, userdata = {}", ctxId,
             fmt::ptr(reinterpret_cast<const void*>(callback)), userdata);

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    ctx->room_message_callback = callback;
    ctx->room_message_callback_arg = userdata;
    return ORBIS_OK;
}

s32 sceNpMatching2GetMemoryInfo() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

s32 sceNpMatching2GetSslMemoryInfo() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

s32 sceNpMatching2GetRoomMemberIdListLocal() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

s32 sceNpMatching2GetRoomPasswordLocal() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

s32 sceNpMatching2GetSignalingOptParamLocal() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

s32 sceNpMatching2GetLobbyInfoList() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

s32 sceNpMatching2GetLobbyMemberDataInternal() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

s32 sceNpMatching2GetLobbyMemberDataInternalList() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

s32 sceNpMatching2GetRoomDataExternalList(OrbisNpMatching2ContextId ctxId,
                                          OrbisNpMatching2GetRoomDataExternalListRequest* request,
                                          OrbisNpMatching2RequestOptParam* requestOpt,
                                          OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmGetRoomDataExternalList(ctxId, reqId, *request, ctx->a_variant);
    return ORBIS_OK;
}

s32 sceNpMatching2GetRoomDataInternal(OrbisNpMatching2ContextId ctxId,
                                      OrbisNpMatching2GetRoomDataInternalRequest* request,
                                      OrbisNpMatching2RequestOptParam* requestOpt,
                                      OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;

    auto request_payload_owner = std::make_shared<CallbackPayload>();
    void* request_data =
        BuildGetRoomDataInternalPayload(*ctx, *request_payload_owner, request->roomId);

    PendingEvent ev{};
    ev.type = PendingEvent::REQUEST_CB;
    ev.ctx_id = ctxId;
    ev.context_owner = ctx;
    ev.fire_at = std::chrono::steady_clock::now();
    ev.req_id = reqId;
    ev.req_event = ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_DATA_INTERNAL;
    ev.error_code = 0;
    const RequestCallbackInfo request_cb = ConsumeRequestCallback(ctx);
    ev.request_cb = request_cb.callback;
    ev.request_cb_arg = request_cb.arg;
    ev.request_data = request_data;
    ev.payload_owner = std::move(request_payload_owner);
    ScheduleEvent(std::move(ev));
    return ORBIS_OK;
}

s32 sceNpMatching2GetRoomMemberDataExternalList(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2GetRoomMemberDataExternalListRequest* request,
    OrbisNpMatching2RequestOptParam* requestOpt, OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmGetRoomMemberDataExternalList(ctxId, reqId, *request, ctx->a_variant);
    return ORBIS_OK;
}

s32 sceNpMatching2GetRoomMemberDataInternal(
    OrbisNpMatching2ContextId ctxId,
    const OrbisNpMatching2GetRoomMemberDataInternalRequest* request,
    const OrbisNpMatching2RequestOptParam* requestOpt, OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId || (request->attrIdNum > 0 && !request->attrId)) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;

    auto request_payload_owner = std::make_shared<CallbackPayload>();
    void* request_data =
        BuildGetRoomMemberDataInternalPayload(*ctx, *request_payload_owner, *request);

    PendingEvent ev{};
    ev.type = PendingEvent::REQUEST_CB;
    ev.ctx_id = ctxId;
    ev.context_owner = ctx;
    ev.fire_at = std::chrono::steady_clock::now();
    ev.req_id = reqId;
    ev.req_event = ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_MEMBER_DATA_INTERNAL_A;
    ev.error_code = 0;
    const RequestCallbackInfo request_cb = ConsumeRequestCallback(ctx);
    ev.request_cb = request_cb.callback;
    ev.request_cb_arg = request_cb.arg;
    ev.request_data = request_data;
    ev.payload_owner = std::move(request_payload_owner);
    ScheduleEvent(std::move(ev));
    return ORBIS_OK;
}

s32 sceNpMatching2GetUserInfoListA(OrbisNpMatching2ContextId ctxId,
                                   OrbisNpMatching2GetUserInfoListRequest* request,
                                   OrbisNpMatching2RequestOptParam* requestOpt,
                                   OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmGetUserInfoList(ctxId, reqId, *request, true);
    return ORBIS_OK;
}

s32 sceNpMatching2GetUserInfoList(OrbisNpMatching2ContextId ctxId,
                                  OrbisNpMatching2GetUserInfoListRequest* request,
                                  OrbisNpMatching2RequestOptParam* requestOpt,
                                  OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmGetUserInfoList(ctxId, reqId, *request, ctx->a_variant);
    return ORBIS_OK;
}

s32 sceNpMatching2GrantRoomOwner(OrbisNpMatching2ContextId ctxId, void* reqParam, void* optParam,
                                 s32* reqId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);
    return ORBIS_OK;
}

s32 sceNpMatching2JoinLobby(OrbisNpMatching2ContextId ctxId, void* reqParam, void* optParam,
                            s32* reqId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);
    return ORBIS_OK;
}

s32 sceNpMatching2JoinRoomA(OrbisNpMatching2ContextId ctxId,
                            OrbisNpMatching2JoinRoomRequestA* request,
                            OrbisNpMatching2RequestOptParam* requestOpt,
                            OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));
    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }
    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmJoinRoomA(ctxId, reqId, *request);
    return ORBIS_OK;
}

s32 sceNpMatching2KickoutRoomMember(OrbisNpMatching2ContextId ctxId,
                                    OrbisNpMatching2KickoutRoomMemberRequest* request,
                                    OrbisNpMatching2RequestOptParam* requestOpt,
                                    OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmKickoutRoomMember(ctxId, reqId, *request);
    return ORBIS_OK;
}

s32 sceNpMatching2LeaveLobby() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

s32 sceNpMatching2SendLobbyChatMessage() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

s32 sceNpMatching2SendRoomChatMessage(OrbisNpMatching2ContextId ctxId, void* reqParam,
                                      void* optParam, s32* reqId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);
    return ORBIS_OK;
}

s32 sceNpMatching2SetLobbyMemberDataInternal() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

s32 sceNpMatching2SetRoomMemberDataInternal(
    OrbisNpMatching2ContextId ctxId,
    const OrbisNpMatching2SetRoomMemberDataInternalRequest* request,
    const OrbisNpMatching2RequestOptParam* requestOpt, OrbisNpMatching2RequestId* requestId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, requestOpt = {}", ctxId, fmt::ptr(requestOpt));

    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!request || !requestId ||
        (request->roomMemberBinAttrInternalNum > 0 && !request->roomMemberBinAttrInternal)) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    StoreRequestCallback(ctx, requestOpt);
    const OrbisNpMatching2RequestId reqId = AllocRequestId();
    *requestId = reqId;
    MmSetRoomMemberDataInternal(ctxId, reqId, *request);
    return ORBIS_OK;
}

s32 sceNpMatching2SetSignalingOptParam() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

s32 sceNpMatching2SignalingGetPeerNetInfo() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

s32 sceNpMatching2SignalingGetPingInfo(OrbisNpMatching2ContextId ctxId, const void* reqParam,
                                       const void* optParam, s32* reqId) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}", ctxId);
    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    if (!reqParam || !reqId) {
        LOG_ERROR(Lib_NpMatching2, "request or requestId null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    const auto* request = static_cast<const OrbisNpMatching2SignalingGetPingInfoRequest*>(reqParam);
    StoreRequestCallback(ctx, static_cast<const OrbisNpMatching2RequestOptParam*>(optParam));
    const OrbisNpMatching2RequestId request_id = AllocRequestId();
    *reqId = request_id;

    auto request_payload_owner = std::make_shared<CallbackPayload>();
    void* request_data =
        BuildSignalingGetPingInfoPayload(*ctx, *request_payload_owner, request->roomId);

    PendingEvent ev{};
    ev.type = PendingEvent::REQUEST_CB;
    ev.ctx_id = ctxId;
    ev.context_owner = ctx;
    ev.fire_at = std::chrono::steady_clock::now();
    ev.req_id = request_id;
    ev.req_event = ORBIS_NP_MATCHING2_REQUEST_EVENT_SIGNALING_GET_PING_INFO;
    ev.error_code = ORBIS_OK;
    const RequestCallbackInfo request_cb = ConsumeRequestCallback(ctx);
    ev.request_cb = request_cb.callback;
    ev.request_cb_arg = request_cb.arg;
    ev.request_data = request_data;
    ev.payload_owner = std::move(request_payload_owner);
    ScheduleEvent(std::move(ev));
    return ORBIS_OK;
}

s32 sceNpMatching2SignalingCancelPeerNetInfo() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

s32 sceNpMatching2SignalingGetConnectionInfoA(OrbisNpMatching2ContextId ctxId,
                                              OrbisNpMatching2RoomId roomId,
                                              OrbisNpMatching2RoomMemberId memberId, u32 infoType,
                                              void* connInfo) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, roomId = {}, memberId = {}", ctxId, roomId,
             memberId);
    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    return FillMatching2ConnectionInfo(*ctx, roomId, memberId, infoType, connInfo, true);
}

s32 sceNpMatching2SignalingGetConnectionInfo(OrbisNpMatching2ContextId ctxId,
                                             OrbisNpMatching2RoomId roomId,
                                             OrbisNpMatching2RoomMemberId memberId, u32 infoType,
                                             void* connInfo) {
    LOG_INFO(Lib_NpMatching2, "called, ctxId = {}, roomId = {}, memberId = {}", ctxId, roomId,
             memberId);
    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    auto ctx = GetContextManager()->Get(ctxId);
    if (!ctx) {
        LOG_ERROR(Lib_NpMatching2, "invalid context id");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    return FillMatching2ConnectionInfo(*ctx, roomId, memberId, infoType, connInfo, false);
}

s32 sceNpMatching2SignalingGetLocalNetInfo(void* info) {
    LOG_INFO(Lib_NpMatching2, "called");
    if (!IsInitialized()) {
        LOG_ERROR(Lib_NpMatching2, "not initialized");
        return ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED;
    }
    return FillMatching2LocalNetInfo(info);
}

s32 sceNpMatching2SignalingGetPeerNetInfoResult() {
    LOG_INFO(Lib_NpMatching2, "called");
    return ORBIS_OK;
}

void RegisterHooks() {
    return RegisterNpMatching2Hooks();
}

} // namespace Libraries::Np::NpMatching2
