// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <chrono>
#include <mutex>
#include <string_view>
#include <thread>
#include <vector>
#include <fmt/format.h>
#include <orbis/UserService.h>

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/kernel/ipmi/np_mgr_ipc.h"
#include "core/libraries/kernel/ipmi/np_ticket/np_ticket.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/rtc/rtc.h"
#include "shadnet/config.h"

namespace Libraries::Kernel::Ipmi {

using Np::NpHandler;
using Np::OrbisNpAccessToken;
using Np::OrbisNpAuthorizationCode;
using Np::OrbisNpClientId;
using Np::OrbisNpGameAccessToken;
using Np::OrbisNpId;
using Np::OrbisNpOnlineId;
using Np::OrbisNpScope;
using Np::OrbisNpSigninState;
using Np::OrbisNpTitleToken;
using Np::NpManager::OrbisNpCountryCode;
using Np::NpManager::OrbisNpParentalControlInfo;

namespace {

// shadNet's bearer token doesn't expire, so tokens report expiring an hour from now.
u64 TokenExpiration() {
    using namespace std::chrono;
    const auto now = duration_cast<microseconds>(system_clock::now().time_since_epoch());
    return static_cast<u64>((now + hours{1}).count() + Rtc::UNIX_EPOCH_TICKS);
}

// Requests that don't set timeouts use this instead.
constexpr std::chrono::seconds DefaultRequestTimeout{30};

// Set our timeout to the sum of the individual timeouts requests receive.
std::chrono::microseconds RequestTimeout(const std::array<s32, 5>& timeouts) {
    s64 total = 0;
    for (u64 i = 1; i < timeouts.size(); i++) {
        total += std::max(timeouts[i], 0);
    }
    if (total == 0) {
        return DefaultRequestTimeout;
    }
    return std::chrono::microseconds{total};
}

enum class Method : u32 {
    GetCallbackEventFlagName = 0x0,
    RegisterCallback = 0x1,
    UnregisterCallback = 0x2,
    GetCallbackEvents = 0x3,
    NotifyCallback = 0x4,
    CreateRequest = 0x5,
    DeleteRequest = 0x6,
    AbortRequest = 0x7,
    RegisterRequestEvent = 0x8,
    FinishRequestEvent = 0x9,
    IsSystemProcess = 0xa,
    GetParentProcess = 0xb,
    GetSigninState = 0xe,
    GetActiveSigninState = 0x10,
    CheckUserState = 0x11,
    GetAccountId = 0x12,
    GetUserIdByAccountId = 0x13,
    GetOnlineId = 0x14,
    GetUserIdByOnlineId = 0x16,
    GetNpId = 0x17,
    GetUserIdByNpId = 0x18,
    GetAccountCountry = 0x1b,
    GetUserList = 0x24,
    SetRequestTimeout = 0x3c,
    ClearUsedFlag = 0x3f,
    SendGetVshAccessTokenRequest = 0x40,
    GetVshAccessTokenResult = 0x41,
    SendGetTicketRequest = 0x44,
    GetTicketResult = 0x45,
    GetAuthorizationCodeResult = 0x47,
    SendGetParentalControlInfoRequest = 0x52,
    GetParentalControlInfoResult = 0x53,
    SendGetIssuerIdRequest = 0x54,
    GetIssuerIdResult = 0x55,
    SendCheckUserNpAvailabilityRequest = 0x84,
    CheckUserNpAvailabilityResult = 0x85,
    SendCheckNpAvailabilityRequest = 0x86,
    CheckNpAvailabilityResult = 0x87,
    SendGetGameAccessTokenRequest = 0x8b,
    GetGameAccessTokenResult = 0x8c,
    SendGetAuthorizationCodeRequest = 0x8f,
};

// Flags for CheckUserState
constexpr u32 CheckSignedIn = 0x1;
constexpr u32 CheckAccount = 0x2;
constexpr u32 CheckOnline = 0x4;

// Per-user flags for GetUserList
constexpr u32 ListSignedIn = 0x1;
constexpr u32 ListSignedOut = 0x4;
constexpr u32 ListNotSignedUp = 0x8;
constexpr u32 ListLoggedIn = 0x10;

struct UserListEntry {
    s32 user_id;
    OrbisNpId np_id;
    u64 account_id;
    OrbisNpSigninState state;
    u32 padding;
    u32 logged_in;
    u32 padding2;
};
static_assert(sizeof(UserListEntry) == 0x40);

OrbisNpSigninState SigninState(s32 user_id) {
    if (NpHandler::Instance().IsSignedIn(user_id)) {
        return OrbisNpSigninState::SignedIn;
    }
    if (!ShadNet::Settings::GetInstance().GetNpId(user_id).empty()) {
        return OrbisNpSigninState::SignedOut;
    }
    return OrbisNpSigninState::NotSignedUp;
}

template <typename T>
const T* InputAs(std::span<const IpmiDataInfo> in, u64 index) {
    if (index >= in.size() || in[index].data == nullptr || in[index].size < sizeof(T)) {
        return nullptr;
    }
    return static_cast<const T*>(in[index].data);
}

struct RequestIdInput {
    s32 request_id;
};

struct CallbackInput {
    u32 slot;
    u32 arg;
};

struct CallbackSlotInput {
    u32 slot;
};

// Callback slots that libSceNpManager uses.
constexpr u32 LastUserListSlot = 3;
constexpr u32 SigninStateSlot = 4;
constexpr u32 ActiveSigninStateSlot = 5;

constexpr u64 CallbackEventFlagNameSize = 0x20;
constexpr u64 MaxCallbackEvents = 20;

// Event flag attributes SceShellCore uses.
constexpr u32 CallbackEventFlagAttributes = 0x121;

// Servers that should hear about shadNet sign-in changes.
std::mutex g_servers_mutex;
std::vector<NpMgrIpcServer*> g_servers;

struct CreateRequestInput {
    u32 type;
    u32 padding;
    u64 data_size;
};

struct RegisterRequestEventInput {
    s32 request_id;
    u32 padding;
    u64 pattern;
};

struct RegisterRequestEventOutput {
    OrbisKernelEventFlag event_flag;
    u64 pattern;
};

struct UserInput {
    s32 user_id;
};

struct CheckUserStateInput {
    s32 user_id;
    u32 flags;
};

struct GetAccountIdInput {
    s32 user_id;
    u8 flag;
};

struct GetUserIdByAccountIdInput {
    u64 account_id;
    u8 flag; // 1 on normal requests, 0 on internal ones.
};

struct RequestUserInput {
    s32 request_id;
    s32 user_id;
};

struct GetAuthorizationCodeInput {
    s32 request_id;
    s32 user_id;
    u32 type;
    u8 unknown1;
    u8 unknown2;
};

struct GetTicketInput {
    s32 request_id;
    s32 user_id;
    u64 buffer_size;
};

struct GetTicketResultInput {
    s32 request_id;
    u32 size_only;
    u64 buffer_size;
};

struct GetNpIdInput {
    s32 user_id;
    u8 flag;
};

struct GetAccountCountryInput {
    s32 user_id;
    u8 flag;
};

struct GetUserListInput {
    u32 flags;
    s32 max_users;
};

struct SetRequestTimeoutInput {
    s32 request_id;
    s32 timeouts[5];
};

struct CheckUserNpAvailabilityInput {
    s32 request_id;
    s32 pid;
    s32 user_id;
};

struct CheckNpAvailabilityInput {
    s32 request_id;
    s32 pid;
    u8 flag;
};

} // namespace

NpMgrIpcServer::NpMgrIpcServer(void* user_data) : Server(std::string(ServiceName), user_data) {
    static std::once_flag registered;
    std::call_once(registered, [] {
        NpHandler::Instance().RegisterStateCallback(&NpMgrIpcServer::OnSigninStateChanged, nullptr);
    });
    std::scoped_lock lk{g_servers_mutex};
    g_servers.push_back(this);
}

NpMgrIpcServer::~NpMgrIpcServer() {
    {
        std::scoped_lock lk{g_servers_mutex};
        std::erase(g_servers, this);
    }
    if (m_callback_event_flag == nullptr) {
        return;
    }
    if (m_callback_event_flag_created) {
        sceKernelDeleteEventFlag(m_callback_event_flag);
    } else {
        sceKernelCloseEventFlag(m_callback_event_flag);
    }
}

void NpMgrIpcServer::OnAdopted(u32 kid) {
    // libSceNpManager already opened the real server's callback event flag
    // Retrieve the real name and open it ourselves.
    std::array<char, CallbackEventFlagNameSize> name{};
    IpmiBufferInfo out{name.data(), name.size(), 0};
    s32 result = 0;
    const s32 ret = InvokeRealSyncMethod(kid, static_cast<u32>(Method::GetCallbackEventFlagName),
                                         {}, {&out, 1}, &result);
    if (ret < 0 || result < 0) {
        LOG_WARNING(Kernel_Ipmi, "{} can't get the real callback event flag ({:#x}, {:#x})",
                    GetName(), ret, result);
        return;
    }
    name.back() = '\0';

    std::scoped_lock lk{m_callback_mutex};
    if (const s32 open = sceKernelOpenEventFlag(&m_callback_event_flag, name.data()); open < 0) {
        LOG_WARNING(Kernel_Ipmi, "{} can't open callback event flag {} ({:#x})", GetName(),
                    name.data(), open);
        m_callback_event_flag = nullptr;
        return;
    }
    LOG_INFO(Kernel_Ipmi, "{} opened callback event flag {}", GetName(), name.data());
}

s32 NpMgrIpcServer::InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                                     std::span<IpmiBufferInfo> out, s32* method_result) {
    s32 res;
    switch (static_cast<Method>(method)) {
    case Method::GetCallbackEventFlagName:
        res = GetCallbackEventFlagName(out);
        break;
    case Method::RegisterCallback:
        res = RegisterCallback(in);
        break;
    case Method::UnregisterCallback:
        res = UnregisterCallback(in);
        break;
    case Method::GetCallbackEvents:
        res = GetCallbackEvents(in, out);
        break;
    case Method::NotifyCallback:
        res = NotifyCallback(in);
        break;
    case Method::CreateRequest:
        res = CreateRequest(in);
        break;
    case Method::DeleteRequest:
        res = DeleteRequest(in);
        break;
    case Method::AbortRequest:
        res = AbortRequest(in);
        break;
    case Method::RegisterRequestEvent:
        res = RegisterRequestEvent(in, out);
        break;
    case Method::FinishRequestEvent:
        res = FinishRequestEvent(in);
        break;
    case Method::IsSystemProcess:
        res = IsSystemProcess(out);
        break;
    case Method::GetParentProcess:
        res = GetParentProcess(out);
        break;
    case Method::GetSigninState:
        res = GetSigninState(in, out);
        break;
    case Method::GetActiveSigninState:
        res = GetActiveSigninState(in, out);
        break;
    case Method::CheckUserState:
        res = CheckUserState(in);
        break;
    case Method::GetAccountId:
        res = GetAccountId(in, out);
        break;
    case Method::GetUserIdByAccountId:
        res = GetUserIdByAccountId(in, out);
        break;
    case Method::SendGetParentalControlInfoRequest:
        res = SendGetParentalControlInfoRequest(in);
        break;
    case Method::GetParentalControlInfoResult:
        res = GetParentalControlInfoResult(in, out);
        break;
    case Method::GetUserIdByOnlineId:
        res = GetUserIdByOnlineId(in, out);
        break;
    case Method::GetUserIdByNpId:
        res = GetUserIdByNpId(in, out);
        break;
    case Method::GetAccountCountry:
        res = GetAccountCountry(in, out);
        break;
    case Method::GetOnlineId:
        res = GetOnlineId(in, out);
        break;
    case Method::GetNpId:
        res = GetNpId(in, out);
        break;
    case Method::GetUserList:
        res = GetUserList(in, out);
        break;
    case Method::SetRequestTimeout:
        res = SetRequestTimeout(in);
        break;
    case Method::ClearUsedFlag:
        res = ClearUsedFlag(in);
        break;
    case Method::SendGetVshAccessTokenRequest:
        res = SendGetVshAccessTokenRequest(in);
        break;
    case Method::GetVshAccessTokenResult:
        res = GetVshAccessTokenResult(in, out);
        break;
    case Method::SendGetTicketRequest:
        res = SendGetTicketRequest(in);
        break;
    case Method::GetTicketResult:
        res = GetTicketResult(in, out);
        break;
    case Method::SendGetIssuerIdRequest:
        res = SendGetIssuerIdRequest(in);
        break;
    case Method::GetIssuerIdResult:
        res = GetIssuerIdResult(in, out);
        break;
    case Method::SendCheckUserNpAvailabilityRequest:
        res = SendCheckUserNpAvailabilityRequest(in);
        break;
    case Method::CheckUserNpAvailabilityResult:
        res = CheckNpAvailabilityResult(in);
        break;
    case Method::SendCheckNpAvailabilityRequest:
        res = SendCheckNpAvailabilityRequest(in);
        break;
    case Method::CheckNpAvailabilityResult:
        res = CheckNpAvailabilityResult(in);
        break;
    case Method::SendGetGameAccessTokenRequest:
        res = SendGetGameAccessTokenRequest(in);
        break;
    case Method::GetGameAccessTokenResult:
        res = GetGameAccessTokenResult(in, out);
        break;
    case Method::SendGetAuthorizationCodeRequest:
        res = SendGetAuthorizationCodeRequest(in);
        break;
    case Method::GetAuthorizationCodeResult:
        res = GetAuthorizationCodeResult(in, out);
        break;
    default:
        return Server::InvokeSyncMethod(method, in, out, method_result);
    }

    if (method_result != nullptr) {
        *method_result = res;
    }
    return 0;
}

