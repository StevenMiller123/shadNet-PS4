// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <deque>
#include <map>
#include <memory>
#include <orbis/libkernel.h>

#include "core/libraries/kernel/ipmi/ipmi_server.h"
#include "core/libraries/kernel/ipmi/np_auth_ctx.h"
#include "core/libraries/np/np_types.h"

namespace Libraries::Kernel::Ipmi {

// Emulated SceNpMgrIpc IPMI server
class NpMgrIpcServer : public Server, public std::enable_shared_from_this<NpMgrIpcServer> {
public:
    static constexpr std::string_view ServiceName = "SceNpMgrIpc";

    explicit NpMgrIpcServer(void* user_data);
    ~NpMgrIpcServer() override;

    void OnAdopted(u32 kid) override;
    s32 InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                         std::span<IpmiBufferInfo> out, s32* method_result) override;

private:
    static constexpr u32 CallbackSlotCount = 8;

    // An event queued for a callback slot, read through GetCallbackEvents.
    struct CallbackEvent {
        u64 unknown;
        s32 value;
        s32 user_id;
        u64 unknown2;
    };

    // An authorization code request that shadNet hasn't answered yet.
    struct PendingAuthorizationCode {
        AuthRequestCtx ctx;
        Np::OrbisNpAuthorizationCode code{};
    };

    // Struct for internal NpManager requests.
    struct Request {
        OrbisKernelEventFlag event_flag = nullptr;
        u64 event_pattern = 0;
        bool event_registered = false;
        bool aborted = false;
        s32 result = 0;
        s32 user_id = -1;
        // Resolver retries, then resolve, connect, send, and receive timeouts in microseconds.
        std::array<s32, 5> timeouts{};
        Np::OrbisNpAuthorizationCode auth_code{};
        std::shared_ptr<PendingAuthorizationCode> pending_auth;
    };

    s32 GetCallbackEventFlagName(std::span<IpmiBufferInfo> out);
    s32 RegisterCallback(std::span<const IpmiDataInfo> in);
    s32 UnregisterCallback(std::span<const IpmiDataInfo> in);
    s32 GetCallbackEvents(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 NotifyCallback(std::span<const IpmiDataInfo> in);
    s32 CreateRequest(std::span<const IpmiDataInfo> in);
    s32 DeleteRequest(std::span<const IpmiDataInfo> in);
    s32 AbortRequest(std::span<const IpmiDataInfo> in);
    s32 RegisterRequestEvent(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 FinishRequestEvent(std::span<const IpmiDataInfo> in);
    s32 IsSystemProcess(std::span<IpmiBufferInfo> out);
    s32 GetParentProcess(std::span<IpmiBufferInfo> out);
    s32 GetSigninState(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 GetActiveSigninState(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 CheckUserState(std::span<const IpmiDataInfo> in);
    s32 GetAccountId(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 GetUserIdByAccountId(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 SendGetParentalControlInfoRequest(std::span<const IpmiDataInfo> in);
    s32 GetParentalControlInfoResult(std::span<const IpmiDataInfo> in,
                                     std::span<IpmiBufferInfo> out);
    s32 GetUserIdByOnlineId(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 GetUserIdByNpId(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 GetAccountCountry(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 GetOnlineId(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 GetNpId(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 GetUserList(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 SetRequestTimeout(std::span<const IpmiDataInfo> in);
    s32 ClearUsedFlag(std::span<const IpmiDataInfo> in);
    s32 SendGetVshAccessTokenRequest(std::span<const IpmiDataInfo> in);
    s32 GetVshAccessTokenResult(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 SendGetTicketRequest(std::span<const IpmiDataInfo> in);
    s32 GetTicketResult(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 SendGetIssuerIdRequest(std::span<const IpmiDataInfo> in);
    s32 GetIssuerIdResult(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 SendCheckUserNpAvailabilityRequest(std::span<const IpmiDataInfo> in);
    s32 SendCheckNpAvailabilityRequest(std::span<const IpmiDataInfo> in);
    s32 CheckNpAvailabilityResult(std::span<const IpmiDataInfo> in);
    s32 SendGetGameAccessTokenRequest(std::span<const IpmiDataInfo> in);
    s32 GetGameAccessTokenResult(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);
    s32 SendGetAuthorizationCodeRequest(std::span<const IpmiDataInfo> in);
    s32 GetAuthorizationCodeResult(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out);

    // Sets and closes a request's registered event flag. Call with m_request_mutex held.
    static void SignalRequest(Request& request);
    // Finishes a request whose shadNet reply arrived, timed out, or was aborted.
    void CompleteAuthorizationCodeRequest(s32 request_id,
                                          const std::shared_ptr<PendingAuthorizationCode>& pending,
                                          s32 result);

    // Sets a callback slot's bit on the callback event flag. Call with m_callback_mutex held.
    void SignalCallback(u32 slot);

    // Notifies registered callbacks of shadNet sign-in changes.
    static void OnSigninStateChanged(s32 user_id, Np::OrbisNpState state);

    std::mutex m_request_mutex;
    std::map<s32, Request> m_requests;
    s32 m_next_request_id = 1;

    std::mutex m_callback_mutex;
    OrbisKernelEventFlag m_callback_event_flag = nullptr;

    // False when the event flag belongs to the real server, for clients we adopted.
    bool m_callback_event_flag_created = false;
    std::array<bool, CallbackSlotCount> m_callback_slots{};
    std::array<std::deque<CallbackEvent>, CallbackSlotCount> m_callback_events{};
};

} // namespace Libraries::Kernel::Ipmi
