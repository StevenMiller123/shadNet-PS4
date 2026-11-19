// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>
#include <orbis/NpMatching2.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/network/fake_psn/fake_matching2.h"
#include "core/libraries/np/np_matching2/np_matching2.h"
#include "core/libraries/np/np_matching2/np_matching2_internal.h"

SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2CreateJoinRoom);
SHADNET_HOOK_DECLARE(Libraries::Np::NpMatching2, sceNpMatching2CreateJoinRoomA);

static void RegisterNpMatching2Hooks() {
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2CreateJoinRoom);
    SHADNET_HOOK(Libraries::Np::NpMatching2, sceNpMatching2CreateJoinRoomA);
}

namespace Libraries::Np::NpMatching2 {

s32 sceNpMatching2CreateJoinRoom(OrbisNpMatching2ContextId ctx_id,
                                 const OrbisNpMatching2CreateJoinRoomRequest* request,
                                 const OrbisNpMatching2RequestOptParam* opt_param,
                                 OrbisNpMatching2RequestId* req_id) {
    std::shared_ptr<CreateJoinRoomParams> params;
    if (request != nullptr) {
        // shadNet's protocol for handling CreateJoinRoom requests differs heavily from Sony's.
        // To bridge this, give the game's parameters to our FakeMatching2 class to use.
        params = std::make_shared<CreateJoinRoomParams>(*request);
        Network::FakeMatching2::Instance().OnCreateJoinRoom(params);
    }
    const s32 ret =
        SHADNET_HOOK_CONTINUE(sceNpMatching2CreateJoinRoom, ctx_id, request, opt_param, req_id);
    if (ret < 0 && params) {
        Network::FakeMatching2::Instance().CancelCreateJoinRoom(params.get());
    }
    return ret;
}

s32 sceNpMatching2CreateJoinRoomA(OrbisNpMatching2ContextId ctx_id,
                                  const OrbisNpMatching2CreateJoinRoomRequestA* request,
                                  const OrbisNpMatching2RequestOptParam* opt_param,
                                  OrbisNpMatching2RequestId* req_id) {
    std::shared_ptr<CreateJoinRoomParams> params;
    if (request != nullptr) {
        params = std::make_shared<CreateJoinRoomParams>(*request);
        Network::FakeMatching2::Instance().OnCreateJoinRoom(params);
    }
    const s32 ret =
        SHADNET_HOOK_CONTINUE(sceNpMatching2CreateJoinRoomA, ctx_id, request, opt_param, req_id);
    if (ret < 0 && params) {
        Network::FakeMatching2::Instance().CancelCreateJoinRoom(params.get());
    }
    return ret;
}

void RegisterHooks() {
    return RegisterNpMatching2Hooks();
}

} // namespace Libraries::Np::NpMatching2
