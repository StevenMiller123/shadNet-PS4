// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "shadnet.pb.h"

namespace Libraries::Np {

std::vector<u8> BuildTusPayload(const std::string& com_id, const std::string& proto_bytes) {
    std::vector<u8> payload;
    payload.reserve(12 + 4 + proto_bytes.size());
    payload.insert(payload.end(), com_id.begin(), com_id.end());
    const u32 sz = static_cast<u32>(proto_bytes.size());
    payload.push_back(static_cast<u8>(sz));
    payload.push_back(static_cast<u8>(sz >> 8));
    payload.push_back(static_cast<u8>(sz >> 16));
    payload.push_back(static_cast<u8>(sz >> 24));
    payload.insert(payload.end(), proto_bytes.begin(), proto_bytes.end());
    return payload;
}

void CopyNpHandle(OrbisNpId& dst, const std::string& src) {
    dst = OrbisNpId{};
    const u64 cp = std::min(src.size(), sizeof(dst.handle.data) - 1);
    std::memcpy(dst.handle.data, src.data(), cp);
}

s32 NpHandler::TusGetMultiSlotVariable(
    s32 user_id, s32 service_label, const std::string& owner_np_id, const std::string& virtual_user,
    const std::vector<s32>& slot_ids, NpTus::OrbisNpTusVariable* variables_out, u64 array_num,
    std::shared_ptr<NpTus::TusRequestCtx> ctx, NpTus::OrbisNpTusVariableA* variables_a_out,
    s64 owner_account_id, NpTus::OrbisNpTusVariableForCrossSave* variables_cs_out) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    shadnet::TusGetMultiSlotVariableRequest proto;
    proto.set_ownernpid(owner_np_id);
    if (!virtual_user.empty()) {
        proto.set_virtualuser(virtual_user);
    } else if (owner_account_id != 0) {
        proto.set_owneraccountid(owner_account_id);
    } else if (owner_np_id.empty() ||
               Common::ToLower(owner_np_id) == Common::ToLower(client->GetNpid())) {
        proto.set_owneraccountid(static_cast<s64>(client->GetUserId()));
    }
    for (s32 s : slot_ids) {
        proto.add_slotids(s);
    }
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::TusGetMultiSlotVariable,
                                             BuildTusPayload(com_id, proto.SerializeAsString()));
    std::lock_guard lock(m_mutex_pending_tus);
    PendingTusRequest p;
    p.req = std::move(ctx);
    p.cmd = ShadNet::CommandType::TusGetMultiSlotVariable;
    p.variable_array = variables_out;
    p.variable_array_a = variables_a_out;
    p.variable_array_cs = variables_cs_out;
    p.array_num = array_num;
    p.user_id = user_id;
    m_pending_tus.emplace(pkt_id, std::move(p));
    return ORBIS_OK;
}

s32 NpHandler::TusSetMultiSlotVariable(
    s32 user_id, s32 service_label, const std::string& owner_np_id, const std::string& virtual_user,
    const std::vector<s32>& slot_ids, const std::vector<s64>& values,
    std::shared_ptr<NpTus::TusRequestCtx> ctx, s64 owner_account_id) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    shadnet::TusSetMultiSlotVariableRequest proto;
    proto.set_ownernpid(owner_np_id);
    if (!virtual_user.empty()) {
        proto.set_virtualuser(virtual_user);
    } else if (owner_account_id != 0) {
        proto.set_owneraccountid(owner_account_id);
    } else if (owner_np_id.empty() ||
               Common::ToLower(owner_np_id) == Common::ToLower(client->GetNpid())) {
        proto.set_owneraccountid(static_cast<s64>(client->GetUserId()));
    }

    for (s32 s : slot_ids) {
        proto.add_slotids(s);
    }
    for (s64 v : values) {
        proto.add_values(v);
    }
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::TusSetMultiSlotVariable,
                                             BuildTusPayload(com_id, proto.SerializeAsString()));
    std::lock_guard lock(m_mutex_pending_tus);
    PendingTusRequest p;
    p.req = std::move(ctx);
    p.cmd = ShadNet::CommandType::TusSetMultiSlotVariable;
    p.user_id = user_id;
    m_pending_tus.emplace(pkt_id, std::move(p));
    return ORBIS_OK;
}

