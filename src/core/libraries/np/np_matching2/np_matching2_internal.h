// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <deque>
#include <vector>

#include "common/types.h"
#include "core/libraries/np/np_matching2/np_matching2.h"

namespace Libraries::Np::NpMatching2 {

class CreateJoinRoomParams {
public:
    explicit CreateJoinRoomParams(const OrbisNpMatching2CreateJoinRoomRequest& request);
    explicit CreateJoinRoomParams(const OrbisNpMatching2CreateJoinRoomRequestA& request);
    CreateJoinRoomParams(const CreateJoinRoomParams&) = delete;
    CreateJoinRoomParams& operator=(const CreateJoinRoomParams&) = delete;

    bool UsesAccountIds() const {
        return m_uses_account_ids;
    }
    const OrbisNpMatching2CreateJoinRoomRequest& GetRequest() const {
        return m_request;
    }
    const OrbisNpMatching2CreateJoinRoomRequestA& GetRequestA() const {
        return m_request_a;
    }

private:
    template <typename Request>
    void Copy(const Request& src, Request& dst);
    OrbisNpMatching2BinAttr* CopyBinAttrs(const OrbisNpMatching2BinAttr* attrs, u64 count);

    bool m_uses_account_ids;
    OrbisNpMatching2CreateJoinRoomRequest m_request{};
    OrbisNpMatching2CreateJoinRoomRequestA m_request_a{};
    OrbisNpMatching2SessionPassword m_password{};
    OrbisNpMatching2RoomPasswordSlotMask m_password_slot_mask{};
    OrbisNpMatching2GroupLabel m_join_group_label{};
    OrbisNpMatching2SignalingOptParam m_signaling_param{};
    std::vector<OrbisNpMatching2RoomGroupConfig> m_group_configs;
    std::vector<OrbisNpOnlineId> m_online_ids;
    std::vector<OrbisNpAccountId> m_account_ids;
    std::vector<OrbisNpMatching2IntAttr> m_int_attrs;
    std::deque<std::vector<OrbisNpMatching2BinAttr>> m_bin_attrs;
    std::deque<std::vector<u8>> m_bin_attr_data;
};

} // namespace Libraries::Np::NpMatching2