void NpMgrIpcServer::SignalRequest(Request& request) {
    if (!request.event_registered) {
        return;
    }
    sceKernelSetEventFlag(request.event_flag, request.event_pattern);
    sceKernelCloseEventFlag(request.event_flag);
    request.event_registered = false;
}

void NpMgrIpcServer::SignalCallback(u32 slot) {
    if (m_callback_event_flag != nullptr) {
        sceKernelSetEventFlag(m_callback_event_flag, 1ULL << slot);
    }
}

void NpMgrIpcServer::OnSigninStateChanged(s32 user_id, Np::OrbisNpState state) {
    const OrbisNpSigninState signin_state = state == Np::OrbisNpState::SignedIn
                                                ? OrbisNpSigninState::SignedIn
                                                : OrbisNpSigninState::SignedOut;
    const CallbackEvent event{0, static_cast<s32>(signin_state), user_id, 0};

    std::scoped_lock lk{g_servers_mutex};
    for (NpMgrIpcServer* server : g_servers) {
        std::scoped_lock callback_lk{server->m_callback_mutex};
        for (u32 slot = 0; slot < CallbackSlotCount; slot++) {
            if (!server->m_callback_slots[slot]) {
                continue;
            }
            if (slot == SigninStateSlot || slot == ActiveSigninStateSlot) {
                server->m_callback_events[slot].push_back(event);
            } else if (slot > LastUserListSlot) {
                continue;
            }
            server->SignalCallback(slot);
        }
    }
}