s32 NpHandler::TssGetData(s32 user_id, s32 service_label, s32 slot_id, bool has_offset, u64 offset,
                          bool has_last_byte, u64 last_byte, bool has_if_param, s32 if_type,
                          u64 if_last_modified, NpTus::OrbisNpTssDataStatus* status_out,
                          void* data_out, u64 data_cap, std::shared_ptr<NpTus::TusRequestCtx> ctx,
                          u64* content_length_out) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    shadnet::TssGetDataRequest proto;
    proto.set_slotid(slot_id);
    if (has_offset) {
        proto.set_hasoffset(true);
        proto.set_offset(offset);
    }
    if (has_last_byte) {
        proto.set_haslastbyte(true);
        proto.set_lastbyte(last_byte);
    }
    if (has_if_param) {
        proto.set_hasifparam(true);
        proto.set_iftype(if_type);
        proto.set_iflastmodified(if_last_modified);
    }
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::TssGetData,
                                             BuildTusPayload(com_id, proto.SerializeAsString()));
    std::lock_guard lock(m_mutex_pending_tus);
    PendingTusRequest p;
    p.req = std::move(ctx);
    p.cmd = ShadNet::CommandType::TssGetData;
    p.user_id = user_id;
    p.tss_status_out = status_out;
    p.tss_content_length_out = content_length_out;
    p.data_out = data_out;
    p.data_cap = data_cap;
    m_pending_tus.emplace(pkt_id, std::move(p));
    return ORBIS_OK;
}

s32 NpHandler::TusGetData(s32 user_id, s32 service_label, const std::string& owner_np_id,
                          const std::string& virtual_user, s64 owner_account_id, s32 slot_id,
                          NpTus::OrbisNpTusDataStatusA* status_a_out, u64 status_cap,
                          void* data_out, u64 data_cap, u64 data_offset,
                          std::shared_ptr<NpTus::TusRequestCtx> ctx,
                          NpTus::OrbisNpTusDataStatus* status_out,
                          NpTus::OrbisNpTusDataStatusForCrossSave* status_cs_out) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    shadnet::TusGetDataRequest proto;
    proto.set_ownernpid(owner_np_id);
    proto.set_slotid(slot_id);
    if (!virtual_user.empty()) {
        proto.set_virtualuser(virtual_user);
    } else if (owner_account_id != 0) {
        proto.set_owneraccountid(owner_account_id);
    } else if (owner_np_id.empty() ||
               Common::ToLower(owner_np_id) == Common::ToLower(client->GetNpid())) {
        proto.set_owneraccountid(static_cast<s64>(client->GetUserId()));
    }
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::TusGetData,
                                             BuildTusPayload(com_id, proto.SerializeAsString()));
    std::lock_guard lock(m_mutex_pending_tus);
    PendingTusRequest p;
    p.req = std::move(ctx);
    p.cmd = ShadNet::CommandType::TusGetData;
    p.user_id = user_id;
    p.status_array_a = status_a_out;
    p.status_array = status_out;
    p.status_array_cs = status_cs_out;
    p.status_cap = status_cap;
    p.data_out = data_out;
    p.data_cap = data_cap;
    p.data_offset = data_offset;
    p.array_num = 1;
    m_pending_tus.emplace(pkt_id, std::move(p));
    return ORBIS_OK;
}

s32 NpHandler::TusDeleteMultiSlotData(s32 user_id, s32 service_label,
                                      const std::string& owner_np_id,
                                      const std::string& virtual_user, s64 owner_account_id,
                                      const std::vector<s32>& slot_ids,
                                      std::shared_ptr<NpTus::TusRequestCtx> ctx) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    shadnet::TusDeleteMultiSlotDataRequest proto;
    proto.set_ownernpid(owner_np_id);
    if (!virtual_user.empty()) {
        proto.set_virtualuser(virtual_user);
    } else if (owner_account_id != 0) {
        proto.set_owneraccountid(owner_account_id);
    } else if (owner_np_id.empty() ||
               Common::ToLower(owner_np_id) == Common::ToLower(client->GetNpid())) {
        proto.set_owneraccountid(static_cast<s64>(client->GetUserId()));
    }
    for (s32 s : slot_ids) {
        proto.add_slotids(s);
    }
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::TusDeleteMultiSlotData,
                                             BuildTusPayload(com_id, proto.SerializeAsString()));
    std::lock_guard lock(m_mutex_pending_tus);
    PendingTusRequest p;
    p.req = std::move(ctx);
    p.cmd = ShadNet::CommandType::TusDeleteMultiSlotData;
    p.user_id = user_id;
    m_pending_tus.emplace(pkt_id, std::move(p));
    return ORBIS_OK;
}

s32 NpHandler::TusDeleteMultiSlotVariable(s32 user_id, s32 service_label,
                                          const std::string& owner_np_id,
                                          const std::string& virtual_user, s64 owner_account_id,
                                          const std::vector<s32>& slot_ids,
                                          std::shared_ptr<NpTus::TusRequestCtx> ctx) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    shadnet::TusDeleteMultiSlotVariableRequest proto;
    proto.set_ownernpid(owner_np_id);
    if (!virtual_user.empty()) {
        proto.set_virtualuser(virtual_user);
    } else if (owner_account_id != 0) {
        proto.set_owneraccountid(owner_account_id);
    } else if (owner_np_id.empty() ||
               Common::ToLower(owner_np_id) == Common::ToLower(client->GetNpid())) {
        proto.set_owneraccountid(static_cast<s64>(client->GetUserId()));
    }
    for (s32 s : slot_ids) {
        proto.add_slotids(s);
    }
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::TusDeleteMultiSlotVariable,
                                             BuildTusPayload(com_id, proto.SerializeAsString()));
    std::lock_guard lock(m_mutex_pending_tus);
    PendingTusRequest p;
    p.req = std::move(ctx);
    p.cmd = ShadNet::CommandType::TusDeleteMultiSlotVariable;
    p.user_id = user_id;
    m_pending_tus.emplace(pkt_id, std::move(p));
    return ORBIS_OK;
}

