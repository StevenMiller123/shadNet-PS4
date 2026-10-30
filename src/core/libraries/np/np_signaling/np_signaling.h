// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "common/types.h"

namespace Libraries::Np::NpSignaling {

using OrbisNpSignalingContextId = s32;
using OrbisNpSignalingConnectionId = s32;
using OrbisNpSignalingRequestId = u32;

using OrbisNpSignalingHandler = void (*)(u32 ctx_id, u32 conn_id, s32 event, s32 error_code,
                                         void* user_arg);

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
    u32 local_addr;
    u32 mapped_addr;
    s32 nat_status;
    u32 pad_14;
};
static_assert(sizeof(OrbisNpSignalingNetInfo) == 0x18);

struct OrbisNpSignalingAccountPlatformPair {
    u64 account_id;
    u32 platform_type;
    u32 pad_0c;
};
static_assert(sizeof(OrbisNpSignalingAccountPlatformPair) == 0x10);

struct OrbisNpSignalingMemoryInfo {
    u64 current_in_use;
    u64 peak_in_use;
    u64 max_system_size;
};
static_assert(sizeof(OrbisNpSignalingMemoryInfo) == 0x18);

struct OrbisNpSignalingConnectionStatistics {
    u32 peak_connection_count;
    u32 active_connection_count;
    u32 transient_connection_count;
    u32 established_connection_count;
};
static_assert(sizeof(OrbisNpSignalingConnectionStatistics) == 0x10);

s32 sceNpSignalingInitialize(s64 memory_size, s32 thread_priority, s32 cpu_affinity_mask,
                             s64 thread_stack_size);
s32 sceNpSignalingTerminate();
s32 sceNpSignalingCreateContext(const void* np_id, void* callback, void* callback_arg,
                                OrbisNpSignalingContextId* out_context_id);
s32 sceNpSignalingCreateContextA(s32 user_id, void* callback, void* callback_arg,
                                 OrbisNpSignalingContextId* out_context_id);
s32 sceNpSignalingDeleteContext(OrbisNpSignalingContextId ctx_id);
s32 sceNpSignalingActivateConnection(OrbisNpSignalingContextId ctx_id, const void* peer_np_id,
                                     OrbisNpSignalingConnectionId* out_conn_id);
s32 sceNpSignalingActivateConnectionA(OrbisNpSignalingContextId ctx_id,
                                      const OrbisNpSignalingAccountPlatformPair* peer_addr,
                                      OrbisNpSignalingConnectionId* out_conn_id);
s32 sceNpSignalingDeactivateConnection(OrbisNpSignalingContextId ctx_id,
                                       OrbisNpSignalingConnectionId conn_id);
s32 sceNpSignalingTerminateConnection(OrbisNpSignalingContextId ctx_id,
                                      OrbisNpSignalingConnectionId conn_id);
s32 sceNpSignalingGetConnectionStatus(OrbisNpSignalingContextId ctx_id,
                                      OrbisNpSignalingConnectionId conn_id, u32* out_status,
                                      u32* out_peer_addr, u16* out_peer_port);
s32 sceNpSignalingGetConnectionInfo(OrbisNpSignalingContextId ctx_id,
                                    OrbisNpSignalingConnectionId conn_id, s32 info_code,
                                    void* out_info);
s32 sceNpSignalingGetConnectionInfoA(OrbisNpSignalingContextId ctx_id,
                                     OrbisNpSignalingConnectionId conn_id, s32 info_code,
                                     void* out_info);
s32 sceNpSignalingGetConnectionFromNpId(OrbisNpSignalingContextId ctx_id, const void* peer_np_id,
                                        OrbisNpSignalingConnectionId* out_conn_id);
s32 sceNpSignalingGetConnectionFromPeerAddress(OrbisNpSignalingContextId ctx_id, u32 peer_addr,
                                               u16 peer_port,
                                               OrbisNpSignalingConnectionId* out_conn_id);
s32 sceNpSignalingGetConnectionFromPeerAddressA(
    OrbisNpSignalingContextId ctx_id, const OrbisNpSignalingAccountPlatformPair* peer_addr,
    OrbisNpSignalingConnectionId* out_conn_id);
s32 sceNpSignalingSetContextOption(OrbisNpSignalingContextId ctx_id, s32 option_id,
                                   s32 option_value);
s32 sceNpSignalingGetContextOption(OrbisNpSignalingContextId ctx_id, s32 option_id,
                                   s32* out_option_value);
s32 sceNpSignalingGetLocalNetInfo(OrbisNpSignalingContextId ctx_id, OrbisNpSignalingNetInfo* info);
s32 sceNpSignalingGetPeerNetInfo(OrbisNpSignalingContextId ctx_id, const void* peer_np_id,
                                 OrbisNpSignalingRequestId* out_req_id);
s32 sceNpSignalingGetPeerNetInfoA(OrbisNpSignalingContextId ctx_id,
                                  const void* peer_account_payload,
                                  OrbisNpSignalingRequestId* out_req_id);
s32 sceNpSignalingCancelPeerNetInfo(OrbisNpSignalingContextId ctx_id, s32 req_or_conn_id);
s32 sceNpSignalingGetPeerNetInfoResult(OrbisNpSignalingContextId ctx_id,
                                       OrbisNpSignalingConnectionId conn_id,
                                       OrbisNpSignalingNetInfo* peer_net_info);
s32 sceNpSignalingGetMemoryInfo(OrbisNpSignalingMemoryInfo* info);
s32 sceNpSignalingGetConnectionStatistics(OrbisNpSignalingConnectionStatistics* stats);
s32 GetActiveConnectionIdForPeer(std::string_view online_id);
s32 GetConnectionStatusForPeer(std::string_view online_id, s32* out_conn_id);
bool GetPeerAddress(std::string_view online_id, u32* out_addr, u16* out_port);

void RegisterHooks();
} // namespace Libraries::Np::NpSignaling