// Internal, called when libSceNpManager first connects.
s32 NpMgrIpcServer::GetCallbackEventFlagName(std::span<IpmiBufferInfo> out) {
    if (out.empty()) {
        return ORBIS_NP_IPC_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{m_callback_mutex};
    std::array<char, CallbackEventFlagNameSize> name{};
    fmt::format_to_n(name.data(), name.size() - 1, "SceNpMgrIpcSvrCbP{:x}ID",
                     reinterpret_cast<uintptr_t>(this));
    if (m_callback_event_flag == nullptr) {
        if (const s32 ret = sceKernelCreateEventFlag(&m_callback_event_flag, name.data(),
                                                     CallbackEventFlagAttributes, 0, nullptr);
            ret < 0) {
            LOG_ERROR(Kernel_Ipmi, "{} can't create callback event flag {} ({:#x})", GetName(),
                      name.data(), ret);
            m_callback_event_flag = nullptr;
            return ret;
        }
        m_callback_event_flag_created = true;
    }
    return WriteOutput(out[0], name.data(), name.size());
}

// Used for various callback APIs
s32 NpMgrIpcServer::RegisterCallback(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<CallbackInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (input->slot >= CallbackSlotCount) {
        return ORBIS_NP_IPC_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{m_callback_mutex};
    if (m_callback_slots[input->slot]) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }
    m_callback_slots[input->slot] = true;
    return 0;
}

// Used for various callback APIs
s32 NpMgrIpcServer::UnregisterCallback(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<CallbackSlotInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (input->slot >= CallbackSlotCount) {
        return ORBIS_NP_IPC_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{m_callback_mutex};
    if (!m_callback_slots[input->slot]) {
        return ORBIS_NP_IPC_ERROR_NO_OBJECT;
    }
    m_callback_slots[input->slot] = false;
    m_callback_events[input->slot].clear();
    return 0;
}

// sceNpCheckCallback
s32 NpMgrIpcServer::GetCallbackEvents(std::span<const IpmiDataInfo> in,
                                      std::span<IpmiBufferInfo> out) {
    struct CallbackEventList {
        s32 count;
        u32 padding;
        std::array<CallbackEvent, MaxCallbackEvents> events;
    };
    static_assert(sizeof(CallbackEventList) == 0x1e8);

    const auto* input = InputAs<CallbackSlotInput>(in, 0);
    if (input == nullptr || out.size() < 2) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (input->slot >= CallbackSlotCount) {
        return ORBIS_NP_IPC_ERROR_INVALID_ARGUMENT;
    }

    CallbackEventList list{};
    s32 remaining;
    {
        std::scoped_lock lk{m_callback_mutex};
        if (!m_callback_slots[input->slot]) {
            return ORBIS_NP_IPC_ERROR_INTERNAL;
        }
        auto& events = m_callback_events[input->slot];
        while (!events.empty() && static_cast<u64>(list.count) < MaxCallbackEvents) {
            list.events[list.count++] = events.front();
            events.pop_front();
        }
        remaining = static_cast<s32>(events.size());
    }
    WriteOutput(out[0], &list, sizeof(list));
    return WriteOutput(out[1], &remaining, sizeof(remaining));
}

// sceNpRegisterStateCallback, sceNpRegisterStateCallbackForToolkit
s32 NpMgrIpcServer::NotifyCallback(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<CallbackSlotInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (input->slot >= CallbackSlotCount) {
        return ORBIS_NP_IPC_ERROR_INVALID_ARGUMENT;
    }
    // Runs the slot's callback right away, so it reports the current sign-in state.
    std::scoped_lock lk{m_callback_mutex};
    SignalCallback(input->slot);
    return 0;
}

// sceNpManagerIntCreateRequest
s32 NpMgrIpcServer::CreateRequest(std::span<const IpmiDataInfo> in) {
    if (InputAs<CreateRequestInput>(in, 0) == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{m_request_mutex};
    const s32 id = m_next_request_id++;
    m_requests.emplace(id, Request{});
    return id;
}

// sceNpManagerIntDeleteRequest
s32 NpMgrIpcServer::DeleteRequest(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<RequestIdInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{m_request_mutex};
    const auto it = m_requests.find(input->request_id);
    if (it == m_requests.end()) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }
    if (it->second.pending_auth) {
        it->second.pending_auth->ctx.SetResult(ORBIS_NP_ERROR_ABORTED);
    }
    SignalRequest(it->second);
    m_requests.erase(it);
    return 0;
}

// sceNpManagerIntAbortRequest
s32 NpMgrIpcServer::AbortRequest(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<RequestIdInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{m_request_mutex};
    const auto it = m_requests.find(input->request_id);
    if (it == m_requests.end()) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }
    it->second.aborted = true;
    // Pending requests finish as aborted once their waiting thread wakes.
    if (it->second.pending_auth) {
        it->second.pending_auth->ctx.SetResult(ORBIS_NP_ERROR_ABORTED);
    }
    return 0;
}

// Internal, called before waiting on internal requests
s32 NpMgrIpcServer::RegisterRequestEvent(std::span<const IpmiDataInfo> in,
                                         std::span<IpmiBufferInfo> out) {
    const auto* input = InputAs<RegisterRequestEventInput>(in, 0);
    if (input == nullptr || in.size() < 2 || in[1].data == nullptr || out.empty()) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }
    const std::string name(static_cast<const char*>(in[1].data),
                           strnlen(static_cast<const char*>(in[1].data), in[1].size));

    std::scoped_lock lk{m_request_mutex};
    const auto it = m_requests.find(input->request_id);
    if (it == m_requests.end()) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }
    Request& request = it->second;
    if (request.event_registered) {
        return ORBIS_NP_IPC_ERROR_REQUEST_NOT_FINISHED;
    }

    OrbisKernelEventFlag event_flag = nullptr;
    if (const s32 ret = sceKernelOpenEventFlag(&event_flag, name.c_str()); ret < 0) {
        LOG_ERROR(Kernel_Ipmi, "{} can't open event flag {} for request {} ({:#x})", GetName(),
                  name, input->request_id, ret);
        return ret;
    }
    request.event_flag = event_flag;
    request.event_pattern = input->pattern;
    request.event_registered = true;

    const RegisterRequestEventOutput output{event_flag, input->pattern};
    return WriteOutput(out[0], &output, sizeof(output));
}

