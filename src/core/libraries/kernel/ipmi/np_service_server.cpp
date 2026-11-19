// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>

#include "common/logging/log.h"
#include "core/libraries/kernel/ipmi/np_service/asm_client/asm_client.h"
#include "core/libraries/kernel/ipmi/np_service/basic_presence/basic_presence.h"
#include "core/libraries/kernel/ipmi/np_service/id_mapper/id_mapper.h"
#include "core/libraries/kernel/ipmi/np_service/in_game_msg/in_game_msg.h"
#include "core/libraries/kernel/ipmi/np_service/match2_sig/match2_sig.h"
#include "core/libraries/kernel/ipmi/np_service/presence/presence.h"
#include "core/libraries/kernel/ipmi/np_service/push/push.h"
#include "core/libraries/kernel/ipmi/np_service/push2/push2.h"
#include "core/libraries/kernel/ipmi/np_service/score/score.h"
#include "core/libraries/kernel/ipmi/np_service/signaling/signaling.h"
#include "core/libraries/kernel/ipmi/np_service/sns/sns.h"
#include "core/libraries/kernel/ipmi/np_service/tpip/tpip.h"
#include "core/libraries/kernel/ipmi/np_service/tus/tus.h"
#include "core/libraries/kernel/ipmi/np_service/web_api2/web_api2.h"
#include "core/libraries/kernel/ipmi/np_service_server.h"
#include "core/libraries/np/np_error.h"

namespace Libraries::Kernel::Ipmi {

namespace {

// Methods handled by the NpServiceServer itself
enum class Method : u32 {
    OpenService = 0x0,
    CloseService = 0x1,
    CreateRequest = 0x2,
    StartRequest = 0x3,
    DestroyRequest = 0x4,
    AbortRequest = 0x5,
    DequeueEvent = 0x6,
};

// First method value belonging to a Service
constexpr u32 FirstServiceMethod = 0x10000;

struct ServiceIdInput {
    u32 service_id;
};

// Request flags, non-blocking requests aren't waited on.
constexpr u32 RequestNonBlocking = 0x1;

struct CreateRequestInput {
    u32 service_id;
    u32 flags;
    u8 params[0x80];
    u64 params_size;
};
static_assert(sizeof(CreateRequestInput) == 0x90);

struct StartRequestInput {
    u32 service_id;
    s32 request_id;
    s32 ipc_slot;
};

struct RequestInput {
    u32 service_id;
    u32 flags;
    s32 request_id;
};

} // namespace

NpServiceServer::NpServiceServer(void* user_data) : Server(std::string(ServiceName), user_data) {
    m_services.push_back(std::make_unique<NpService::TusService>());
    m_services.push_back(std::make_unique<NpService::ScoreService>());
    m_services.push_back(std::make_unique<NpService::PushService>());
    m_services.push_back(std::make_unique<NpService::PresenceService>());
    m_services.push_back(std::make_unique<NpService::BasicPresenceService>());
    m_services.push_back(std::make_unique<NpService::InGameMsgService>());
    m_services.push_back(std::make_unique<NpService::SnsService>());
    m_services.push_back(std::make_unique<NpService::SignalingService>());
    m_services.push_back(std::make_unique<NpService::AsmClientService>());
    m_services.push_back(std::make_unique<NpService::Match2SigService>());
    m_services.push_back(std::make_unique<NpService::IdMapperService>());
    m_services.push_back(std::make_unique<NpService::TpipService>());
    m_services.push_back(std::make_unique<NpService::Push2Service>());
    m_services.push_back(std::make_unique<NpService::WebApi2Service>());

    // Every service is notified of a client connecting.
    for (const auto& service : m_services) {
        service->SetEventFlagSetter(
            [this](u32 index, u64 pattern) { SetEventFlag(index, pattern); });
        service->OnClientConnected();
    }
}

NpServiceServer::~NpServiceServer() {
    for (const auto& service : m_services) {
        service->OnClientDisconnected();
    }
}

s32 NpServiceServer::InvokeSyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                                      std::span<IpmiBufferInfo> out, s32* method_result) {
    s32 res;
    if (method >= FirstServiceMethod) {
        NpService::Service* service = FindOpenService(method);
        if (service == nullptr) {
            LOG_ERROR(Kernel_Ipmi, "{} sync method {:#x} has no service", GetName(), method);
            res = ORBIS_NP_IPC_ERROR_SERVICE_NOT_INITIALIZED;
        } else {
            res = service->InvokeSyncMethod(method, in, out);
        }
    } else {
        switch (static_cast<Method>(method)) {
        case Method::OpenService:
            res = OpenService(in);
            break;
        case Method::CloseService:
            res = CloseService(in);
            break;
        case Method::CreateRequest:
            res = CreateRequest(in);
            break;
        case Method::StartRequest:
            res = StartRequest(in);
            break;
        case Method::DestroyRequest:
            res = DestroyRequest(in);
            break;
        case Method::AbortRequest:
            res = AbortRequest(in);
            break;
        default:
            return Server::InvokeSyncMethod(method, in, out, method_result);
        }
    }

    if (method_result != nullptr) {
        *method_result = res;
    }
    return 0;
}

s32 NpServiceServer::InvokeAsyncMethod(u32 method, std::span<const IpmiDataInfo> in,
                                       s32* method_result) {
    NpService::Service* service = method >= FirstServiceMethod ? FindOpenService(method) : nullptr;
    if (service == nullptr) {
        return Server::InvokeAsyncMethod(method, in, method_result);
    }

    PushAsyncResult(method, service->InvokeAsyncMethod(method, in), {});
    if (method_result != nullptr) {
        *method_result = 0;
    }
    return 0;
}

