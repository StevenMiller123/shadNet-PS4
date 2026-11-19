// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <set>
#include <magic_enum/magic_enum.hpp>
#include <orbis/libkernel.h>
#include <pthread.h>
#include <pthread_np.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/kernel/ipmi/ipmi_server.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/kernel_ipmi.h"

extern "C" void ipmimgr_call();

SHADNET_HOOK_DECLARE(Libraries::Kernel, ipmimgr_call);

static void RegisterKernelIpmiHooks() {
    SHADNET_HOOK(Libraries::Kernel, ipmimgr_call);
}

namespace Libraries::Kernel {

namespace {

std::mutex g_ipmi_mutex;
std::unordered_map<u32, std::string> g_ipmi_clients;
std::set<std::tuple<std::string, IpmiOp, u32>> g_ipmi_traced;
std::unordered_map<u32, std::shared_ptr<Ipmi::Server>> g_ipmi_emulated;
u32 g_ipmi_next_emulated_kid = 0x40000001;

// IPMI is used for much more than just NP calls.
// This global array is clients our hook should ignore.
constexpr std::array<std::string_view, 18> IgnoredClients = {
    "SceUserService",   "SceNetCtl",
    "SceAvSettingIpc",  "SceMbusIpc",
    "SceLncService",    "SceAppContent",
    "SceSaveData",      "SceGlsIpc",
    "SceAvCaptureIpc",  "SceScreenShot",
    "SceSharePlayIpc",  "ScePlayGo",
    "SceRemoteplayIpc", "SceSysAudioSystemIpc",
    "SceNpTrophyIpc",   "SceImeService",
    "SceNpEntAccess",   "SceNpGameIntent"};

bool IsIgnoredClient(std::string_view name) {
    return std::find(IgnoredClients.begin(), IgnoredClients.end(), name) != IgnoredClients.end();
}

bool IsClientOp(IpmiOp op) {
    switch (op) {
    case IpmiOp::DestroyClient:
    case IpmiOp::ClientInvokeAsyncMethod:
    case IpmiOp::ClientTryGetResult:
    case IpmiOp::ClientGetMessage:
    case IpmiOp::ClientTryGetMessage:
    case IpmiOp::ClientDisconnect:
    case IpmiOp::ClientInvokeSyncMethod:
    case IpmiOp::ClientConnect:
    case IpmiOp::ClientGetName:
    case IpmiOp::ClientWaitEventFlag:
    case IpmiOp::ClientPollEventFlag:
    case IpmiOp::ClientGetUserData:
    case IpmiOp::ClientTerminateConnection:
        return true;
    default:
        return false;
    }
}

bool IsPollingOp(IpmiOp op) {
    switch (op) {
    case IpmiOp::ClientTryGetResult:
    case IpmiOp::ClientGetMessage:
    case IpmiOp::ClientTryGetMessage:
    case IpmiOp::ClientWaitEventFlag:
    case IpmiOp::ClientPollEventFlag:
        return true;
    default:
        return false;
    }
}

// The method id is the first field of the Invoke and TryGetResult params.
std::optional<u32> IpmiMethod(IpmiOp op, const void* params, u64 params_size) {
    switch (op) {
    case IpmiOp::ClientInvokeSyncMethod:
    case IpmiOp::ClientInvokeAsyncMethod:
    case IpmiOp::ClientTryGetResult:
        if (params != nullptr && params_size >= sizeof(u32)) {
            return *static_cast<const u32*>(params);
        }
        return std::nullopt;
    default:
        return std::nullopt;
    }
}

std::string ClientName(u32 kid) {
    {
        std::scoped_lock lk{g_ipmi_mutex};
        if (const auto it = g_ipmi_clients.find(kid); it != g_ipmi_clients.end()) {
            return it->second;
        }
    }

    // Clients may be created before we made this hook.
    // Run ipmimgr_call ourselves to find the name.
    char name[0x40]{};
    IpmiGetNameParams params{name};
    u32 result = 0;
    const s32 ret = SHADNET_HOOK_CONTINUE(ipmimgr_call, IpmiOp::ClientGetName, kid, &result,
                                          &params, sizeof(params), IpmiCookie);
    std::string found =
        ret == 0 && result == 0 ? std::string(name, strnlen(name, sizeof(name))) : std::string{};

    std::shared_ptr<Ipmi::Server> adopted;
    if (!found.empty()) {
        // Emulate this client with our own fake server
        void* user_data = nullptr;
        IpmiGetUserDataParams user_params{&user_data};
        u32 user_result = 0;
        SHADNET_HOOK_CONTINUE(ipmimgr_call, IpmiOp::ClientGetUserData, kid, &user_result,
                              &user_params, sizeof(user_params), IpmiCookie);
        adopted = Ipmi::CreateServer(found, user_data);
        if (adopted != nullptr) {
            LOG_INFO(Kernel_Ipmi, "{} kid {:#x} emulated", found, kid);
            adopted->OnAdopted(kid);
        }
    }

    std::scoped_lock lk{g_ipmi_mutex};
    g_ipmi_clients.emplace(kid, found);
    if (adopted != nullptr) {
        g_ipmi_emulated.emplace(kid, std::move(adopted));
    }
    return found;
}

// sceKernelPrintBacktraceWithModuleInfo can't be used because it breaks during plugin_start.
std::string Backtrace() {
    pthread_attr_t attr;
    void* stack_addr = nullptr;
    u64 stack_size = 0;
    pthread_attr_init(&attr);
    const bool have_stack = pthread_attr_get_np(pthread_self(), &attr) == 0 &&
                            pthread_attr_getstack(&attr, &stack_addr, &stack_size) == 0;
    pthread_attr_destroy(&attr);
    if (!have_stack) {
        return " stack bounds unavailable";
    }

    const auto low = reinterpret_cast<uintptr_t>(stack_addr);
    const auto high = low + stack_size;
    std::string out;
    auto fp = reinterpret_cast<uintptr_t>(__builtin_frame_address(0));
    for (u32 depth = 0; depth < 64; ++depth) {
        if (fp < low || fp + 2 * sizeof(uintptr_t) > high || fp % sizeof(uintptr_t) != 0) {
            break;
        }
        const auto* frame = reinterpret_cast<const uintptr_t*>(fp);
        const uintptr_t ret = frame[1];
        if (ret == 0) {
            break;
        }

        OrbisKernelModuleInfoEx info{};
        info.size = sizeof(info);
        if (sceKernelGetModuleInfoFromAddr(reinterpret_cast<void*>(ret), 1, &info) == 0 &&
            info.segmentCount > 0) {
            out += fmt::format("\n    {:#018x} => {}+{:#x}", ret, info.name,
                               ret - reinterpret_cast<uintptr_t>(info.segmentInfo[0].address));
        } else {
            out += fmt::format("\n    {:#018x}", ret);
        }

        if (frame[0] <= fp) {
            break;
        }
        fp = frame[0];
    }
    return out;
}

void PrintFirstBacktrace(const std::string& name, IpmiOp op, std::optional<u32> method) {
    {
        std::scoped_lock lk{g_ipmi_mutex};
        if (!g_ipmi_traced.emplace(name, op, method.value_or(0)).second) {
            return;
        }
    }
    const std::string tag =
        method ? fmt::format("IPMI {} {} {:#x}", name, magic_enum::enum_name(op), *method)
               : fmt::format("IPMI {} {}", name, magic_enum::enum_name(op));
    LOG_INFO(Kernel_Ipmi, "{} backtrace:{}", tag, Backtrace());
}

std::shared_ptr<Ipmi::Server> FindEmulatedServer(u32 kid) {
    std::scoped_lock lk{g_ipmi_mutex};
    const auto it = g_ipmi_emulated.find(kid);
    return it != g_ipmi_emulated.end() ? it->second : nullptr;
}

template <typename T>
const T* ParamsAs(const void* params, u64 params_size) {
    return params != nullptr && params_size == sizeof(T) ? static_cast<const T*>(params) : nullptr;
}

s32 DispatchEmulated(Ipmi::Server& server, IpmiOp op, u32 kid, const void* params,
                     u64 params_size) {
    switch (static_cast<IpmiOp>(op)) {
    case IpmiOp::DestroyClient: {
        std::scoped_lock lk{g_ipmi_mutex};
        g_ipmi_emulated.erase(kid);
        g_ipmi_clients.erase(kid);
        return 0;
    }
    case IpmiOp::ClientConnect:
        if (const auto* p = ParamsAs<IpmiClientConnectParams>(params, params_size)) {
            return server.Connect(p->user_data, p->user_data_size, p->status);
        }
        break;
    case IpmiOp::ClientDisconnect:
        if (const auto* p = ParamsAs<IpmiDisconnectParams>(params, params_size)) {
            return server.Disconnect(p->status);
        }
        break;
    case IpmiOp::ClientTerminateConnection:
        return server.TerminateConnection();
    case IpmiOp::ClientInvokeSyncMethod:
        if (const auto* p = ParamsAs<IpmiSyncCallParams>(params, params_size);
            p != nullptr && p->flags <= 1) {
            return server.InvokeSyncMethod(p->method, {p->in_data, p->num_in_data},
                                           {p->out_data, p->num_out_data}, p->result);
        }
        break;
    case IpmiOp::ClientInvokeAsyncMethod:
        if (const auto* p = ParamsAs<IpmiAsyncCallParams>(params, params_size);
            p != nullptr && p->flags <= 1) {
            return server.InvokeAsyncMethod(p->method, {p->in_data, p->num_in_data}, p->result);
        }
        break;
    case IpmiOp::ClientTryGetResult:
        if (const auto* p = ParamsAs<IpmiTryGetResultParams>(params, params_size)) {
            return server.TryGetResult(p->method, p->result, {p->out_data, p->num_out_data});
        }
        break;
    case IpmiOp::ClientGetMessage:
        if (const auto* p = ParamsAs<IpmiGetMessageParams>(params, params_size)) {
            return server.GetMessage(p->queue_index, p->message, p->max_size, p->size, p->timeout);
        }
        break;
    case IpmiOp::ClientTryGetMessage:
        if (const auto* p = ParamsAs<IpmiTryGetMessageParams>(params, params_size)) {
            return server.TryGetMessage(p->queue_index, p->message, p->max_size, p->size);
        }
        break;
    case IpmiOp::ClientWaitEventFlag:
        if (const auto* p = ParamsAs<IpmiWaitEventFlagParams>(params, params_size)) {
            return server.WaitEventFlag(p->index, p->pattern, p->mode, p->result_pattern,
                                        p->timeout);
        }
        break;
    case IpmiOp::ClientPollEventFlag:
        if (const auto* p = ParamsAs<IpmiPollEventFlagParams>(params, params_size)) {
            return server.PollEventFlag(p->index, p->pattern, p->mode, p->result_pattern);
        }
        break;
    case IpmiOp::ClientGetUserData:
        if (const auto* p = ParamsAs<IpmiGetUserDataParams>(params, params_size)) {
            return server.GetUserData(p->user_data);
        }
        break;
    case IpmiOp::ClientGetName:
        if (const auto* p = ParamsAs<IpmiGetNameParams>(params, params_size)) {
            const auto& name = server.GetName();
            std::memcpy(p->name, name.c_str(), name.size() + 1);
            return 0;
        }
        break;
    default:
        LOG_ERROR(Kernel_Ipmi, "{} {} ({:#x}) not implemented yet", server.GetName(),
                  magic_enum::enum_name(op), op);
        return ORBIS_KERNEL_ERROR_ENOSYS;
    }

    LOG_ERROR(Kernel_Ipmi, "{} {} with bad params (size {:#x})", server.GetName(),
              magic_enum::enum_name(op), params_size);
    return ORBIS_KERNEL_ERROR_EINVAL;
}

} // namespace

s32 ipmimgr_call(IpmiOp op, u32 kid, u32* result, void* params, u64 params_size, u64 cookie) {
    if (op == IpmiOp::CreateClient) {
        std::string name;
        if (params != nullptr && params_size == sizeof(IpmiCreateClientParams)) {
            const auto* create = static_cast<const IpmiCreateClientParams*>(params);
            if (create->name != nullptr) {
                name = create->name;
            }
        }
        const bool ignored = IsIgnoredClient(name);
        if (!ignored) {
            PrintFirstBacktrace(name, op, std::nullopt);
        }

        // Serve this client from our emulated server, if we have one
        void* user_data = nullptr;
        if (const auto* create = ParamsAs<IpmiCreateClientParams>(params, params_size);
            create != nullptr && create->config != nullptr) {
            user_data = create->config->user_data;
        }
        if (auto server = Ipmi::CreateServer(name, user_data)) {
            u32 emulated_kid;
            {
                std::scoped_lock lk{g_ipmi_mutex};
                emulated_kid = g_ipmi_next_emulated_kid++;
                g_ipmi_emulated.insert_or_assign(emulated_kid, std::move(server));
                g_ipmi_clients.insert_or_assign(emulated_kid, name);
            }
            LOG_INFO(Kernel_Ipmi, "created emulated {} client, kid {:#x}", name, emulated_kid);
            *result = emulated_kid;
            return 0;
        }

        // We don't emulate this server, fall back on the firmware's client.
        const s32 ret =
            SHADNET_HOOK_CONTINUE(ipmimgr_call, op, kid, result, params, params_size, cookie);
        const u32 res = result != nullptr ? *result : 0;
        if (!ignored) {
            LOG_INFO(Kernel_Ipmi, "created real {} client, ret {:#x}, result {:#x}", name, ret,
                     res);
        }
        if (ret == 0 && result != nullptr && (res & 0x80000000) == 0) {
            std::scoped_lock lk{g_ipmi_mutex};
            g_ipmi_clients.insert_or_assign(res, name);
        }
        return ret;
    }

    if (!IsClientOp(op)) {
        const s32 ret =
            SHADNET_HOOK_CONTINUE(ipmimgr_call, op, kid, result, params, params_size, cookie);
        LOG_DEBUG(Kernel_Ipmi, "kid {:#x} {} ({:#x}) returned {:#x}, outputted result {:#x}", kid,
                  magic_enum::enum_name(op), op, ret, result != nullptr ? *result : 0);
        return ret;
    }

    const std::string name = ClientName(kid);
    auto server = FindEmulatedServer(kid);
    if (server == nullptr && IsIgnoredClient(name)) {
        const s32 ret =
            SHADNET_HOOK_CONTINUE(ipmimgr_call, op, kid, result, params, params_size, cookie);
        if (op == IpmiOp::DestroyClient && ret == 0) {
            std::scoped_lock lk{g_ipmi_mutex};
            g_ipmi_clients.erase(kid);
        }
        return ret;
    }

    const auto method = IpmiMethod(op, params, params_size);
    if (server != nullptr) {
        // Run our emulated ipmimgr_call behavior
        const s32 res = DispatchEmulated(*server, op, kid, params, params_size);
        s32 method_result = 0;
        if (const auto* p = ParamsAs<IpmiSyncCallParams>(params, params_size);
            op == IpmiOp::ClientInvokeSyncMethod && p != nullptr && p->result != nullptr) {
            method_result = *p->result;
        }
        if (IsPollingOp(op)) {
            LOG_TRACE(Kernel_Ipmi,
                      "emulated client {}: operation {} method {:#x} returned result {:#x}", name,
                      magic_enum::enum_name(op), method.value_or(0), res);
        } else {
            if (method_result >= 0) {
                LOG_DEBUG(
                    Kernel_Ipmi,
                    "emulated client {}: operation {} method {:#x} returned result {:#x}, method "
                    "result {:#x}",
                    name, magic_enum::enum_name(op), method.value_or(0), res, method_result);
            } else if (method_result < 0) {
                LOG_ERROR(
                    Kernel_Ipmi,
                    "emulated client {}: operation {} method {:#x} returned result {:#x}, method "
                    "result {:#x}",
                    name, magic_enum::enum_name(op), method.value_or(0), res, method_result);
                PrintFirstBacktrace(name, op, method);
            }
        }
        *result = static_cast<u32>(res);
        return 0;
    }

    // We don't emulate this client's server, run the real ipmimgr_call instead.
    const s32 ret =
        SHADNET_HOOK_CONTINUE(ipmimgr_call, op, kid, result, params, params_size, cookie);
    const u32 res = result != nullptr ? *result : 0;
    if (res < 0 && !IsIgnoredClient(name)) {
        PrintFirstBacktrace(name, op, method);
    }

    if (IsPollingOp(op)) {
        LOG_DEBUG(Kernel_Ipmi,
                  "real client {}: operation {} method {:#x} returned result {:#x}, method "
                  "result {:#x}",
                  name, magic_enum::enum_name(op), method.value_or(0), ret, res);
    } else if (method) {
        LOG_INFO(Kernel_Ipmi,
                 "real client {}: operation {} method {:#x} returned result {:#x}, method "
                 "result {:#x}",
                 name, magic_enum::enum_name(op), *method, ret, res);
    } else {
        LOG_INFO(Kernel_Ipmi,
                 "real client {}: operation {} returned result {:#x}, method result {:#x}", name,
                 magic_enum::enum_name(op), ret, res);
    }

    if (op == IpmiOp::DestroyClient && ret == 0) {
        std::scoped_lock lk{g_ipmi_mutex};
        g_ipmi_clients.erase(kid);
    }
    return ret;
}

s32 InvokeRealSyncMethod(u32 kid, u32 method, std::span<const IpmiDataInfo> in,
                         std::span<IpmiBufferInfo> out, s32* method_result) {
    IpmiSyncCallParams params{};
    params.method = method;
    params.num_in_data = static_cast<u32>(in.size());
    params.num_out_data = static_cast<u32>(out.size());
    params.in_data = in.data();
    params.out_data = out.data();
    params.result = method_result;
    // libSceIpmi sets this for calls with DataInfo and BufferInfo arrays, servers assert on it.
    params.flags = 1;
    u32 result = 0;
    // libSceIpmi passes the method id as the cookie for sync calls.
    const s32 ret = SHADNET_HOOK_CONTINUE(ipmimgr_call, IpmiOp::ClientInvokeSyncMethod, kid,
                                          &result, &params, sizeof(params), method);
    return ret < 0 ? ret : static_cast<s32>(result);
}

void RegisterIpmiHooks() {
    return RegisterKernelIpmiHooks();
}

} // namespace Libraries::Kernel
