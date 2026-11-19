// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>

#include "core/libraries/np/np_matching2/np_matching2_internal.h"

namespace Libraries::Np::NpMatching2 {

CreateJoinRoomParams::CreateJoinRoomParams(const OrbisNpMatching2CreateJoinRoomRequest& request)
    : m_uses_account_ids{false} {
    Copy(request, m_request);
    if (request.allowed_user != nullptr) {
        m_online_ids.insert(m_online_ids.end(), request.allowed_user,
                            request.allowed_user + request.allowed_users);
    }
    if (request.blocked_user != nullptr) {
        m_online_ids.insert(m_online_ids.end(), request.blocked_user,
                            request.blocked_user + request.blocked_users);
    }
    // Allowed users come first, then blocked users.
    m_request.allowed_user = request.allowed_user != nullptr ? m_online_ids.data() : nullptr;
    m_request.blocked_user =
        request.blocked_user != nullptr ? m_online_ids.data() + request.allowed_users : nullptr;
}

CreateJoinRoomParams::CreateJoinRoomParams(const OrbisNpMatching2CreateJoinRoomRequestA& request)
    : m_uses_account_ids{true} {
    Copy(request, m_request_a);
    if (request.allowed_user != nullptr) {
        m_account_ids.insert(m_account_ids.end(), request.allowed_user,
                             request.allowed_user + request.allowed_users);
    }
    if (request.blocked_user != nullptr) {
        m_account_ids.insert(m_account_ids.end(), request.blocked_user,
                             request.blocked_user + request.blocked_users);
    }
    m_request_a.allowed_user = request.allowed_user != nullptr ? m_account_ids.data() : nullptr;
    m_request_a.blocked_user =
        request.blocked_user != nullptr ? m_account_ids.data() + request.allowed_users : nullptr;
}

template <typename Request>
void CreateJoinRoomParams::Copy(const Request& src, Request& dst) {
    dst = src;
    if (src.room_passwd != nullptr) {
        std::memcpy(&m_password, src.room_passwd, sizeof(m_password));
        dst.room_passwd = &m_password;
    }
    if (src.passwd_slot_mask != nullptr) {
        m_password_slot_mask = *src.passwd_slot_mask;
        dst.passwd_slot_mask = &m_password_slot_mask;
    }
    if (src.group_config != nullptr) {
        const auto* groups = static_cast<const OrbisNpMatching2RoomGroupConfig*>(src.group_config);
        m_group_configs.assign(groups, groups + src.group_configs);
        dst.group_config = m_group_configs.data();
    }
    if (src.join_group_label != nullptr) {
        std::memcpy(&m_join_group_label, src.join_group_label, sizeof(m_join_group_label));
        dst.join_group_label = &m_join_group_label;
    }
    if (src.external_search_int_attr != nullptr) {
        m_int_attrs.assign(src.external_search_int_attr,
                           src.external_search_int_attr + src.external_search_int_attrs);
        dst.external_search_int_attr = m_int_attrs.data();
    }
    dst.internal_bin_attr = CopyBinAttrs(src.internal_bin_attr, src.internal_bin_attrs);
    dst.external_search_bin_attr =
        CopyBinAttrs(src.external_search_bin_attr, src.external_search_bin_attrs);
    dst.external_bin_attr = CopyBinAttrs(src.external_bin_attr, src.external_bin_attrs);
    dst.member_internal_bin_attr =
        CopyBinAttrs(src.member_internal_bin_attr, src.member_internal_bin_attrs);
    if (src.signaling_param != nullptr) {
        m_signaling_param = *src.signaling_param;
        dst.signaling_param = &m_signaling_param;
    }
}

OrbisNpMatching2BinAttr* CreateJoinRoomParams::CopyBinAttrs(const OrbisNpMatching2BinAttr* attrs,
                                                            u64 count) {
    if (attrs == nullptr) {
        return nullptr;
    }
    auto& copies = m_bin_attrs.emplace_back(attrs, attrs + count);
    for (auto& attr : copies) {
        if (attr.data == nullptr) {
            continue;
        }
        auto& data = m_bin_attr_data.emplace_back(attr.data, attr.data + attr.data_size);
        attr.data = data.data();
    }
    return copies.data();
}

} // namespace Libraries::Np::NpMatching2