// Internal, called after waiting on internal requests
s32 NpMgrIpcServer::FinishRequestEvent(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<RequestIdInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{m_request_mutex};
    const auto it = m_requests.find(input->request_id);
    if (it == m_requests.end()) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }
    SignalRequest(it->second);
    return 0;
}

// sceNpManagerIntCheckNpAvailability, sceNpManagerIntGetTicketA,
// sceNpManagerIntGetVshAccessToken
s32 NpMgrIpcServer::IsSystemProcess(std::span<IpmiBufferInfo> out) {
    return 0;
}

// sceNpManagerIntCheckNpAvailability, sceNpManagerIntGetTicketA,
// sceNpManagerIntGetVshAccessToken
s32 NpMgrIpcServer::GetParentProcess(std::span<IpmiBufferInfo> out) {
    if (out.empty()) {
        return 0;
    }
    const s32 pid = -1;
    WriteOutput(out[0], &pid, sizeof(pid));
    return 0;
}

// sceNpManagerIntGetSigninState
s32 NpMgrIpcServer::GetSigninState(std::span<const IpmiDataInfo> in,
                                   std::span<IpmiBufferInfo> out) {
    const auto* input = InputAs<UserInput>(in, 0);
    if (input == nullptr || input->user_id == -1 || out.empty()) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    const OrbisNpSigninState state = SigninState(input->user_id);
    return WriteOutput(out[0], &state, sizeof(state));
}