s32 NpHandler::TusGetMultiUserDataStatus(s32 user_id, s32 service_label, s32 slot_id,
                                         const std::vector<std::string>& owner_np_ids,
                                         const std::vector<std::string>& virtual_users,
                                         const std::vector<s64>& owner_account_ids,
                                         NpTus::OrbisNpTusDataStatus* status_out,
                                         NpTus::OrbisNpTusDataStatusA* status_a_out, u64 array_num,
                                         std::shared_ptr<NpTus::TusRequestCtx> ctx,
                                         NpTus::OrbisNpTusDataStatusForCrossSave* status_cs_out) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    shadnet::TusGetMultiUserDataStatusRequest proto;
    proto.set_slotid(slot_id);
    for (const auto& vu : virtual_users) {
        proto.add_virtualusers(vu);
    }
    for (const auto& np : owner_np_ids) {
        proto.add_ownernpids(np);
    }
    for (s64 acc : owner_account_ids) {
        proto.add_owneraccountids(acc);
    }
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::TusGetMultiUserDataStatus,
                                             BuildTusPayload(com_id, proto.SerializeAsString()));
    std::lock_guard lock(m_mutex_pending_tus);
    PendingTusRequest p;
    p.req = std::move(ctx);
    p.cmd = ShadNet::CommandType::TusGetMultiUserDataStatus;
    p.user_id = user_id;
    p.status_array = status_out;
    p.status_array_a = status_a_out;
    p.status_array_cs = status_cs_out;
    p.array_num = array_num;
    m_pending_tus.emplace(pkt_id, std::move(p));
    return ORBIS_OK;
}

s32 NpHandler::TusGetMultiUserVariable(s32 user_id, s32 service_label, s32 slot_id,
                                       const std::vector<std::string>& owner_np_ids,
                                       const std::vector<std::string>& virtual_users,
                                       const std::vector<s64>& owner_account_ids,
                                       NpTus::OrbisNpTusVariable* variables_out,
                                       NpTus::OrbisNpTusVariableA* variables_a_out, u64 array_num,
                                       std::shared_ptr<NpTus::TusRequestCtx> ctx,
                                       NpTus::OrbisNpTusVariableForCrossSave* variables_cs_out) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    shadnet::TusGetMultiUserVariableRequest proto;
    proto.set_slotid(slot_id);
    for (const auto& vu : virtual_users) {
        proto.add_virtualusers(vu);
    }
    for (const auto& np : owner_np_ids) {
        proto.add_ownernpids(np);
    }
    for (s64 acc : owner_account_ids) {
        proto.add_owneraccountids(acc);
    }
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::TusGetMultiUserVariable,
                                             BuildTusPayload(com_id, proto.SerializeAsString()));
    std::lock_guard lock(m_mutex_pending_tus);
    PendingTusRequest p;
    p.req = std::move(ctx);
    p.cmd = ShadNet::CommandType::TusGetMultiUserVariable;
    p.user_id = user_id;
    p.variable_array = variables_out;
    p.variable_array_a = variables_a_out;
    p.variable_array_cs = variables_cs_out;
    p.array_num = array_num;
    m_pending_tus.emplace(pkt_id, std::move(p));
    return ORBIS_OK;
}

s32 NpHandler::TusTryAndSetVariable(
    s32 user_id, s32 service_label, const std::string& owner_np_id, const std::string& virtual_user,
    s64 owner_account_id, s32 slot_id, s32 ope_type, s64 value, bool has_compare, s64 compare_value,
    bool has_author_check, s64 is_last_changed_author,
    const std::string& is_last_changed_author_np_id, bool has_date_check, u64 is_last_changed_date,
    NpTus::OrbisNpTusVariable* variable_out, NpTus::OrbisNpTusVariableA* variable_a_out,
    std::shared_ptr<NpTus::TusRequestCtx> ctx,
    NpTus::OrbisNpTusVariableForCrossSave* variable_cs_out) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    shadnet::TusTryAndSetVariableRequest proto;
    proto.set_ownernpid(owner_np_id);
    proto.set_slotid(slot_id);
    proto.set_opetype(ope_type);
    proto.set_value(value);
    if (has_compare) {
        proto.set_hascompare(true);
        proto.set_comparevalue(compare_value);
    }
    if (!virtual_user.empty()) {
        proto.set_virtualuser(virtual_user);
    } else if (owner_account_id != 0) {
        proto.set_owneraccountid(owner_account_id);
    } else if (owner_np_id.empty() ||
               Common::ToLower(owner_np_id) == Common::ToLower(client->GetNpid())) {
        proto.set_owneraccountid(static_cast<s64>(client->GetUserId()));
    }
    if (has_author_check) {
        proto.set_hasauthorcheck(true);
        proto.set_islastchangedauthor(is_last_changed_author);
        if (!is_last_changed_author_np_id.empty()) {
            proto.set_islastchangedauthornpid(is_last_changed_author_np_id);
        }
    }
    if (has_date_check) {
        proto.set_hasdatecheck(true);
        proto.set_islastchangeddate(is_last_changed_date);
    }
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::TusTryAndSetVariable,
                                             BuildTusPayload(com_id, proto.SerializeAsString()));
    std::lock_guard lock(m_mutex_pending_tus);
    PendingTusRequest p;
    p.req = std::move(ctx);
    p.cmd = ShadNet::CommandType::TusTryAndSetVariable;
    p.user_id = user_id;
    p.variable_array = variable_out;
    p.variable_array_a = variable_a_out;
    p.variable_array_cs = variable_cs_out;
    p.array_num = 1;
    m_pending_tus.emplace(pkt_id, std::move(p));
    return ORBIS_OK;
}