// sce::np::ipc::ServiceIpmiClient::CreateRequest
s32 NpServiceServer::CreateRequest(std::span<const IpmiDataInfo> in) {
    if (in.empty() || in[0].data == nullptr || in[0].size < sizeof(CreateRequestInput)) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }
    const auto* input = static_cast<const CreateRequestInput*>(in[0].data);
    NpService::Service* service = FindOpenServiceById(input->service_id);
    if (service == nullptr) {
        return ORBIS_NP_IPC_ERROR_SERVICE_NOT_INITIALIZED;
    }

    const std::span<const u8> params{input->params,
                                     std::min<u64>(input->params_size, sizeof(input->params))};
    const bool blocking = (input->flags & RequestNonBlocking) == 0;
    s32 request_id = -1;
    if (const s32 ret = service->CreateRequest(params, blocking, &request_id); ret < 0) {
        return ret;
    }
    LOG_INFO(Kernel_Ipmi, "{} created request {} ({} bytes of params, flags {:#x})",
             service->GetName(), request_id, params.size(), input->flags);
    return request_id;
}

// sce::np::ipc::ServiceIpmiClient::StartRequest
s32 NpServiceServer::StartRequest(std::span<const IpmiDataInfo> in) {
    if (in.empty() || in[0].data == nullptr || in[0].size < sizeof(StartRequestInput)) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }
    const auto* input = static_cast<const StartRequestInput*>(in[0].data);
    NpService::Service* service = FindOpenServiceById(input->service_id);
    if (service == nullptr) {
        return ORBIS_NP_IPC_ERROR_SERVICE_NOT_INITIALIZED;
    }
    return service->StartRequest(input->request_id, input->ipc_slot);
}

// sce::np::ipc::ServiceIpmiClient::DestroyRequest
s32 NpServiceServer::DestroyRequest(std::span<const IpmiDataInfo> in) {
    if (in.empty() || in[0].data == nullptr || in[0].size < sizeof(RequestInput)) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }
    const auto* input = static_cast<const RequestInput*>(in[0].data);
    NpService::Service* service = FindOpenServiceById(input->service_id);
    if (service == nullptr) {
        return ORBIS_NP_IPC_ERROR_SERVICE_NOT_INITIALIZED;
    }
    return service->DestroyRequest(input->request_id, (input->flags & RequestNonBlocking) == 0);
}

// sce::np::ipc::ServiceIpmiClient::AbortRequest
s32 NpServiceServer::AbortRequest(std::span<const IpmiDataInfo> in) {
    if (in.empty() || in[0].data == nullptr || in[0].size < sizeof(RequestInput)) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }
    const auto* input = static_cast<const RequestInput*>(in[0].data);
    NpService::Service* service = FindOpenServiceById(input->service_id);
    if (service == nullptr) {
        return ORBIS_NP_IPC_ERROR_SERVICE_NOT_INITIALIZED;
    }
    return service->AbortRequest(input->request_id, (input->flags & RequestNonBlocking) == 0);
}

s32 NpServiceServer::OpenService(std::span<const IpmiDataInfo> in) {
    if (in.empty() || in[0].data == nullptr || in[0].size < sizeof(ServiceIdInput)) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }
    const u32 id = static_cast<const ServiceIdInput*>(in[0].data)->service_id;

    NpService::Service* service = FindService(id);
    if (service == nullptr) {
        return ORBIS_NP_IPC_ERROR_SERVICE_NOT_FOUND;
    }

    {
        std::scoped_lock lk{m_mutex};
        if (m_open_services.contains(id)) {
            return ORBIS_NP_IPC_ERROR_SERVICE_ALREADY_INITIALIZED;
        }
        if (m_open_services.size() >= MaxOpenServices) {
            return ORBIS_NP_IPC_ERROR_TOO_MANY_SERVICE;
        }
        m_open_services.insert(id);
    }
    service->OnOpened();
    return 0;
}

s32 NpServiceServer::CloseService(std::span<const IpmiDataInfo> in) {
    if (in.empty() || in[0].data == nullptr || in[0].size < sizeof(ServiceIdInput)) {
        return ORBIS_NP_IPC_ERROR_INTERNAL;
    }
    const u32 id = static_cast<const ServiceIdInput*>(in[0].data)->service_id;

    {
        std::scoped_lock lk{m_mutex};
        if (m_open_services.erase(id) == 0) {
            return ORBIS_NP_IPC_ERROR_SERVICE_NOT_INITIALIZED;
        }
    }
    if (NpService::Service* service = FindService(id)) {
        service->OnClosed();
    }
    return 0;
}

NpService::Service* NpServiceServer::FindService(u32 id) {
    for (const auto& service : m_services) {
        if (service->GetId() == id) {
            return service.get();
        }
    }
    return nullptr;
}

NpService::Service* NpServiceServer::FindOpenServiceById(u32 id) {
    NpService::Service* service = FindService(id);
    if (service == nullptr) {
        return nullptr;
    }
    {
        std::scoped_lock lk{m_mutex};
        if (!m_open_services.insert(id).second) {
            return service;
        }
    }
    LOG_INFO(Kernel_Ipmi, "{} service {} was opened before the hook, opening it", GetName(),
             service->GetName());
    service->OnOpened();
    return service;
}

NpService::Service* NpServiceServer::FindOpenService(u32 method) {
    NpService::Service* found = nullptr;
    for (const auto& service : m_services) {
        if (service->HandlesMethod(method)) {
            found = service.get();
            break;
        }
    }
    if (found == nullptr) {
        return nullptr;
    }

    {
        std::scoped_lock lk{m_mutex};
        if (!m_open_services.insert(found->GetId()).second) {
            return found;
        }
    }
    found->OnOpened();
    return found;
}

} // namespace Libraries::Kernel::Ipmi