// sceNpManagerIntGetActiveSigninState
s32 NpMgrIpcServer::GetActiveSigninState(std::span<const IpmiDataInfo> in,
                                         std::span<IpmiBufferInfo> out) {
    const auto* input = InputAs<UserInput>(in, 0);
    if (input == nullptr || input->user_id == -1 || out.empty()) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    const OrbisNpSigninState state = SigninState(input->user_id);
    return WriteOutput(out[0], &state, sizeof(state));
}

// sceNpManagerIntCheckNpState, sceNpManagerIntCheckNpStateA, sceNpManagerIntCheckNpAvailability
s32 NpMgrIpcServer::CheckUserState(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<CheckUserStateInput>(in, 0);
    if (input == nullptr || input->user_id == -1) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    const bool signed_in = NpHandler::Instance().IsSignedIn(input->user_id);
    if ((input->flags & (CheckSignedIn | CheckAccount | CheckOnline)) != 0 && !signed_in) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    return 0;
}

// sceNpManagerIntGetAccountId, sceNpManagerIntGetAccountIdSdk
s32 NpMgrIpcServer::GetAccountId(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out) {
    const auto* input = InputAs<GetAccountIdInput>(in, 0);
    if (input == nullptr || input->user_id == -1 || out.empty()) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!NpHandler::Instance().IsSignedIn(input->user_id)) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }

    const u64 account_id = NpHandler::Instance().GetAccountId(input->user_id);
    return WriteOutput(out[0], &account_id, sizeof(account_id));
}

// sceNpGetUserIdByAccountId, sceNpManagerIntGetUserIdByAccountId
s32 NpMgrIpcServer::GetUserIdByAccountId(std::span<const IpmiDataInfo> in,
                                         std::span<IpmiBufferInfo> out) {
    const auto* input = InputAs<GetUserIdByAccountIdInput>(in, 0);
    if (input == nullptr || out.empty()) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    const s32 user_id = NpHandler::Instance().GetUserIdByAccountId(input->account_id);
    if (user_id == -1) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }

    return WriteOutput(out[0], &user_id, sizeof(user_id));
}

// sceNpManagerIntGetParentalControlInfoA
s32 NpMgrIpcServer::SendGetParentalControlInfoRequest(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<RequestUserInput>(in, 0);
    if (input == nullptr || input->user_id == -1) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    const bool signed_in = NpHandler::Instance().IsSignedIn(input->user_id);
    std::scoped_lock lk{m_request_mutex};
    const auto it = m_requests.find(input->request_id);
    if (it == m_requests.end()) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }

    it->second.result = signed_in ? 0 : ORBIS_NP_ERROR_SIGNED_OUT;
    SignalRequest(it->second);
    return 0;
}

// sceNpManagerIntGetParentalControlInfoA
s32 NpMgrIpcServer::GetParentalControlInfoResult(std::span<const IpmiDataInfo> in,
                                                 std::span<IpmiBufferInfo> out) {
    const auto* input = InputAs<RequestIdInput>(in, 0);
    if (input == nullptr || out.size() < 2) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{m_request_mutex};
    const auto it = m_requests.find(input->request_id);
    if (it == m_requests.end()) {
        return ORBIS_NP_MANAGER_ERROR_REQUEST_NOT_FOUND;
    }
    if (it->second.result < 0) {
        return it->second.result;
    }

    // TODO: Retrieve from console?
    const s8 age = 18;
    const OrbisNpParentalControlInfo restrictions{};
    WriteOutput(out[0], &age, sizeof(age));
    return WriteOutput(out[1], &restrictions, sizeof(restrictions));
}

// sceNpManagerIntGetUserIdByOnlineId
s32 NpMgrIpcServer::GetUserIdByOnlineId(std::span<const IpmiDataInfo> in,
                                        std::span<IpmiBufferInfo> out) {
    const auto* online_id = InputAs<OrbisNpOnlineId>(in, 1);
    if (online_id == nullptr || out.empty()) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    const s32 user_id = NpHandler::Instance().GetUserIdByOnlineId(*online_id);
    if (user_id == -1) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }

    return WriteOutput(out[0], &user_id, sizeof(user_id));
}

// sceNpManagerIntGetUserIdByNpId, sceNpManagerIntGetTicket
s32 NpMgrIpcServer::GetUserIdByNpId(std::span<const IpmiDataInfo> in,
                                    std::span<IpmiBufferInfo> out) {
    const auto* np_id = InputAs<OrbisNpId>(in, 1);
    if (np_id == nullptr || out.empty()) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    const s32 user_id = NpHandler::Instance().GetUserIdByOnlineId(np_id->handle);
    if (user_id == -1) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }

    return WriteOutput(out[0], &user_id, sizeof(user_id));
}