s32 NpHandler::TusAddAndGetVariable(
    s32 user_id, s32 service_label, const std::string& owner_np_id, const std::string& virtual_user,
    s64 owner_account_id, s32 slot_id, s64 in_variable, bool has_author_check,
    s64 is_last_changed_author, const std::string& is_last_changed_author_np_id,
    bool has_date_check, u64 is_last_changed_date, NpTus::OrbisNpTusVariable* variable_out,
    NpTus::OrbisNpTusVariableA* variable_a_out, std::shared_ptr<NpTus::TusRequestCtx> ctx,
    NpTus::OrbisNpTusVariableForCrossSave* variable_cs_out) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    shadnet::TusAddAndGetVariableRequest proto;
    proto.set_ownernpid(owner_np_id);
    proto.set_slotid(slot_id);
    proto.set_invalue(in_variable);
    if (!virtual_user.empty()) {
        proto.set_virtualuser(virtual_user);
    } else if (owner_account_id != 0) {
        proto.set_owneraccountid(owner_account_id);
    } else if (owner_np_id.empty() ||
               Common::ToLower(owner_np_id) == Common::ToLower(client->GetNpid())) {
        proto.set_owneraccountid(static_cast<s64>(client->GetUserId()));
    }
    if (has_author_check) {
        proto.set_hasauthorcheck(true);
        proto.set_islastchangedauthor(is_last_changed_author);
        if (!is_last_changed_author_np_id.empty()) {
            proto.set_islastchangedauthornpid(is_last_changed_author_np_id);
        }
    }
    if (has_date_check) {
        proto.set_hasdatecheck(true);
        proto.set_islastchangeddate(is_last_changed_date);
    }
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::TusAddAndGetVariable,
                                             BuildTusPayload(com_id, proto.SerializeAsString()));
    std::lock_guard lock(m_mutex_pending_tus);
    PendingTusRequest p;
    p.req = std::move(ctx);
    p.cmd = ShadNet::CommandType::TusAddAndGetVariable;
    p.user_id = user_id;
    p.variable_array = variable_out;
    p.variable_array_a = variable_a_out;
    p.variable_array_cs = variable_cs_out;
    p.array_num = 1;
    m_pending_tus.emplace(pkt_id, std::move(p));
    return ORBIS_OK;
}

s32 NpHandler::TusGetFriendsDataStatus(s32 user_id, s32 service_label, s32 slot_id,
                                       bool include_self, s32 sort_type, u32 max,
                                       NpTus::OrbisNpTusDataStatus* status_out,
                                       NpTus::OrbisNpTusDataStatusA* status_a_out, u64 array_num,
                                       std::shared_ptr<NpTus::TusRequestCtx> ctx, u32 start_offset,
                                       u32* hits_out,
                                       NpTus::OrbisNpTusDataStatusForCrossSave* status_cs_out) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    shadnet::TusGetFriendsDataStatusRequest proto;
    proto.set_slotid(slot_id);
    proto.set_includeself(include_self);
    proto.set_sorttype(sort_type);
    proto.set_max(max);
    proto.set_startoffset(start_offset);
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::TusGetFriendsDataStatus,
                                             BuildTusPayload(com_id, proto.SerializeAsString()));
    std::lock_guard lock(m_mutex_pending_tus);
    PendingTusRequest p;
    p.req = std::move(ctx);
    p.cmd = ShadNet::CommandType::TusGetFriendsDataStatus;
    p.user_id = user_id;
    p.status_array = status_out;
    p.status_array_a = status_a_out;
    p.status_array_cs = status_cs_out;
    p.array_num = array_num;
    p.total_out = hits_out;
    m_pending_tus.emplace(pkt_id, std::move(p));
    return ORBIS_OK;
}

