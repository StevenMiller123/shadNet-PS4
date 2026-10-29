// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "common/types.h"

namespace Libraries::Np::NpSignaling {

using OrbisNpSignalingContextId = s32;
using OrbisNpSignalingConnectionId = s32;
using OrbisNpSignalingRequestId = u32;

using OrbisNpSignalingHandler = void (*)(u32 ctxId, u32 connId, s32 event, s32 errorCode,
                                         void* userArg);

constexpr s32 ORBIS_NP_SIGNALING_EVENT_DEAD = 0;
constexpr s32 ORBIS_NP_SIGNALING_EVENT_ESTABLISHED = 1;
constexpr s32 ORBIS_NP_SIGNALING_EVENT_NETINFO_ERROR = 2;
constexpr s32 ORBIS_NP_SIGNALING_EVENT_NETINFO_RESULT = 3;
constexpr s32 ORBIS_NP_SIGNALING_EVENT_PEER_ACTIVATED = 10;
constexpr s32 ORBIS_NP_SIGNALING_EVENT_PEER_DEACTIVATED = 11;
constexpr s32 ORBIS_NP_SIGNALING_EVENT_MUTUAL_ACTIVATED = 12;
constexpr s32 ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE = 0;
constexpr s32 ORBIS_NP_SIGNALING_CONN_STATUS_PENDING = 1;
constexpr s32 ORBIS_NP_SIGNALING_CONN_STATUS_ACTIVE = 2;
constexpr s32 ORBIS_NP_SIGNALING_CONN_INFO_RTT = 1;
constexpr s32 ORBIS_NP_SIGNALING_CONN_INFO_BANDWIDTH = 2;
constexpr s32 ORBIS_NP_SIGNALING_CONN_INFO_PEER_NP_ID = 3;
constexpr s32 ORBIS_NP_SIGNALING_CONN_INFO_PEER_ADDR = 4;
constexpr s32 ORBIS_NP_SIGNALING_CONN_INFO_MAPPED_ADDR = 5;
constexpr s32 ORBIS_NP_SIGNALING_CONN_INFO_PACKET_LOSS = 6;
constexpr s32 ORBIS_NP_SIGNALING_CONN_INFO_PEER_ADDRESS_A = 7;
constexpr s32 ORBIS_NP_SIGNALING_CONTEXT_OPTION_FLAG = 1;

struct OrbisNpSignalingNetInfo {
    u64 size;
    u32 localAddr;
    u32 mappedAddr;
    s32 natStatus;
    u32 _pad_14;
};
static_assert(sizeof(OrbisNpSignalingNetInfo) == 0x18);

struct OrbisNpSignalingAccountPlatformPair {
    u64 accountId;
    u32 platformType;
    u32 _pad_0c;
};
static_assert(sizeof(OrbisNpSignalingAccountPlatformPair) == 0x10);

struct OrbisNpSignalingMemoryInfo {
    u64 currentInUse;
    u64 peakInUse;
    u64 maxSystemSize;
};
static_assert(sizeof(OrbisNpSignalingMemoryInfo) == 0x18);

struct OrbisNpSignalingConnectionStatistics {
    u32 peakConnectionCount;
    u32 activeConnectionCount;
    u32 transientConnectionCount;
    u32 establishedConnectionCount;
};
static_assert(sizeof(OrbisNpSignalingConnectionStatistics) == 0x10);

s32 sceNpSignalingInitialize(s64 memorySize, s32 threadPriority, s32 cpuAffinityMask,
                             s64 threadStackSize);
s32 sceNpSignalingTerminate();
s32 sceNpSignalingCreateContext(const void* npId, void* callback, void* callbackArg,
                                OrbisNpSignalingContextId* outContextId);
s32 sceNpSignalingCreateContextA(s32 userId, void* callback, void* callbackArg,
                                 OrbisNpSignalingContextId* outContextId);
s32 sceNpSignalingDeleteContext(OrbisNpSignalingContextId ctxId);
s32 sceNpSignalingActivateConnection(OrbisNpSignalingContextId ctxId, const void* peerNpId,
                                     OrbisNpSignalingConnectionId* outConnId);
s32 sceNpSignalingActivateConnectionA(OrbisNpSignalingContextId ctxId,
                                      const OrbisNpSignalingAccountPlatformPair* peerAddr,
                                      OrbisNpSignalingConnectionId* outConnId);
s32 sceNpSignalingDeactivateConnection(OrbisNpSignalingContextId ctxId,
                                       OrbisNpSignalingConnectionId connId);
s32 sceNpSignalingTerminateConnection(OrbisNpSignalingContextId ctxId,
                                      OrbisNpSignalingConnectionId connId);
s32 sceNpSignalingGetConnectionStatus(OrbisNpSignalingContextId ctxId,
                                      OrbisNpSignalingConnectionId connId, u32* outStatus,
                                      u32* outPeerAddr, u16* outPeerPort);
s32 sceNpSignalingGetConnectionInfo(OrbisNpSignalingContextId ctxId,
                                    OrbisNpSignalingConnectionId connId, s32 infoCode,
                                    void* outInfo);
s32 sceNpSignalingGetConnectionInfoA(OrbisNpSignalingContextId ctxId,
                                     OrbisNpSignalingConnectionId connId, s32 infoCode,
                                     void* outInfo);
s32 sceNpSignalingGetConnectionFromNpId(OrbisNpSignalingContextId ctxId, const void* peerNpId,
                                        OrbisNpSignalingConnectionId* outConnId);
s32 sceNpSignalingGetConnectionFromPeerAddress(OrbisNpSignalingContextId ctxId, u32 peerAddr,
                                               u16 peerPort,
                                               OrbisNpSignalingConnectionId* outConnId);
s32 sceNpSignalingGetConnectionFromPeerAddressA(OrbisNpSignalingContextId ctxId,
                                                const OrbisNpSignalingAccountPlatformPair* peerAddr,
                                                OrbisNpSignalingConnectionId* outConnId);
s32 sceNpSignalingSetContextOption(OrbisNpSignalingContextId ctxId, s32 optionId, s32 optionValue);
s32 sceNpSignalingGetContextOption(OrbisNpSignalingContextId ctxId, s32 optionId,
                                   s32* outOptionValue);
s32 sceNpSignalingGetLocalNetInfo(OrbisNpSignalingContextId ctxId, OrbisNpSignalingNetInfo* info);
s32 sceNpSignalingGetPeerNetInfo(OrbisNpSignalingContextId ctxId, const void* peerNpId,
                                 OrbisNpSignalingRequestId* outReqId);
s32 sceNpSignalingGetPeerNetInfoA(OrbisNpSignalingContextId ctxId, const void* peerAccountPayload,
                                  OrbisNpSignalingRequestId* outReqId);
s32 sceNpSignalingCancelPeerNetInfo(OrbisNpSignalingContextId ctxId, s32 reqOrConnId);
s32 sceNpSignalingGetPeerNetInfoResult(OrbisNpSignalingContextId ctxId,
                                       OrbisNpSignalingConnectionId connId,
                                       OrbisNpSignalingNetInfo* peerNetInfo);
s32 sceNpSignalingGetMemoryInfo(OrbisNpSignalingMemoryInfo* info);
s32 sceNpSignalingGetConnectionStatistics(OrbisNpSignalingConnectionStatistics* stats);
s32 GetActiveConnectionIdForPeer(std::string_view online_id);
s32 GetConnectionStatusForPeer(std::string_view online_id, s32* out_conn_id);
bool GetPeerAddress(std::string_view online_id, u32* out_addr, u16* out_port);

void RegisterHooks();
} // namespace Libraries::Np::NpSignaling