// sceNpManagerIntGetAccountCountryA, sceNpManagerIntGetAccountCountrySdk
s32 NpMgrIpcServer::GetAccountCountry(std::span<const IpmiDataInfo> in,
                                      std::span<IpmiBufferInfo> out) {
    const auto* input = InputAs<GetAccountCountryInput>(in, 0);
    if (input == nullptr || input->user_id == -1 || out.empty()) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!NpHandler::Instance().IsSignedIn(input->user_id)) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }

    OrbisNpCountryCode country_code{{'u', 's'}, '0', '0'};
    return WriteOutput(out[0], &country_code, sizeof(country_code));
}

// sceNpManagerIntGetOnlineId, sceNpManagerIntGetOnlineIdSdk
s32 NpMgrIpcServer::GetOnlineId(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out) {
    const auto* input = InputAs<GetNpIdInput>(in, 0);
    if (input == nullptr || input->user_id == -1 || out.empty()) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!NpHandler::Instance().IsSignedIn(input->user_id)) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }

    const OrbisNpOnlineId online_id = NpHandler::Instance().GetNpId(input->user_id).handle;
    return WriteOutput(out[0], &online_id, sizeof(online_id));
}

// sceNpManagerIntGetNpId, sceNpManagerIntGetNpIdSdk
s32 NpMgrIpcServer::GetNpId(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out) {
    const auto* input = InputAs<GetNpIdInput>(in, 0);
    if (input == nullptr || input->user_id == -1 || out.empty()) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!NpHandler::Instance().IsSignedIn(input->user_id)) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }

    const OrbisNpId np_id = NpHandler::Instance().GetNpId(input->user_id);
    return WriteOutput(out[0], &np_id, sizeof(np_id));
}

// sceNpManagerIntGetUserList
s32 NpMgrIpcServer::GetUserList(std::span<const IpmiDataInfo> in, std::span<IpmiBufferInfo> out) {
    const auto* input = InputAs<GetUserListInput>(in, 0);
    if (input == nullptr || input->max_users <= 0 || out.size() < 2) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    OrbisUserServiceLoginUserIdList login_users{};
    if (sceUserServiceGetLoginUserIdList(&login_users) < 0) {
        std::fill(std::begin(login_users.userId), std::end(login_users.userId), -1);
    }

    const u64 max_users = std::min<u64>(input->max_users, out[0].capacity / sizeof(UserListEntry));
    std::vector<UserListEntry> entries(input->max_users);
    s32 count = 0;
    auto& np_handler = NpHandler::Instance();
    for (const s32 user_id : login_users.userId) {
        if (user_id == -1 || static_cast<u64>(count) >= max_users) {
            continue;
        }

        const OrbisNpSigninState state = SigninState(user_id);
        u32 state_flag;
        switch (state) {
        case OrbisNpSigninState::SignedIn:
            state_flag = ListSignedIn;
            break;
        case OrbisNpSigninState::SignedOut:
            state_flag = ListSignedOut;
            break;
        default:
            state_flag = ListNotSignedUp;
            break;
        }
        if ((input->flags & (state_flag | ListLoggedIn)) == 0) {
            continue;
        }

        UserListEntry& entry = entries[count++];
        entry.user_id = user_id;
        entry.state = state;
        entry.logged_in = 1;
        if (state == OrbisNpSigninState::SignedIn) {
            entry.np_id = np_handler.GetNpId(user_id);
            entry.account_id = np_handler.GetAccountId(user_id);
        }
    }

    // The server always reports room for every requested entry.
    const u64 size = std::min<u64>(input->max_users * sizeof(UserListEntry), out[0].capacity);
    WriteOutput(out[0], entries.data(), size);
    return WriteOutput(out[1], &count, sizeof(count));
}

// sceNpManagerIntSetTimeout
s32 NpMgrIpcServer::SetRequestTimeout(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<SetRequestTimeoutInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{m_request_mutex};
    const auto it = m_requests.find(input->request_id);
    if (it == m_requests.end()) {
        return ORBIS_NP_MANAGER_ERROR_REQUEST_NOT_FOUND;
    }
    std::copy(std::begin(input->timeouts), std::end(input->timeouts), it->second.timeouts.begin());
    return 0;
}

// sceNpManagerIntClearUsedFlag
s32 NpMgrIpcServer::ClearUsedFlag(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<RequestIdInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{m_request_mutex};
    return m_requests.contains(input->request_id) ? 0 : ORBIS_NP_MANAGER_ERROR_REQUEST_NOT_FOUND;
}

// sceNpManagerIntGetVshAccessToken
s32 NpMgrIpcServer::SendGetVshAccessTokenRequest(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<RequestUserInput>(in, 0);
    if (input == nullptr || input->user_id == -1) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    const bool signed_in = NpHandler::Instance().IsSignedIn(input->user_id);
    std::scoped_lock lk{m_request_mutex};
    const auto it = m_requests.find(input->request_id);
    if (it == m_requests.end()) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }

    it->second.result = signed_in ? 0 : ORBIS_NP_ERROR_SIGNED_OUT;
    it->second.user_id = input->user_id;
    SignalRequest(it->second);
    return 0;
}