s32 NpHandler::TusGetFriendsVariable(s32 user_id, s32 service_label, s32 slot_id, bool include_self,
                                     s32 sort_type, u32 max,
                                     NpTus::OrbisNpTusVariable* variables_out,
                                     NpTus::OrbisNpTusVariableA* variables_a_out, u64 array_num,
                                     std::shared_ptr<NpTus::TusRequestCtx> ctx, u32 start_offset,
                                     u32* hits_out,
                                     NpTus::OrbisNpTusVariableForCrossSave* variables_cs_out) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    shadnet::TusGetFriendsVariableRequest proto;
    proto.set_slotid(slot_id);
    proto.set_includeself(include_self);
    proto.set_sorttype(sort_type);
    proto.set_max(max);
    proto.set_startoffset(start_offset);
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::TusGetFriendsVariable,
                                             BuildTusPayload(com_id, proto.SerializeAsString()));
    std::lock_guard lock(m_mutex_pending_tus);
    PendingTusRequest p;
    p.req = std::move(ctx);
    p.cmd = ShadNet::CommandType::TusGetFriendsVariable;
    p.user_id = user_id;
    p.variable_array = variables_out;
    p.variable_array_a = variables_a_out;
    p.variable_array_cs = variables_cs_out;
    p.array_num = array_num;
    p.total_out = hits_out;
    m_pending_tus.emplace(pkt_id, std::move(p));
    return ORBIS_OK;
}

s32 NpHandler::TusGetMultiSlotDataStatus(s32 user_id, s32 service_label,
                                         const std::string& owner_np_id,
                                         const std::string& virtual_user, s64 owner_account_id,
                                         const std::vector<s32>& slot_ids,
                                         NpTus::OrbisNpTusDataStatus* status_out,
                                         NpTus::OrbisNpTusDataStatusA* status_a_out, u64 array_num,
                                         std::shared_ptr<NpTus::TusRequestCtx> ctx,
                                         NpTus::OrbisNpTusDataStatusForCrossSave* status_cs_out) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    shadnet::TusGetMultiSlotDataStatusRequest proto;
    proto.set_ownernpid(owner_np_id);
    if (!virtual_user.empty()) {
        proto.set_virtualuser(virtual_user);
    } else if (owner_account_id != 0) {
        proto.set_owneraccountid(owner_account_id);
    } else if (owner_np_id.empty() ||
               Common::ToLower(owner_np_id) == Common::ToLower(client->GetNpid())) {
        proto.set_owneraccountid(static_cast<s64>(client->GetUserId()));
    }
    for (s32 s : slot_ids) {
        proto.add_slotids(s);
    }
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::TusGetMultiSlotDataStatus,
                                             BuildTusPayload(com_id, proto.SerializeAsString()));
    std::lock_guard lock(m_mutex_pending_tus);
    PendingTusRequest p;
    p.req = std::move(ctx);
    p.cmd = ShadNet::CommandType::TusGetMultiSlotDataStatus;
    p.user_id = user_id;
    p.status_array = status_out;
    p.status_array_a = status_a_out;
    p.status_array_cs = status_cs_out;
    p.array_num = array_num;
    m_pending_tus.emplace(pkt_id, std::move(p));
    return ORBIS_OK;
}

s32 NpHandler::TusSetData(s32 user_id, s32 service_label, const std::string& owner_np_id,
                          const std::string& virtual_user, s64 owner_account_id, s32 slot_id,
                          const std::vector<u8>& blob, const std::vector<u8>& info,
                          bool has_author_check, s64 is_last_changed_author,
                          const std::string& is_last_changed_author_np_id, bool has_date_check,
                          u64 is_last_changed_date, std::shared_ptr<NpTus::TusRequestCtx> ctx) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(m_mutex_clients);
        auto it = m_clients.find(user_id);
        if (it == m_clients.end()) {
            return ORBIS_NP_ERROR_SIGNED_OUT;
        }
        client = it->second;
    }
    shadnet::TusSetDataRequest proto;
    proto.set_ownernpid(owner_np_id);
    proto.set_slotid(slot_id);
    proto.set_data(blob.data(), blob.size());
    if (!info.empty()) {
        proto.set_info(info.data(), info.size());
    }
    if (!virtual_user.empty()) {
        proto.set_virtualuser(virtual_user);
    } else if (owner_account_id != 0) {
        proto.set_owneraccountid(owner_account_id);
    } else if (owner_np_id.empty() ||
               Common::ToLower(owner_np_id) == Common::ToLower(client->GetNpid())) {
        proto.set_owneraccountid(static_cast<s64>(client->GetUserId()));
    }
    if (has_author_check) {
        proto.set_hasauthorcheck(true);
        proto.set_islastchangedauthor(is_last_changed_author);
        if (!is_last_changed_author_np_id.empty()) {
            proto.set_islastchangedauthornpid(is_last_changed_author_np_id);
        }
    }
    if (has_date_check) {
        proto.set_hasdatecheck(true);
        proto.set_islastchangeddate(is_last_changed_date);
    }
    const std::string com_id = GetNpCommId(service_label);
    if (!IsValidNpCommId(com_id)) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const u64 pkt_id = client->SubmitRequest(ShadNet::CommandType::TusSetData,
                                             BuildTusPayload(com_id, proto.SerializeAsString()));
    std::lock_guard lock(m_mutex_pending_tus);
    PendingTusRequest p;
    p.req = std::move(ctx);
    p.cmd = ShadNet::CommandType::TusSetData;
    p.user_id = user_id;
    m_pending_tus.emplace(pkt_id, std::move(p));
    return ORBIS_OK;
}

