// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include <magic_enum/magic_enum.hpp>

#include "common/types.h"

namespace Libraries::Kernel {

// A cookie of sorts that gets passed to some ipmimgr_call calls.
constexpr u64 IpmiCookie = 0xdeadbadecafebeaf;

enum class IpmiOp : u32 {
    CreateServer = 0x0,
    DestroyServer = 0x1,
    CreateClient = 0x2,
    DestroyClient = 0x3,
    CreateSession = 0x4,
    DestroySession = 0x5,
    Trace = 0x10,
    ServerReceivePacket = 0x201,
    SendConnectResult = 0x212,
    SessionRespondSync = 0x232,
    ClientInvokeAsyncMethod = 0x241,
    SessionRespondAsync = 0x242,
    ClientTryGetResult = 0x243,
    ClientGetMessage = 0x251,
    ClientTryGetMessage = 0x252,
    SessionTrySendMessage = 0x254,
    SessionGetClientPid = 0x302,
    ClientDisconnect = 0x310,
    ClientInvokeSyncMethod = 0x320,
    ClientConnect = 0x400,
    SessionGetClientAppId = 0x463,
    ClientGetUserData = 0x467,
    SessionGetUserData = 0x468,
    ServerGetName = 0x46a,
    ClientGetName = 0x46b,
    ClientWaitEventFlag = 0x490,
    ClientPollEventFlag = 0x491,
    SessionSetEventFlag = 0x493,
    ClientTerminateConnection = 0x520,
};

// Event flag wait modes, matching sceKernelWaitEventFlag.
constexpr u32 IpmiEventFlagWaitAnd = 0x01;
constexpr u32 IpmiEventFlagWaitOr = 0x02;
constexpr u32 IpmiEventFlagClearAll = 0x10;
constexpr u32 IpmiEventFlagClearPattern = 0x20;

struct IpmiCreateClientConfig {
    u64 size;
    u32 unk[80];
    void* user_data;
};
static_assert(sizeof(IpmiCreateClientConfig) == 0x150);

struct IpmiDataInfo {
    const void* data;
    u64 size;
};
static_assert(sizeof(IpmiDataInfo) == 0x10);

struct IpmiBufferInfo {
    void* data;
    u64 capacity;
    u64 size;
};
static_assert(sizeof(IpmiBufferInfo) == 0x18);

struct IpmiCreateClientParams {
    void* client_impl;
    const char* name;
    const IpmiCreateClientConfig* config;
};
static_assert(sizeof(IpmiCreateClientParams) == 0x18);

struct IpmiClientConnectParams {
    const void* user_data;
    u64 user_data_size;
    s32* status;
    s32* arg3;
};
static_assert(sizeof(IpmiClientConnectParams) == 0x20);

struct IpmiSyncCallParams {
    u32 method;
    u32 num_in_data;
    u32 num_out_data;
    u32 unk;
    const IpmiDataInfo* in_data;
    IpmiBufferInfo* out_data;
    s32* result;
    u32 flags;
};
static_assert(sizeof(IpmiSyncCallParams) == 0x30);

struct IpmiAsyncCallParams {
    u32 method;
    u32 evf_index;
    u64 evf_value;
    u32 num_in_data;
    u32 padding;
    const IpmiDataInfo* in_data;
    s32* result;
    u32 flags;
};
static_assert(sizeof(IpmiAsyncCallParams) == 0x30);

struct IpmiTryGetResultParams {
    u32 method;
    u32 unk;
    s32* result;
    u32 num_out_data;
    u32 padding;
    IpmiBufferInfo* out_data;
    u32 flags;
    u32 padding2;
};
static_assert(sizeof(IpmiTryGetResultParams) == 0x28);

struct IpmiGetMessageParams {
    u32 queue_index;
    u32 padding;
    void* message;
    u64* size;
    u64 max_size;
    u32* timeout;
};
static_assert(sizeof(IpmiGetMessageParams) == 0x28);

struct IpmiTryGetMessageParams {
    u32 queue_index;
    u32 padding;
    void* message;
    u64* size;
    u64 max_size;
};
static_assert(sizeof(IpmiTryGetMessageParams) == 0x20);

struct IpmiDisconnectParams {
    s32* status;
};
static_assert(sizeof(IpmiDisconnectParams) == 0x8);

struct IpmiGetNameParams {
    char* name;
};
static_assert(sizeof(IpmiGetNameParams) == 0x8);

struct IpmiGetUserDataParams {
    void** user_data;
};
static_assert(sizeof(IpmiGetUserDataParams) == 0x8);

struct IpmiWaitEventFlagParams {
    u32 index;
    u32 padding0;
    u64 pattern;
    u32 mode;
    u32 padding1;
    u64* result_pattern;
    u32* timeout;
};
static_assert(sizeof(IpmiWaitEventFlagParams) == 0x28);

struct IpmiPollEventFlagParams {
    u32 index;
    u32 padding0;
    u64 pattern;
    u32 mode;
    u32 padding1;
    u64* result_pattern;
};
static_assert(sizeof(IpmiPollEventFlagParams) == 0x20);

s32 ipmimgr_call(IpmiOp op, u32 kid, u32* result, void* params, u64 params_size, u64 cookie);

// Calls a sync method on the real server a client is connected to, bypassing our emulated servers.
s32 InvokeRealSyncMethod(u32 kid, u32 method, std::span<const IpmiDataInfo> in,
                         std::span<IpmiBufferInfo> out, s32* method_result);

} // namespace Libraries::Kernel

template <>
struct magic_enum::customize::enum_range<Libraries::Kernel::IpmiOp> {
    static constexpr u32 min = static_cast<u32>(Libraries::Kernel::IpmiOp::CreateServer);
    static constexpr u32 max =
        static_cast<u32>(Libraries::Kernel::IpmiOp::ClientTerminateConnection);
};