// sceNpManagerIntGetVshAccessToken
s32 NpMgrIpcServer::GetVshAccessTokenResult(std::span<const IpmiDataInfo> in,
                                            std::span<IpmiBufferInfo> out) {
    const auto* input = InputAs<RequestIdInput>(in, 0);
    if (input == nullptr || out.empty()) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    s32 result;
    s32 user_id;
    {
        std::scoped_lock lk{m_request_mutex};
        const auto it = m_requests.find(input->request_id);
        if (it == m_requests.end()) {
            return ORBIS_NP_IPC_ERROR_INTERNAL;
        }
        result = it->second.result;
        user_id = it->second.user_id;
    }
    if (result < 0) {
        return result;
    }

    // Use the shadNet bearer token as our access token
    const std::string bearer = NpHandler::Instance().GetBearerToken(user_id);
    OrbisNpAccessToken token{};
    bearer.copy(token.token, sizeof(token.token) - 1);
    token.expiration_time = TokenExpiration();
    return WriteOutput(out[0], &token, sizeof(token));
}

// sceNpManagerIntGetTicket, sceNpManagerIntGetTicketA
s32 NpMgrIpcServer::SendGetTicketRequest(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<GetTicketInput>(in, 0);
    if (input == nullptr || input->user_id == -1) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    const bool signed_in = NpHandler::Instance().IsSignedIn(input->user_id);
    std::scoped_lock lk{m_request_mutex};
    const auto it = m_requests.find(input->request_id);
    if (it == m_requests.end()) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }

    it->second.result = signed_in ? 0 : ORBIS_NP_ERROR_SIGNED_OUT;
    it->second.user_id = input->user_id;
    SignalRequest(it->second);
    return 0;
}

// sceNpManagerIntGetTicket, sceNpManagerIntGetTicketA
s32 NpMgrIpcServer::GetTicketResult(std::span<const IpmiDataInfo> in,
                                    std::span<IpmiBufferInfo> out) {
    const auto* input = InputAs<GetTicketResultInput>(in, 0);
    if (input == nullptr || out.size() < 2) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    s32 result;
    s32 user_id;
    {
        std::scoped_lock lk{m_request_mutex};
        const auto it = m_requests.find(input->request_id);
        if (it == m_requests.end()) {
            return ORBIS_NP_IPC_ERROR_INTERNAL;
        }
        result = it->second.result;
        user_id = it->second.user_id;
    }
    if (result < 0) {
        return result;
    }

    // Tickets name the service they're for, use the title's NP title id.
    const auto& np_title = Common::ElfInfo::Instance().GetNpTitle();
    const std::string service_id =
        np_title ? std::string(np_title->GetTitleIdString()) : std::string{};
    const NpTicket ticket =
        CreateNpTicket(NpHandler::Instance().GetAccountId(user_id),
                       NpHandler::Instance().GetNpId(user_id).handle, service_id);

    const u64 ticket_size = sizeof(ticket);
    WriteOutput(out[0], &ticket_size, sizeof(ticket_size));
    if (input->size_only != 0) {
        return 0;
    }
    if (input->buffer_size < ticket_size) {
        return ORBIS_NP_MANAGER_ERROR_INSUFFICIENT_BUFFER;
    }
    const s32 ret = WriteOutput(out[1], &ticket, sizeof(ticket));
    if (ret < 0) {
        return ret;
    }
    // SceShellCore reports the caller's buffer size as the output size.
    out[1].size = std::min<u64>(input->buffer_size, out[1].capacity);
    return 0;
}

// sceNpManagerIntGetIssuerId
s32 NpMgrIpcServer::SendGetIssuerIdRequest(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<RequestUserInput>(in, 0);
    if (input == nullptr || input->user_id == -1) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    const bool signed_in = NpHandler::Instance().IsSignedIn(input->user_id);
    std::scoped_lock lk{m_request_mutex};
    const auto it = m_requests.find(input->request_id);
    if (it == m_requests.end()) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }

    // For released games, issuer ID is always 0x100.
    it->second.result = signed_in ? 0x100 : ORBIS_NP_ERROR_SIGNED_OUT;
    SignalRequest(it->second);
    return 0;
}

// sceNpManagerIntGetIssuerId
s32 NpMgrIpcServer::GetIssuerIdResult(std::span<const IpmiDataInfo> in,
                                      std::span<IpmiBufferInfo> out) {
    const auto* input = InputAs<RequestIdInput>(in, 0);
    if (input == nullptr || out.empty()) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{m_request_mutex};
    const auto it = m_requests.find(input->request_id);
    if (it == m_requests.end()) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }
    if (it->second.result < 0) {
        return it->second.result;
    }

    // The request's result holds the issuer id.
    return WriteOutput(out[0], &it->second.result, sizeof(it->second.result));
}

// sceNpManagerIntCheckNpAvailability
s32 NpMgrIpcServer::SendCheckNpAvailabilityRequest(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<CheckNpAvailabilityInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{m_request_mutex};
    const auto it = m_requests.find(input->request_id);
    if (it == m_requests.end()) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }

    it->second.result = 0;
    SignalRequest(it->second);
    return 0;
}

// sceNpManagerIntCheckNpAvailability
s32 NpMgrIpcServer::SendCheckUserNpAvailabilityRequest(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<CheckUserNpAvailabilityInput>(in, 0);
    if (input == nullptr || input->user_id == -1) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    const bool signed_in = NpHandler::Instance().IsSignedIn(input->user_id);
    std::scoped_lock lk{m_request_mutex};
    const auto it = m_requests.find(input->request_id);
    if (it == m_requests.end()) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }

    it->second.result = signed_in ? 0 : ORBIS_NP_ERROR_SIGNED_OUT;
    SignalRequest(it->second);
    return 0;
}