void NpHandler::OnTusReply(s32 user_id, ShadNet::CommandType cmd, u64 pkt_id,
                           ShadNet::ErrorType error, const std::vector<u8>& body) {
    PendingTusRequest pending;
    {
        std::lock_guard lock(m_mutex_pending_tus);
        auto it = m_pending_tus.find(pkt_id);
        if (it == m_pending_tus.end()) {
            LOG_WARNING(NpHandler, "no pending request for pkt_id={} cmd={}", pkt_id,
                        static_cast<s32>(cmd));
            return;
        }
        pending = std::move(it->second);
        m_pending_tus.erase(it);
    }
    auto& req = pending.req;

    if (error != ShadNet::ErrorType::NoError) {
        s32 orbis_err = ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE;
        switch (error) {
        case ShadNet::ErrorType::Unauthorized:
            orbis_err = ORBIS_NP_COMMUNITY_SERVER_ERROR_FORBIDDEN;
            break;
        case ShadNet::ErrorType::NotFound:
            orbis_err = ORBIS_NP_COMMUNITY_SERVER_ERROR_USER_PROFILE_NOT_FOUND;
            break;
        case ShadNet::ErrorType::DbFail:
            orbis_err = ORBIS_NP_COMMUNITY_SERVER_ERROR_INTERNAL_SERVER_ERROR;
            break;
        case ShadNet::ErrorType::CondFail:
            orbis_err = ORBIS_NP_COMMUNITY_SERVER_ERROR_CONDITIONS_NOT_SATISFIED;
            break;
        default:
            break;
        }
        LOG_WARNING(NpHandler, "user_id={} pkt_id={} cmd={} server_error={} -> orbis {:#x}",
                    user_id, pkt_id, static_cast<s32>(cmd), static_cast<s32>(error),
                    static_cast<u32>(orbis_err));
        req->SetResult(orbis_err);
        return;
    }

    auto parse_tus_body = [&](auto& msg) -> bool {
        if (body.size() < 4) {
            LOG_ERROR(NpHandler, "invalid body size {}", body.size());
            return false;
        }
        const u32 proto_size = static_cast<u32>(body[0]) | (static_cast<u32>(body[1]) << 8) |
                               (static_cast<u32>(body[2]) << 16) |
                               (static_cast<u32>(body[3]) << 24);
        if (static_cast<u64>(4) + proto_size > body.size()) {
            LOG_ERROR(NpHandler, "invalid proto size {}", proto_size);
            return false;
        }
        if (!msg.ParseFromArray(body.data() + 4, static_cast<s32>(proto_size))) {
            LOG_ERROR(NpHandler, "failed to parse reply");
            return false;
        }
        return true;
    };

    auto fill_variable = [](NpTus::OrbisNpTusVariable& v, const shadnet::TusVariable& s) {
        v = NpTus::OrbisNpTusVariable{};
        CopyNpHandle(v.owner_id, s.ownernpid());
        v.has_data = s.set() ? 1 : 0;
        v.last_changed_date.tick = s.lastchangeddate();
        CopyNpHandle(v.last_changed_author_id, s.lastchangedauthornpid());
        v.variable = s.variable();
        v.old_variable = s.oldvariable();
        v.owner_account_id = static_cast<OrbisNpAccountId>(s.owneraccountid());
        v.last_changed_author_account_id =
            static_cast<OrbisNpAccountId>(s.lastchangedauthoraccountid());
    };
    auto fill_variable_a = [](NpTus::OrbisNpTusVariableA& v, const shadnet::TusVariable& s) {
        v = NpTus::OrbisNpTusVariableA{};
        std::memcpy(v.owner_id.data, s.ownernpid().data(),
                    std::min(s.ownernpid().size(), sizeof(v.owner_id.data)));
        v.has_data = s.set() ? 1 : 0;
        v.last_changed_date.tick = s.lastchangeddate();
        std::memcpy(
            v.last_changed_author_id.data, s.lastchangedauthornpid().data(),
            std::min(s.lastchangedauthornpid().size(), sizeof(v.last_changed_author_id.data)));
        v.variable = s.variable();
        v.old_variable = s.oldvariable();
        v.owner_account_id = static_cast<OrbisNpAccountId>(s.owneraccountid());
        v.last_changed_author_account_id =
            static_cast<OrbisNpAccountId>(s.lastchangedauthoraccountid());
    };
    auto fill_variable_for_cross_save = [](NpTus::OrbisNpTusVariableForCrossSave& v,
                                           const shadnet::TusVariable& s) {
        v = NpTus::OrbisNpTusVariableForCrossSave{};
        CopyNpHandle(v.owner_id, s.ownernpid());
        v.has_data = s.set() ? 1 : 0;
        v.last_changed_date.tick = s.lastchangeddate();
        CopyNpHandle(v.last_changed_author_id, s.lastchangedauthornpid());
        v.variable = s.variable();
        v.old_variable = s.oldvariable();
        v.owner_account_id = static_cast<OrbisNpAccountId>(s.owneraccountid());
        v.last_changed_author_account_id =
            static_cast<OrbisNpAccountId>(s.lastchangedauthoraccountid());
    };
    auto fill_status = [](NpTus::OrbisNpTusDataStatus& d, const shadnet::TusDataStatus& s) {
        d = NpTus::OrbisNpTusDataStatus{};
        CopyNpHandle(d.owner_id, s.ownernpid());
        d.has_data = s.set() ? 1 : 0;
        d.last_changed_date.tick = s.lastchangeddate();
        CopyNpHandle(d.last_changed_author_id, s.lastchangedauthornpid());
        d.data_size = s.datasize();
        d.info.size = s.info().size();
        const u64 cp = std::min(s.info().size(), sizeof(d.info.data));
        std::memcpy(d.info.data, s.info().data(), cp);
    };
    auto fill_status_a = [](NpTus::OrbisNpTusDataStatusA& d, const shadnet::TusDataStatus& s) {
        d = NpTus::OrbisNpTusDataStatusA{};
        std::memcpy(d.owner_id.data, s.ownernpid().data(),
                    std::min(s.ownernpid().size(), sizeof(d.owner_id.data)));
        d.has_data = s.set() ? 1 : 0;
        d.last_changed_date.tick = s.lastchangeddate();
        std::memcpy(
            d.last_changed_author_id.data, s.lastchangedauthornpid().data(),
            std::min(s.lastchangedauthornpid().size(), sizeof(d.last_changed_author_id.data)));
        d.data_size = s.datasize();
        d.info.size = s.info().size();
        const u64 cp = std::min(s.info().size(), sizeof(d.info.data));
        std::memcpy(d.info.data, s.info().data(), cp);
        d.owner_account_id = static_cast<OrbisNpAccountId>(s.owneraccountid());
        d.last_changed_author_account_id =
            static_cast<OrbisNpAccountId>(s.lastchangedauthoraccountid());
    };
    auto fill_status_for_cross_save = [](NpTus::OrbisNpTusDataStatusForCrossSave& d,
                                         const shadnet::TusDataStatus& s) {
        d = NpTus::OrbisNpTusDataStatusForCrossSave{};
        CopyNpHandle(d.owner_id, s.ownernpid());
        d.has_data = s.set() ? 1 : 0;
        d.last_changed_date.tick = s.lastchangeddate();
        CopyNpHandle(d.last_changed_author_id, s.lastchangedauthornpid());
        d.data_size = s.datasize();
        d.info.size = s.info().size();
        const u64 cp = std::min(s.info().size(), sizeof(d.info.data));
        std::memcpy(d.info.data, s.info().data(), cp);
        d.owner_account_id = static_cast<OrbisNpAccountId>(s.owneraccountid());
        d.last_changed_author_account_id =
            static_cast<OrbisNpAccountId>(s.lastchangedauthoraccountid());
    };

    switch (cmd) {
    case ShadNet::CommandType::TusGetMultiSlotVariable:
    case ShadNet::CommandType::TusTryAndSetVariable:
    case ShadNet::CommandType::TusGetMultiUserVariable:
    case ShadNet::CommandType::TusGetFriendsVariable:
    case ShadNet::CommandType::TusAddAndGetVariable: {
        shadnet::TusVariableResponse resp;
        if (!parse_tus_body(resp)) {
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            return;
        }
        u64 filled = 0;
        if (pending.variable_array_a) {
            filled = std::min<u64>(pending.array_num, resp.variables_size());
            for (u64 i = 0; i < filled; ++i) {
                fill_variable_a(pending.variable_array_a[i], resp.variables(static_cast<s32>(i)));
            }
        } else if (pending.variable_array_cs) {
            filled = std::min<u64>(pending.array_num, resp.variables_size());
            for (u64 i = 0; i < filled; ++i) {
                fill_variable_for_cross_save(pending.variable_array_cs[i],
                                             resp.variables(static_cast<s32>(i)));
            }
        } else if (pending.variable_array) {
            filled = std::min<u64>(pending.array_num, resp.variables_size());
            for (u64 i = 0; i < filled; ++i) {
                fill_variable(pending.variable_array[i], resp.variables(static_cast<s32>(i)));
            }
        }
        if (pending.total_out) {
            *pending.total_out =
                resp.total() ? resp.total() : static_cast<u32>(resp.variables_size());
        }
        if (cmd == ShadNet::CommandType::TusGetMultiSlotVariable ||
            cmd == ShadNet::CommandType::TusGetMultiUserVariable ||
            cmd == ShadNet::CommandType::TusGetFriendsVariable) {
            req->SetResult(static_cast<s32>(filled));
        }
        break;
    }
    case ShadNet::CommandType::TssGetData: {
        shadnet::TssGetDataResponse resp;
        if (!parse_tus_body(resp)) {
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            return;
        }
        u64 recv = 0;
        if (pending.data_out && !resp.data().empty()) {
            recv = std::min<u64>(pending.data_cap, resp.data().size());
            std::memcpy(pending.data_out, resp.data().data(), recv);
        }
        if (pending.tss_content_length_out) {
            *pending.tss_content_length_out = resp.contentlength();
        }
        if (pending.tss_status_out) {
            *pending.tss_status_out = NpTus::OrbisNpTssDataStatus{};
            pending.tss_status_out->modified.tick = resp.lastmodified();
            pending.tss_status_out->status =
                static_cast<NpTus::OrbisNpTssStatus>(resp.statuscodetype());
            pending.tss_status_out->content_length = resp.contentlength();
        }
        req->SetResult(ORBIS_OK);
        return;
    }
    case ShadNet::CommandType::TusGetData: {
        shadnet::TusGetDataResponse resp;
        if (!parse_tus_body(resp)) {
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            return;
        }
        u64 recv = 0;
        const u64 blob_size = resp.data().size();
        if (pending.data_out && pending.data_offset < blob_size) {
            const u64 avail = blob_size - pending.data_offset;
            recv = std::min<u64>(pending.data_cap, avail);
            std::memcpy(pending.data_out, resp.data().data() + pending.data_offset, recv);
        }
        if (pending.status_array_a) {
            NpTus::OrbisNpTusDataStatusA tmp{};
            fill_status_a(tmp, resp.status());
            if (recv) {
                tmp.data = pending.data_out;
            }
            const u64 cap =
                pending.status_cap ? std::min<u64>(pending.status_cap, sizeof(tmp)) : sizeof(tmp);
            std::memcpy(pending.status_array_a, &tmp, cap);
        } else if (pending.status_array_cs) {
            NpTus::OrbisNpTusDataStatusForCrossSave tmp{};
            fill_status_for_cross_save(tmp, resp.status());
            if (recv) {
                tmp.data = pending.data_out;
            }
            const u64 cap =
                pending.status_cap ? std::min<u64>(pending.status_cap, sizeof(tmp)) : sizeof(tmp);
            std::memcpy(pending.status_array_cs, &tmp, cap);
        } else if (pending.status_array) {
            NpTus::OrbisNpTusDataStatus tmp{};
            fill_status(tmp, resp.status());
            if (recv) {
                tmp.data = pending.data_out;
            }
            const u64 cap =
                pending.status_cap ? std::min<u64>(pending.status_cap, sizeof(tmp)) : sizeof(tmp);
            std::memcpy(pending.status_array, &tmp, cap);
        }
        req->SetResult(static_cast<s32>(recv));
        return;
    }
    case ShadNet::CommandType::TusGetMultiSlotDataStatus:
    case ShadNet::CommandType::TusGetMultiUserDataStatus:
    case ShadNet::CommandType::TusGetFriendsDataStatus: {
        shadnet::TusDataStatusResponse resp;
        if (!parse_tus_body(resp)) {
            req->SetResult(ORBIS_NP_COMMUNITY_ERROR_BAD_RESPONSE);
            return;
        }
        if (pending.status_array_a) {
            const u64 n = std::min<u64>(pending.array_num, resp.statuses_size());
            for (u64 i = 0; i < n; ++i) {
                fill_status_a(pending.status_array_a[i], resp.statuses(static_cast<s32>(i)));
            }
        } else if (pending.status_array_cs) {
            const u64 n = std::min<u64>(pending.array_num, resp.statuses_size());
            for (u64 i = 0; i < n; ++i) {
                fill_status_for_cross_save(pending.status_array_cs[i],
                                           resp.statuses(static_cast<s32>(i)));
            }
        } else if (pending.status_array) {
            const u64 n = std::min<u64>(pending.array_num, resp.statuses_size());
            for (u64 i = 0; i < n; ++i) {
                fill_status(pending.status_array[i], resp.statuses(static_cast<s32>(i)));
            }
        }
        if (pending.total_out) {
            *pending.total_out =
                resp.total() ? resp.total() : static_cast<u32>(resp.statuses_size());
        }
        if (cmd == ShadNet::CommandType::TusGetFriendsDataStatus ||
            cmd == ShadNet::CommandType::TusGetMultiSlotDataStatus ||
            cmd == ShadNet::CommandType::TusGetMultiUserDataStatus) {
            const s32 cnt =
                static_cast<s32>(std::min<u64>(pending.array_num, resp.statuses_size()));
            req->SetResult(cnt);
            return;
        }
        break;
    }
    default:
        LOG_DEBUG(NpHandler, "cmd={} ok (no payload)", static_cast<s32>(cmd));
        break;
    }
    req->SetResult(ORBIS_OK);
}

} // namespace Libraries::Np