// sceNpManagerIntCheckNpAvailability
s32 NpMgrIpcServer::CheckNpAvailabilityResult(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<RequestIdInput>(in, 0);
    if (input == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{m_request_mutex};
    const auto it = m_requests.find(input->request_id);
    if (it == m_requests.end()) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }

    return std::min(it->second.result, 0);
}

// sceNpManagerIntGetGameAccessToken
s32 NpMgrIpcServer::SendGetGameAccessTokenRequest(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<RequestUserInput>(in, 0);
    // const auto* client_id = InputAs<OrbisNpClientId>(in, 1);
    // const auto* title_token = InputAs<OrbisNpTitleToken>(in, 2);
    // const auto* scope = InputAs<OrbisNpScope>(in, 3);
    if (input == nullptr || input->user_id == -1) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    const bool signed_in = NpHandler::Instance().IsSignedIn(input->user_id);
    std::scoped_lock lk{m_request_mutex};
    const auto it = m_requests.find(input->request_id);
    if (it == m_requests.end()) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }

    it->second.result = signed_in ? 0 : ORBIS_NP_ERROR_SIGNED_OUT;
    it->second.user_id = input->user_id;
    SignalRequest(it->second);
    return 0;
}

// sceNpManagerIntGetGameAccessToken
s32 NpMgrIpcServer::GetGameAccessTokenResult(std::span<const IpmiDataInfo> in,
                                             std::span<IpmiBufferInfo> out) {
    const auto* input = InputAs<RequestIdInput>(in, 0);
    if (input == nullptr || out.empty()) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    s32 result;
    s32 user_id;
    {
        std::scoped_lock lk{m_request_mutex};
        const auto it = m_requests.find(input->request_id);
        if (it == m_requests.end()) {
            return ORBIS_NP_IPC_ERROR_INTERNAL;
        }
        result = it->second.result;
        user_id = it->second.user_id;
    }
    if (result < 0) {
        return result;
    }

    // Use the shadNet bearer token as our access token
    const std::string bearer = NpHandler::Instance().GetBearerToken(user_id);
    OrbisNpGameAccessToken token{};
    bearer.copy(token.token, sizeof(token.token) - 1);
    token.expiration_time = TokenExpiration();
    return WriteOutput(out[0], &token, sizeof(token));
}

// sceNpManagerIntAuthGetAuthorizationCode
s32 NpMgrIpcServer::SendGetAuthorizationCodeRequest(std::span<const IpmiDataInfo> in) {
    const auto* input = InputAs<GetAuthorizationCodeInput>(in, 0);
    const auto* client_id = InputAs<OrbisNpClientId>(in, 1);
    if (input == nullptr || client_id == nullptr || in.size() < 3 || in[2].data == nullptr ||
        input->user_id == -1) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    auto pending = std::make_shared<PendingAuthorizationCode>();
    pending->ctx.auth_code = &pending->code;
    std::chrono::microseconds timeout{};
    {
        std::scoped_lock lk{m_request_mutex};
        const auto it = m_requests.find(input->request_id);
        if (it == m_requests.end()) {
            return ORBIS_NP_IPC_ERROR_INTERNAL;
        }
        it->second.user_id = input->user_id;
        it->second.pending_auth = pending;
        timeout = RequestTimeout(it->second.timeouts);
    }

    const std::string client(client_id->id, strnlen(client_id->id, sizeof(client_id->id)));
    const std::string scope(static_cast<const char*>(in[2].data),
                            strnlen(static_cast<const char*>(in[2].data), in[2].size));
    const std::shared_ptr<AuthRequestCtx> ctx{pending, &pending->ctx};
    const s32 ret =
        NpHandler::Instance().RequestAuthorizationCode(input->user_id, client, scope, ctx);
    if (ret < 0) {
        CompleteAuthorizationCodeRequest(input->request_id, pending, ret);
        return 0;
    }

    // shadNet replies asynchronously, finish the request once it does or the request times out.
    std::thread([weak_self = weak_from_this(), request_id = input->request_id, pending, timeout] {
        if (!pending->ctx.WaitFor(timeout)) {
            pending->ctx.SetResult(ORBIS_NP_ERROR_TIMEOUT);
        }
        if (const auto self = weak_self.lock()) {
            self->CompleteAuthorizationCodeRequest(request_id, pending, pending->ctx.Wait());
        }
    }).detach();
    return 0;
}

void NpMgrIpcServer::CompleteAuthorizationCodeRequest(
    s32 request_id, const std::shared_ptr<PendingAuthorizationCode>& pending, s32 result) {
    std::scoped_lock lk{m_request_mutex};
    const auto it = m_requests.find(request_id);
    if (it == m_requests.end() || it->second.pending_auth != pending) {
        return;
    }
    Request& request = it->second;
    request.result = result;
    if (result >= 0) {
        request.auth_code = pending->code;
    }
    request.pending_auth.reset();
    SignalRequest(request);
}

// sceNpManagerIntAuthGetAuthorizationCode
s32 NpMgrIpcServer::GetAuthorizationCodeResult(std::span<const IpmiDataInfo> in,
                                               std::span<IpmiBufferInfo> out) {
    const auto* input = InputAs<RequestIdInput>(in, 0);
    if (input == nullptr || out.empty()) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{m_request_mutex};
    const auto it = m_requests.find(input->request_id);
    if (it == m_requests.end()) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }
    if (it->second.result < 0) {
        return it->second.result;
    }
    return WriteOutput(out[0], &it->second.auth_code, sizeof(it->second.auth_code));
}

} // namespace Libraries::Kernel::Ipmi
