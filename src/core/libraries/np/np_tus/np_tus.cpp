// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <orbis/NpTus.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_manager/np_manager.h"
#include "core/libraries/np/np_tus/np_tus.h"
#include "core/libraries/np/np_tus/np_tus_ctx.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/user_service/user_service.h"

extern "C" {
void sceNpTusAddAndGetVariableForCrossSave();
void sceNpTusAddAndGetVariableForCrossSaveAsync();
void sceNpTusAddAndGetVariableForCrossSaveVUser();
void sceNpTusAddAndGetVariableForCrossSaveVUserAsync();
void sceNpTusGetDataAVUser();
void sceNpTusGetDataForCrossSave();
void sceNpTusGetDataForCrossSaveAsync();
void sceNpTusGetDataForCrossSaveVUser();
void sceNpTusGetDataForCrossSaveVUserAsync();
void sceNpTusGetFriendsDataStatusForCrossSave();
void sceNpTusGetFriendsDataStatusForCrossSaveAsync();
void sceNpTusGetFriendsVariableForCrossSave();
void sceNpTusGetFriendsVariableForCrossSaveAsync();
void sceNpTusGetMultiSlotDataStatusAVUser();
void sceNpTusGetMultiSlotDataStatusAVUserAsync();
void sceNpTusGetMultiSlotDataStatusForCrossSave();
void sceNpTusGetMultiSlotDataStatusForCrossSaveAsync();
void sceNpTusGetMultiSlotDataStatusForCrossSaveVUser();
void sceNpTusGetMultiSlotDataStatusForCrossSaveVUserAsync();
void sceNpTusGetMultiSlotVariableAVUser();
void sceNpTusGetMultiSlotVariableAVUserAsync();
void sceNpTusGetMultiSlotVariableForCrossSave();
void sceNpTusGetMultiSlotVariableForCrossSaveAsync();
void sceNpTusGetMultiSlotVariableForCrossSaveVUser();
void sceNpTusGetMultiSlotVariableForCrossSaveVUserAsync();
void sceNpTusGetMultiUserDataStatusAVUser();
void sceNpTusGetMultiUserDataStatusAVUserAsync();
void sceNpTusGetMultiUserDataStatusForCrossSave();
void sceNpTusGetMultiUserDataStatusForCrossSaveAsync();
void sceNpTusGetMultiUserDataStatusForCrossSaveVUser();
void sceNpTusGetMultiUserDataStatusForCrossSaveVUserAsync();
void sceNpTusGetMultiUserVariableAVUser();
void sceNpTusGetMultiUserVariableAVUserAsync();
void sceNpTusGetMultiUserVariableForCrossSave();
void sceNpTusGetMultiUserVariableForCrossSaveAsync();
void sceNpTusGetMultiUserVariableForCrossSaveVUser();
void sceNpTusGetMultiUserVariableForCrossSaveVUserAsync();
void sceNpTusSetThreadParam();
void sceNpTusTryAndSetVariableForCrossSave();
void sceNpTusTryAndSetVariableForCrossSaveAsync();
void sceNpTusTryAndSetVariableForCrossSaveVUser();
void sceNpTusTryAndSetVariableForCrossSaveVUserAsync();
}

SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTssCreateNpTitleCtx);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTssCreateNpTitleCtxA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTssGetData);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTssGetDataAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTssGetSmallStorage);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTssGetSmallStorageAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTssGetStorage);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTssGetStorageAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusAbortRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusAddAndGetVariable);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusAddAndGetVariableA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusAddAndGetVariableAAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusAddAndGetVariableAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusAddAndGetVariableAVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusAddAndGetVariableAVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusAddAndGetVariableForCrossSave);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusAddAndGetVariableForCrossSaveAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusAddAndGetVariableForCrossSaveVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusAddAndGetVariableForCrossSaveVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusAddAndGetVariableVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusAddAndGetVariableVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusChangeModeForOtherSaveDataOwners);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusCreateNpTitleCtx);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusCreateNpTitleCtxA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusCreateRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusCreateTitleCtx);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotData);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotDataA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotDataAAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotDataAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotDataVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotDataVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotVariable);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotVariableA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotVariableAAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotVariableAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotVariableVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotVariableVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusDeleteNpTitleCtx);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusDeleteRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetData);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetDataA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetDataAAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetDataAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetDataAVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetDataAVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetDataForCrossSave);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetDataForCrossSaveAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetDataForCrossSaveVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetDataForCrossSaveVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetDataVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetDataVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetFriendsDataStatus);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetFriendsDataStatusA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetFriendsDataStatusAAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetFriendsDataStatusAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetFriendsDataStatusForCrossSave);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetFriendsDataStatusForCrossSaveAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetFriendsVariable);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetFriendsVariableA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetFriendsVariableAAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetFriendsVariableAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetFriendsVariableForCrossSave);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetFriendsVariableForCrossSaveAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatus);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusAAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusAVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusAVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusForCrossSave);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusForCrossSaveAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusForCrossSaveVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusForCrossSaveVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariable);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableAAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableAVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableAVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableForCrossSave);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableForCrossSaveAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableForCrossSaveVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableForCrossSaveVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatus);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusAAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusAVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusAVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusForCrossSave);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusForCrossSaveAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusForCrossSaveVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusForCrossSaveVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserVariable);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableAAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableAVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableAVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableForCrossSave);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableForCrossSaveAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableForCrossSaveVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableForCrossSaveVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusPollAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusSetData);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusSetDataA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusSetDataAAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusSetDataAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusSetDataAVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusSetDataAVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusSetDataVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusSetDataVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusSetMultiSlotVariable);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusSetMultiSlotVariableA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusSetMultiSlotVariableAAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusSetMultiSlotVariableAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusSetMultiSlotVariableVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusSetMultiSlotVariableVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusSetThreadParam);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusSetTimeout);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusTryAndSetVariable);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusTryAndSetVariableA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusTryAndSetVariableAAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusTryAndSetVariableAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusTryAndSetVariableAVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusTryAndSetVariableAVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusTryAndSetVariableForCrossSave);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusTryAndSetVariableForCrossSaveAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusTryAndSetVariableForCrossSaveVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusTryAndSetVariableForCrossSaveVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusTryAndSetVariableVUser);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusTryAndSetVariableVUserAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpTus, sceNpTusWaitAsync);

static void RegisterNpTusHooks() {
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTssCreateNpTitleCtx);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTssCreateNpTitleCtxA);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTssGetData);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTssGetDataAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTssGetSmallStorage);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTssGetSmallStorageAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTssGetStorage);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTssGetStorageAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusAbortRequest);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusAddAndGetVariable);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusAddAndGetVariableA);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusAddAndGetVariableAAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusAddAndGetVariableAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusAddAndGetVariableAVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusAddAndGetVariableAVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusAddAndGetVariableForCrossSave);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusAddAndGetVariableForCrossSaveAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusAddAndGetVariableForCrossSaveVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusAddAndGetVariableForCrossSaveVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusAddAndGetVariableVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusAddAndGetVariableVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusChangeModeForOtherSaveDataOwners);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusCreateNpTitleCtx);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusCreateNpTitleCtxA);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusCreateRequest);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusCreateTitleCtx);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotData);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotDataA);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotDataAAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotDataAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotDataVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotDataVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotVariable);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotVariableA);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotVariableAAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotVariableAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotVariableVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusDeleteMultiSlotVariableVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusDeleteNpTitleCtx);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusDeleteRequest);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetData);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetDataA);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetDataAAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetDataAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetDataAVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetDataAVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetDataForCrossSave);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetDataForCrossSaveAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetDataForCrossSaveVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetDataForCrossSaveVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetDataVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetDataVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetFriendsDataStatus);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetFriendsDataStatusA);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetFriendsDataStatusAAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetFriendsDataStatusAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetFriendsDataStatusForCrossSave);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetFriendsDataStatusForCrossSaveAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetFriendsVariable);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetFriendsVariableA);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetFriendsVariableAAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetFriendsVariableAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetFriendsVariableForCrossSave);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetFriendsVariableForCrossSaveAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatus);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusA);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusAAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusAVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusAVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusForCrossSave);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusForCrossSaveAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusForCrossSaveVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusForCrossSaveVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotDataStatusVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariable);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableA);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableAAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableAVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableAVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableForCrossSave);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableForCrossSaveAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableForCrossSaveVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableForCrossSaveVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiSlotVariableVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatus);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusA);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusAAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusAVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusAVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusForCrossSave);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusForCrossSaveAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusForCrossSaveVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusForCrossSaveVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserDataStatusVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserVariable);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableA);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableAAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableAVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableAVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableForCrossSave);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableForCrossSaveAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableForCrossSaveVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableForCrossSaveVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusGetMultiUserVariableVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusPollAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusSetData);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusSetDataA);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusSetDataAAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusSetDataAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusSetDataAVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusSetDataAVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusSetDataVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusSetDataVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusSetMultiSlotVariable);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusSetMultiSlotVariableA);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusSetMultiSlotVariableAAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusSetMultiSlotVariableAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusSetMultiSlotVariableVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusSetMultiSlotVariableVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusSetThreadParam);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusSetTimeout);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusTryAndSetVariable);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusTryAndSetVariableA);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusTryAndSetVariableAAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusTryAndSetVariableAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusTryAndSetVariableAVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusTryAndSetVariableAVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusTryAndSetVariableForCrossSave);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusTryAndSetVariableForCrossSaveAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusTryAndSetVariableForCrossSaveVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusTryAndSetVariableForCrossSaveVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusTryAndSetVariableVUser);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusTryAndSetVariableVUserAsync);
    SHADNET_HOOK(Libraries::Np::NpTus, sceNpTusWaitAsync);
}

namespace Libraries::Np::NpTus {

struct NpTusDataXfer {
    bool active = false;
    s32 slot_id = -1;
    s64 account_id = 0;
    std::string virtual_user;
    std::vector<u8> send_buf;
    u64 total_size = 0;
    std::vector<u8> info;
    u64 recv_offset = 0;

    bool SameTarget(s32 slot, s64 account, const std::string& vuser) const {
        return active && slot_id == slot && account_id == account && virtual_user == vuser;
    }
    void Begin(s32 slot, s64 account, const std::string& vuser) {
        *this = NpTusDataXfer{};
        active = true;
        slot_id = slot;
        account_id = account;
        virtual_user = vuser;
    }
};

struct NpTusRequest {
    s32 title_ctx_id = 0;
    std::shared_ptr<TusRequestCtx> ctx;
    NpTusDataXfer xfer;
};

struct NpTusTitleContext {
    u32 service_label = 0;
    OrbisNpId np_id{};
};

constexpr u64 TusMaxTitleCtx = 32;
constexpr u64 TusMaxRequests = 256;
constexpr s32 TusMaxSlotsPerRequest = 64;
constexpr s32 TusMaxSelectedFriends = 100;
constexpr u64 TssMaxSize = 64 * 1024U;
constexpr s32 TssMaxSlotId = 15;
constexpr s32 TusMaxUsersPerRequest = 101;

static std::mutex g_mutex;
static std::map<s32, NpTusTitleContext> g_title_ctxs;
static std::map<s32, NpTusRequest> g_requests;

static s32 FakeAsyncComplete(s32 req_id, s32 result = ORBIS_OK) {
    std::lock_guard lock(g_mutex);
    auto it = g_requests.find(req_id);
    if (it == g_requests.end()) {
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    ctx->SetResult(result);
    it->second.ctx = std::move(ctx);
    return ORBIS_OK;
}
static s32 g_next_ctx_id = 1;
static s32 g_next_req_id = 1;

s32 GetRequest(s32 request_id, NpTusRequest** out) {
    std::lock_guard lock(g_mutex);
    auto it = g_requests.find(request_id);
    if (it == g_requests.end()) {
        LOG_ERROR(Lib_NpTus, "invalid request id {}", request_id);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    *out = &it->second;
    return ORBIS_OK;
}
static s32 ResolveTus(s32 request_id, NpTusRequest** req_out, u32* svc_out, s32* uid_out,
                      std::string* self_np_id_out) {
    OrbisNpId self_np{};
    {
        std::lock_guard lock(g_mutex);
        auto rit = g_requests.find(request_id);
        if (rit == g_requests.end()) {
            LOG_ERROR(Lib_NpTus, "invalid request id {}", request_id);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        auto cit = g_title_ctxs.find(rit->second.title_ctx_id);
        if (cit == g_title_ctxs.end()) {
            LOG_ERROR(Lib_NpTus, "invalid title context {}", rit->second.title_ctx_id);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        *req_out = &rit->second;
        *svc_out = cit->second.service_label;
        self_np = cit->second.np_id;
    }
    *uid_out = NpHandler::Instance().GetUserIdByOnlineId(self_np.handle);
    if (self_np_id_out) {
        *self_np_id_out = std::string(self_np.handle.data);
    }
    return ORBIS_OK;
}

s32 sceNpTusCreateNpTitleCtx(OrbisNpServiceLabel service_label, OrbisNpId* np_id) {
    if (!np_id) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (service_label == ORBIS_NP_INVALID_SERVICE_LABEL) {
        LOG_ERROR(Lib_NpTus, "invalid service label {}", service_label);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpTus, "service_label = {}, np_id->data = {}", service_label, np_id->handle.data);

    std::lock_guard lock(g_mutex);
    if (g_title_ctxs.size() >= TusMaxTitleCtx) {
        LOG_ERROR(Lib_NpTus, "too many title contexts");
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
    }
    const s32 id = g_next_ctx_id++;
    NpTusTitleContext tc;
    tc.service_label = service_label;
    tc.np_id = *np_id;
    g_title_ctxs.emplace(id, tc);
    return id;
}

s32 sceNpTssCreateNpTitleCtx(OrbisNpServiceLabel service_label, OrbisNpId* np_id) {
    LOG_INFO(Lib_NpTus, "redirecting to sceNpTusCreateNpTitleCtx");
    return sceNpTusCreateNpTitleCtx(service_label, np_id);
}

s32 sceNpTusCreateNpTitleCtxA(OrbisNpServiceLabel service_label,
                              UserService::OrbisUserServiceUserId user_id) {
    LOG_INFO(Lib_NpTus, "service_label = {}, user_id = {}", service_label, user_id);
    OrbisNpId np_id;
    auto ret = NpManager::sceNpGetNpId(user_id, &np_id);

    if (ret < 0) {
        return ret;
    }

    return sceNpTusCreateNpTitleCtx(service_label, &np_id);
}

s32 sceNpTssCreateNpTitleCtxA(OrbisNpServiceLabel service_label,
                              UserService::OrbisUserServiceUserId user_id) {
    LOG_INFO(Lib_NpTus, "redirecting to sceNpTusCreateNpTitleCtxA");
    return sceNpTusCreateNpTitleCtxA(service_label, user_id);
}

s32 sceNpTusDeleteNpTitleCtx(s32 ctx_id) {
    LOG_INFO(Lib_NpTus, "ctx_id = {}", ctx_id);

    std::lock_guard lock(g_mutex);
    auto it = g_title_ctxs.find(ctx_id);
    if (it == g_title_ctxs.end()) {
        LOG_ERROR(Lib_NpTus, "invalid context id {}", ctx_id);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }

    for (auto rit = g_requests.begin(); rit != g_requests.end();) {
        if (rit->second.title_ctx_id == ctx_id) {
            if (rit->second.ctx) {
                LOG_ERROR(Lib_NpTus, "aborting request {}", rit->first);
                rit->second.ctx->SetResult(ORBIS_NP_COMMUNITY_ERROR_ABORTED);
            }
            rit = g_requests.erase(rit);
        } else {
            ++rit;
        }
    }
    g_title_ctxs.erase(it);
    return ORBIS_OK;
}

s32 sceNpTusCreateRequest(s32 lib_ctx_id) {
    LOG_INFO(Lib_NpTus, "lib_ctx_id = {}", lib_ctx_id);

    std::lock_guard lock(g_mutex);
    if (!g_title_ctxs.count(lib_ctx_id)) {
        LOG_ERROR(Lib_NpTus, "invalid context id {}", lib_ctx_id);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    if (g_requests.size() >= TusMaxRequests) {
        LOG_ERROR(Lib_NpTus, "too many requests");
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_OBJECTS;
    }
    const s32 id = g_next_req_id++;
    NpTusRequest r;
    r.title_ctx_id = lib_ctx_id;
    g_requests.emplace(id, std::move(r));
    return id;
}

s32 sceNpTusDeleteRequest(s32 request_id) {
    LOG_INFO(Lib_NpTus, "request_id = {:#x}", request_id);

    std::lock_guard lock(g_mutex);
    auto it = g_requests.find(request_id);
    if (it == g_requests.end()) {
        LOG_ERROR(Lib_NpTus, "invalid request id {}", request_id);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
    }
    if (it->second.ctx) {
        if (!it->second.ctx->HasResult()) {
            LOG_INFO(Lib_NpTus, "request {} still in flight, aborting it", request_id);
        }
        it->second.ctx->SetResult(ORBIS_NP_COMMUNITY_ERROR_ABORTED);
    }
    g_requests.erase(it);
    return ORBIS_OK;
}

s32 sceNpTusAbortRequest(s32 req_id) {
    LOG_INFO(Lib_NpTus, "req_id = {:#x}", req_id);

    std::shared_ptr<TusRequestCtx> ctx;
    {
        std::lock_guard lock(g_mutex);
        auto it = g_requests.find(req_id);
        if (it == g_requests.end()) {
            LOG_ERROR(Lib_NpTus, "invalid request id {}", req_id);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        ctx = it->second.ctx;
    }
    if (ctx) {
        ctx->SetResult(ORBIS_NP_COMMUNITY_ERROR_ABORTED);
    }
    return ORBIS_OK;
}

s32 sceNpTusPollAsync(s32 req_id, s32* result) {
    LOG_INFO(Lib_NpTus, "req_id = {:#x}", req_id);

    std::shared_ptr<TusRequestCtx> ctx;
    {
        std::lock_guard glock(g_mutex);
        auto it = g_requests.find(req_id);
        if (it == g_requests.end()) {
            LOG_ERROR(Lib_NpTus, "invalid request id {}", req_id);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        ctx = it->second.ctx;
    }

    if (!ctx) {
        LOG_ERROR(Lib_NpTus, "request not started");
        return 1;
    }

    std::lock_guard lock(ctx->mutex);
    if (!ctx->result.has_value()) {
        return 1;
    }
    const s32 res = *ctx->result;
    if (result) {
        *result = res;
    }
    if (res < 0) {
        LOG_ERROR(Lib_NpTus, "request {:#x} failed with {:#x}", req_id, static_cast<u32>(res));
    } else {
        LOG_DEBUG(Lib_NpTus, "req_id = {:#x} finished, result = {}", req_id, res);
    }
    return 0;
}

s32 sceNpTusWaitAsync(s32 req_id, s32* result) {
    LOG_INFO(Lib_NpTus, "req_id = {:#x}", req_id);

    std::shared_ptr<TusRequestCtx> ctx;
    {
        std::lock_guard glock(g_mutex);
        auto it = g_requests.find(req_id);
        if (it == g_requests.end()) {
            LOG_ERROR(Lib_NpTus, "invalid request id {}", req_id);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ID;
        }
        ctx = it->second.ctx;
    }

    if (!ctx) {
        LOG_ERROR(Lib_NpTus, "request not started");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    std::unique_lock lock(ctx->mutex);
    ctx->cv.wait(lock, [&] { return ctx->result.has_value(); });
    const s32 res = *ctx->result;
    if (result) {
        *result = res;
    }
    if (res < 0) {
        LOG_ERROR(Lib_NpTus, "request {:#x} failed with {:#x}", req_id, static_cast<u32>(res));
    } else {
        LOG_DEBUG(Lib_NpTus, "req_id = {:#x} finished, result = {}", req_id, res);
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiSlotVariableAsync(s32 req_id, OrbisNpId* np_id, s32* slot_ids,
                                      OrbisNpTusVariable* variable_array, u64 variables_size,
                                      s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, np_id = {}, slot_ids = {}, variable_array = {}, variables_size = {}, "
             "array_len = {}, option = {}",
             req_id, np_id ? np_id->handle.data : "", fmt::ptr(slot_ids), fmt::ptr(variable_array),
             variables_size, array_len, fmt::ptr(option));
    if (!slot_ids || !variable_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (array_len < 1 || option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid slot id");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (variables_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusVariable)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", variables_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const std::string owner =
        (np_id && np_id->handle.data[0]) ? std::string(np_id->handle.data) : self;
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    const s32 n = array_len;
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiSlotVariable(uid, static_cast<s32>(svc), owner,
                                                               std::string(), slots, variable_array,
                                                               static_cast<u64>(n), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiSlotVariable(s32 req_id, OrbisNpId* np_id, s32* slot_ids,
                                 OrbisNpTusVariable* variable_array, u64 variables_size,
                                 s32 array_len, void* option) {
    auto ret = sceNpTusGetMultiSlotVariableAsync(req_id, np_id, slot_ids, variable_array,
                                                 variables_size, array_len, option);
    if (ret < 0) {
        return ret;
    }

    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }

    return ret;
}

s32 sceNpTusSetMultiSlotVariableAsync(s32 req_id, OrbisNpId* np_id, s32* slot_ids, s64* variables,
                                      s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, np_id = {}, slot_ids = {}, variables = {}, array_len = {}, option = {}",
             req_id, np_id ? np_id->handle.data : "", fmt::ptr(slot_ids), fmt::ptr(variables),
             array_len, fmt::ptr(option));

    if (!slot_ids || !variables) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (array_len < 1 || option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid slot id");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const std::string owner =
        (np_id && np_id->handle.data[0]) ? std::string(np_id->handle.data) : self;
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    std::vector<s64> vals(variables, variables + array_len);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusSetMultiSlotVariable(uid, static_cast<s32>(svc), owner,
                                                               std::string(), slots, vals, ctx);
    if (submit < 0) {
        return submit;
    }

    return ORBIS_OK;
}

s32 sceNpTusSetMultiSlotVariable(s32 req_id, OrbisNpId* np_id, s32* slot_ids, s64* variables,
                                 s32 array_len, void* option) {
    auto ret =
        sceNpTusSetMultiSlotVariableAsync(req_id, np_id, slot_ids, variables, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiSlotVariableAVUserAsync(s32 req_id,
                                            const OrbisNpTusVirtualUserId* virtual_user_id,
                                            s32* slot_ids, OrbisNpTusVariableA* variable_array,
                                            u64 variables_size, s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, virtual_user_id = {}, slot_ids = {}, variable_array = {}, "
             "variables_size = {}, array_len = {}, option = {}",
             req_id, virtual_user_id ? virtual_user_id->data : "", fmt::ptr(slot_ids),
             fmt::ptr(variable_array), variables_size, array_len, fmt::ptr(option));
    if (!virtual_user_id || !slot_ids || !variable_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (array_len < 1 || option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid slot id");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (variables_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusVariableA)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", variables_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtual_user(virtual_user_id->data,
                                   strnlen(virtual_user_id->data, sizeof(virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid virtual user");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    const s32 n = array_len;
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiSlotVariable(
        uid, static_cast<s32>(svc), std::string(), virtual_user, slots, nullptr,
        static_cast<u64>(n), ctx, variable_array);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiSlotVariableAVUser(s32 req_id,
                                       const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                       s32* slot_ids, OrbisNpTusVariableA* variables,
                                       u64 variables_size, s32 array_len, void* option) {
    auto ret = sceNpTusGetMultiSlotVariableAVUserAsync(
        req_id, target_virtual_user_id, slot_ids, variables, variables_size, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusSetMultiSlotVariableVUserAsync(s32 req_id,
                                           const OrbisNpTusVirtualUserId* virtual_user_id,
                                           s32* slot_ids, const s64* variables, s32 array_len,
                                           void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, virtual_user_id = {}, slot_ids = {}, variables = {}, array_len = {}, "
             "option = {}",
             req_id, virtual_user_id ? virtual_user_id->data : "", fmt::ptr(slot_ids),
             fmt::ptr(variables), array_len, fmt::ptr(option));
    if (!virtual_user_id || !slot_ids || !variables) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (array_len < 1 || option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid slot id");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const std::string virtual_user(virtual_user_id->data,
                                   strnlen(virtual_user_id->data, sizeof(virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid virtual user");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    std::vector<s64> values(variables, variables + array_len);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusSetMultiSlotVariable(
        uid, static_cast<s32>(svc), std::string(), virtual_user, slots, values, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusSetMultiSlotVariableVUser(s32 req_id,
                                      const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                      s32* slot_ids, s64* variables, s32 array_len, void* option) {
    auto ret = sceNpTusSetMultiSlotVariableVUserAsync(req_id, target_virtual_user_id, slot_ids,
                                                      variables, array_len, option);
    if (ret < 0) {
        return ret;
    }

    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }

    return ret;
}

s32 sceNpTusSetMultiSlotVariableAAsync(s32 req_id, OrbisNpAccountId target_account_id,
                                       s32* slot_ids, const s64* variables, s32 array_len,
                                       void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_account_id = {}, slot_ids = {}, variables = {}, array_len = {}, "
             "option = {}",
             req_id, target_account_id, fmt::ptr(slot_ids), fmt::ptr(variables), array_len,
             fmt::ptr(option));
    if (!slot_ids || !variables) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (array_len < 1 || option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid slot id");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    std::vector<s64> vals(variables, variables + array_len);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusSetMultiSlotVariable(
        uid, static_cast<s32>(svc), std::string(), std::string(), slots, vals, ctx,
        static_cast<s64>(target_account_id));
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiSlotVariableAAsync(s32 req_id, OrbisNpAccountId target_account_id,
                                       s32* slot_ids, OrbisNpTusVariableA* variable_array,
                                       u64 variables_size, s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_account_id = {}, slot_ids = {}, variable_array = {}, "
             "variables_size = {}, array_len = {}, option = {}",
             req_id, target_account_id, fmt::ptr(slot_ids), fmt::ptr(variable_array),
             variables_size, array_len, fmt::ptr(option));
    if (!slot_ids || !variable_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (array_len < 1 || option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid slot id");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (variables_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusVariableA)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", variables_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    const s32 n = array_len;
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiSlotVariable(
        uid, static_cast<s32>(svc), std::string(), std::string(), slots, nullptr,
        static_cast<u64>(n), ctx, variable_array, static_cast<s64>(target_account_id));
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetDataAAsync(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id,
                          OrbisNpTusDataStatusA* data_status, u64 data_status_size, void* data,
                          u64 recv_size, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_account_id = {}, slot_id = {}, data_status = {}, "
             "data_status_size = {}, data = {}, recv_size = {}, option = {}",
             req_id, target_account_id, slot_id, fmt::ptr(data_status), data_status_size,
             fmt::ptr(data), recv_size, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!data_status || data_status_size == 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid slot id");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    const s64 account = static_cast<s64>(target_account_id);
    u64 offset = 0;
    {
        std::lock_guard lock(g_mutex);
        if (!req->xfer.SameTarget(slot_id, account, std::string())) {
            req->xfer.Begin(slot_id, account, std::string());
        }
        offset = req->xfer.recv_offset;
        if (data) {
            req->xfer.recv_offset += recv_size;
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetData(
        uid, static_cast<s32>(svc), std::string(), std::string(), account, slot_id, data_status,
        data_status_size, data, data ? recv_size : 0, offset, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusSetDataAAsync(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id,
                          u64 total_size, u64 send_size, const void* data,
                          const OrbisNpTusDataInfo* info, u64 info_struct_size,
                          const OrbisNpAccountId* is_last_changed_author,
                          const Rtc::OrbisRtcTick* is_last_changed_date, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_account_id = {}, slot_id = {}, total_size = {}, send_size = {}, "
             "data = {}, info = {}, info_struct_size = {}, option = {}",
             req_id, target_account_id, slot_id, total_size, send_size, fmt::ptr(data),
             fmt::ptr(info), info_struct_size, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid option");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!data || send_size == 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slot_id < 0 || send_size > total_size) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    const s64 account = static_cast<s64>(target_account_id);
    std::vector<u8> blob;
    std::vector<u8> info_bytes;
    bool complete = false;
    {
        std::lock_guard lock(g_mutex);
        auto& x = req->xfer;
        if (!x.SameTarget(slot_id, account, std::string())) {
            x.Begin(slot_id, account, std::string());
            x.total_size = total_size;
            x.send_buf.reserve(total_size);
            if (info && info_struct_size >= sizeof(OrbisNpTusDataInfo)) {
                const u64 n = std::min<u64>(info->size, sizeof(info->data));
                info_bytes.assign(info->data, info->data + n);
                x.info = info_bytes;
            }
        }
        const u8* src = static_cast<const u8*>(data);
        x.send_buf.insert(x.send_buf.end(), src, src + send_size);
        if (x.send_buf.size() >= x.total_size) {
            blob = x.send_buf;
            info_bytes = x.info;
            complete = true;
            x = NpTusDataXfer{};
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    if (!complete) {
        ctx->SetResult(ORBIS_OK);
        return ORBIS_OK;
    }

    const bool has_author_check = is_last_changed_author != nullptr;
    const bool has_date_check = is_last_changed_date != nullptr;
    s32 submit = NpHandler::Instance().TusSetData(
        uid, static_cast<s32>(svc), std::string(), std::string(), account, slot_id, blob,
        info_bytes, has_author_check,
        has_author_check ? static_cast<s64>(*is_last_changed_author) : 0, std::string(),
        has_date_check, has_date_check ? is_last_changed_date->tick : 0, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetDataAVUserAsync(s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id,
                               s32 slot_id, OrbisNpTusDataStatusA* data_status,
                               u64 data_status_size, void* data, u64 recv_size, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_virtual_user_id = {}, slot_id = {}, data_status = {}, "
             "data_status_size = {}, data = {}, recv_size = {}, option = {}",
             req_id, target_virtual_user_id ? target_virtual_user_id->data : "", slot_id,
             fmt::ptr(data_status), data_status_size, fmt::ptr(data), recv_size, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid option");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!target_virtual_user_id || !data_status || data_status_size == 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid slot id");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const std::string virtual_user(
        target_virtual_user_id->data,
        strnlen(target_virtual_user_id->data, sizeof(target_virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid virtual user");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    u64 offset = 0;
    {
        std::lock_guard lock(g_mutex);
        if (!req->xfer.SameTarget(slot_id, 0, virtual_user)) {
            req->xfer.Begin(slot_id, 0, virtual_user);
        }
        offset = req->xfer.recv_offset;
        if (data) {
            req->xfer.recv_offset += recv_size;
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetData(
        uid, static_cast<s32>(svc), std::string(), virtual_user, 0, slot_id, data_status,
        data_status_size, data, data ? recv_size : 0, offset, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusSetDataAVUserAsync(s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id,
                               s32 slot_id, u64 total_size, u64 send_size, const void* data,
                               const OrbisNpTusDataInfo* info, u64 info_struct_size,
                               const OrbisNpAccountId* is_last_changed_author,
                               const Rtc::OrbisRtcTick* is_last_changed_date, void* option) {
    LOG_INFO(
        Lib_NpTus,
        "req_id = {}, target_virtual_user_id = {}, slot_id = {}, total_size = {}, send_size = {}, "
        "data = {}, info = {}, info_struct_size = {}, option = {}",
        req_id, target_virtual_user_id ? target_virtual_user_id->data : "", slot_id, total_size,
        send_size, fmt::ptr(data), fmt::ptr(info), info_struct_size, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid option");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!target_virtual_user_id || !data || send_size == 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slot_id < 0 || send_size > total_size) {
        LOG_ERROR(Lib_NpTus, "invalid slot id");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const std::string virtual_user(
        target_virtual_user_id->data,
        strnlen(target_virtual_user_id->data, sizeof(target_virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid virtual user");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    std::vector<u8> blob;
    std::vector<u8> info_bytes;
    bool complete = false;
    {
        std::lock_guard lock(g_mutex);
        auto& x = req->xfer;
        if (!x.SameTarget(slot_id, 0, virtual_user)) {
            x.Begin(slot_id, 0, virtual_user);
            x.total_size = total_size;
            x.send_buf.reserve(total_size);
            if (info && info_struct_size >= sizeof(OrbisNpTusDataInfo)) {
                const u64 n = std::min<u64>(info->size, sizeof(info->data));
                x.info.assign(info->data, info->data + n);
            }
        }
        const u8* src = static_cast<const u8*>(data);
        x.send_buf.insert(x.send_buf.end(), src, src + send_size);
        if (x.send_buf.size() >= x.total_size) {
            blob = x.send_buf;
            info_bytes = x.info;
            complete = true;
            x = NpTusDataXfer{};
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    if (!complete) {
        ctx->SetResult(ORBIS_OK);
        return ORBIS_OK;
    }

    const bool has_author_check = is_last_changed_author != nullptr;
    const bool has_date_check = is_last_changed_date != nullptr;
    s32 submit = NpHandler::Instance().TusSetData(
        uid, static_cast<s32>(svc), std::string(), virtual_user, 0, slot_id, blob, info_bytes,
        has_author_check, has_author_check ? static_cast<s64>(*is_last_changed_author) : 0,
        std::string(), has_date_check, has_date_check ? is_last_changed_date->tick : 0, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusAddAndGetVariable(s32 req_id, const OrbisNpId* target_np_id, s32 slot_id,
                              s64 in_variable, const OrbisNpId* is_last_changed_author,
                              const Rtc::OrbisRtcTick* is_last_changed_date,
                              OrbisNpTusVariable* out_variable, u64 out_variable_size,
                              void* option) {
    auto ret = sceNpTusAddAndGetVariableAsync(req_id, target_np_id, slot_id, in_variable,
                                              is_last_changed_author, is_last_changed_date,
                                              out_variable, out_variable_size, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusAddAndGetVariableAsync(s32 req_id, const OrbisNpId* target_np_id, s32 slot_id,
                                   s64 in_variable, const OrbisNpId* is_last_changed_author,
                                   const Rtc::OrbisRtcTick* is_last_changed_date,
                                   OrbisNpTusVariable* out_variable, u64 out_variable_size,
                                   void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_np_id = {}, slot_id = {}, in_variable = {}, out_variable = {}, "
             "out_variable_size = {}, option = {}",
             req_id, target_np_id ? target_np_id->handle.data : "", slot_id, in_variable,
             fmt::ptr(out_variable), out_variable_size, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!out_variable) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid slot id");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (out_variable_size != sizeof(OrbisNpTusVariable)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", out_variable_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const std::string owner = (target_np_id && target_np_id->handle.data[0])
                                  ? std::string(target_np_id->handle.data)
                                  : self;

    const bool has_author_check = is_last_changed_author != nullptr;
    const bool has_date_check = is_last_changed_date != nullptr;
    const std::string author_np_id =
        has_author_check ? std::string(is_last_changed_author->handle.data) : std::string();

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusAddAndGetVariable(
        uid, static_cast<s32>(svc), owner, std::string(), 0, slot_id, in_variable, has_author_check,
        0, author_np_id, has_date_check, has_date_check ? is_last_changed_date->tick : 0,
        out_variable, nullptr, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusAddAndGetVariableVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s64 in_variable,
    const OrbisNpId* is_last_changed_author, const Rtc::OrbisRtcTick* is_last_changed_date,
    OrbisNpTusVariable* out_variable, u64 out_variable_size, void* option) {
    auto ret = sceNpTusAddAndGetVariableVUserAsync(
        req_id, target_virtual_user_id, slot_id, in_variable, is_last_changed_author,
        is_last_changed_date, out_variable, out_variable_size, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusAddAndGetVariableVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s64 in_variable,
    const OrbisNpId* is_last_changed_author, const Rtc::OrbisRtcTick* is_last_changed_date,
    OrbisNpTusVariable* out_variable, u64 out_variable_size, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_virtual_user_id = {}, slot_id = {}, in_variable = {}, "
             "out_variable = {}, out_variable_size = {}, option = {}",
             req_id, target_virtual_user_id ? target_virtual_user_id->data : "", slot_id,
             in_variable, fmt::ptr(out_variable), out_variable_size, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!target_virtual_user_id || !out_variable) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid slot id");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (out_variable_size != sizeof(OrbisNpTusVariable)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", out_variable_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtual_user(
        target_virtual_user_id->data,
        strnlen(target_virtual_user_id->data, sizeof(target_virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid virtual user");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    const bool has_author_check = is_last_changed_author != nullptr;
    const bool has_date_check = is_last_changed_date != nullptr;
    const std::string author_np_id =
        has_author_check ? std::string(is_last_changed_author->handle.data) : std::string();

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusAddAndGetVariable(
        uid, static_cast<s32>(svc), std::string(), virtual_user, 0, slot_id, in_variable,
        has_author_check, 0, author_np_id, has_date_check,
        has_date_check ? is_last_changed_date->tick : 0, out_variable, nullptr, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetDataAsync(s32 req_id, const OrbisNpId* target_np_id, s32 slot_id,
                         OrbisNpTusDataStatus* data_status, u64 data_status_size, void* data,
                         u64 recv_size, void* option) {
    LOG_INFO(
        Lib_NpTus,
        "req_id = {}, target_np_id = {}, slot_id = {}, data_status = {}, data_status_size = {}, "
        "data = {}, recv_size = {}, option = {}",
        req_id, target_np_id ? target_np_id->handle.data : "", slot_id, fmt::ptr(data_status),
        data_status_size, fmt::ptr(data), recv_size, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!data_status || data_status_size == 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid slot id");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const std::string owner = (target_np_id && target_np_id->handle.data[0])
                                  ? std::string(target_np_id->handle.data)
                                  : self;

    u64 offset = 0;
    {
        std::lock_guard lock(g_mutex);
        if (!req->xfer.SameTarget(slot_id, 0, owner)) {
            req->xfer.Begin(slot_id, 0, owner);
        }
        offset = req->xfer.recv_offset;
        if (data) {
            req->xfer.recv_offset += recv_size;
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetData(uid, static_cast<s32>(svc), owner, std::string(),
                                                  0, slot_id, nullptr, data_status_size, data,
                                                  data ? recv_size : 0, offset, ctx, data_status);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetDataVUserAsync(s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id,
                              s32 slot_id, OrbisNpTusDataStatus* data_status, u64 data_status_size,
                              void* data, u64 recv_size, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_virtual_user_id = {}, slot_id = {}, data_status = {}, "
             "data_status_size = {}, data = {}, recv_size = {}, option = {}",
             req_id, target_virtual_user_id ? target_virtual_user_id->data : "", slot_id,
             fmt::ptr(data_status), data_status_size, fmt::ptr(data), recv_size, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!target_virtual_user_id || !data_status || data_status_size == 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid slot id");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const std::string virtual_user(
        target_virtual_user_id->data,
        strnlen(target_virtual_user_id->data, sizeof(target_virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid virtual user");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    u64 offset = 0;
    {
        std::lock_guard lock(g_mutex);
        if (!req->xfer.SameTarget(slot_id, 0, virtual_user)) {
            req->xfer.Begin(slot_id, 0, virtual_user);
        }
        offset = req->xfer.recv_offset;
        if (data) {
            req->xfer.recv_offset += recv_size;
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetData(
        uid, static_cast<s32>(svc), std::string(), virtual_user, 0, slot_id, nullptr,
        data_status_size, data, data ? recv_size : 0, offset, ctx, data_status);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetFriendsDataStatus(s32 req_id, s32 slot_id, s32 include_self, s32 sort_type,
                                 OrbisNpTusDataStatus* status_array, u64 status_array_size,
                                 s32 array_len, void* option) {
    auto ret =
        sceNpTusGetFriendsDataStatusAsync(req_id, slot_id, include_self, sort_type, status_array,
                                          status_array_size, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetFriendsDataStatusAsync(s32 req_id, s32 slot_id, s32 include_self, s32 sort_type,
                                      OrbisNpTusDataStatus* status_array, u64 status_array_size,
                                      s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, slot_id = {}, include_self = {}, sort_type = {}, status_array = {}, "
             "status_array_size = {}, array_len = {}, option = {}",
             req_id, slot_id, include_self, sort_type, fmt::ptr(status_array), status_array_size,
             array_len, fmt::ptr(option));
    if (!status_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || array_len < 1 || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSelectedFriends) {
        LOG_ERROR(Lib_NpTus, "too many friends");
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (sort_type < ORBIS_NP_TUS_DATASTATUS_SORTTYPE_DESCENDING_DATE ||
        sort_type > ORBIS_NP_TUS_DATASTATUS_SORTTYPE_ASCENDING_DATE) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (status_array_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusDataStatus)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", status_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetFriendsDataStatus(
        uid, static_cast<s32>(svc), slot_id, include_self != 0, sort_type,
        static_cast<u32>(array_len), status_array, nullptr, static_cast<u64>(array_len), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetFriendsVariable(s32 req_id, s32 slot_id, s32 include_self, s32 sort_type,
                               OrbisNpTusVariable* variable_array, u64 variable_array_size,
                               s32 array_len, void* option) {
    auto ret =
        sceNpTusGetFriendsVariableAsync(req_id, slot_id, include_self, sort_type, variable_array,
                                        variable_array_size, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetFriendsVariableAsync(s32 req_id, s32 slot_id, s32 include_self, s32 sort_type,
                                    OrbisNpTusVariable* variable_array, u64 variable_array_size,
                                    s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, slot_id = {}, include_self = {}, sort_type = {}, variable_array = {}, "
             "variable_array_size = {}, array_len = {}, option = {}",
             req_id, slot_id, include_self, sort_type, fmt::ptr(variable_array),
             variable_array_size, array_len, fmt::ptr(option));
    if (!variable_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || array_len < 1 || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSelectedFriends) {
        LOG_ERROR(Lib_NpTus, "too many friends");
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (sort_type < ORBIS_NP_TUS_VARIABLE_SORTTYPE_DESCENDING_DATE ||
        sort_type > ORBIS_NP_TUS_VARIABLE_SORTTYPE_ASCENDING_VALUE) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (variable_array_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusVariable)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", variable_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetFriendsVariable(
        uid, static_cast<s32>(svc), slot_id, include_self != 0, sort_type,
        static_cast<u32>(array_len), variable_array, nullptr, static_cast<u64>(array_len), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiSlotDataStatus(s32 req_id, const OrbisNpId* target_np_id, s32* slot_ids,
                                   OrbisNpTusDataStatus* status_array, u64 status_array_size,
                                   s32 array_len, void* option) {
    auto ret = sceNpTusGetMultiSlotDataStatusAsync(req_id, target_np_id, slot_ids, status_array,
                                                   status_array_size, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiSlotDataStatusAsync(s32 req_id, const OrbisNpId* target_np_id, s32* slot_ids,
                                        OrbisNpTusDataStatus* status_array, u64 status_array_size,
                                        s32 array_len, void* option) {
    LOG_INFO(
        Lib_NpTus,
        "req_id = {}, target_np_id = {}, slot_ids = {}, status_array = {}, status_array_size = {}, "
        "array_len = {}, option = {}",
        req_id, target_np_id ? target_np_id->handle.data : "", fmt::ptr(slot_ids),
        fmt::ptr(status_array), status_array_size, array_len, fmt::ptr(option));
    if (!target_np_id || !slot_ids || !status_array || array_len < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots");
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (status_array_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusDataStatus)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", status_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const std::string owner =
        target_np_id->handle.data[0] ? std::string(target_np_id->handle.data) : self;
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiSlotDataStatus(
        uid, static_cast<s32>(svc), owner, std::string(), 0, slots, status_array, nullptr,
        static_cast<u64>(array_len), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiSlotDataStatusVUser(s32 req_id,
                                        const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                        s32* slot_ids, OrbisNpTusDataStatus* status_array,
                                        u64 status_array_size, s32 array_len, void* option) {
    auto ret = sceNpTusGetMultiSlotDataStatusVUserAsync(req_id, target_virtual_user_id, slot_ids,
                                                        status_array, status_array_size, array_len,
                                                        option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiSlotDataStatusVUserAsync(s32 req_id,
                                             const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                             s32* slot_ids, OrbisNpTusDataStatus* status_array,
                                             u64 status_array_size, s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_virtual_user_id = {}, slot_ids = {}, status_array = {}, "
             "status_array_size = {}, array_len = {}, option = {}",
             req_id, target_virtual_user_id ? target_virtual_user_id->data : "", fmt::ptr(slot_ids),
             fmt::ptr(status_array), status_array_size, array_len, fmt::ptr(option));
    if (!target_virtual_user_id || !slot_ids || !status_array || array_len < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots");
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (status_array_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusDataStatus)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", status_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtual_user(
        target_virtual_user_id->data,
        strnlen(target_virtual_user_id->data, sizeof(target_virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid virtual user");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiSlotDataStatus(
        uid, static_cast<s32>(svc), std::string(), virtual_user, 0, slots, status_array, nullptr,
        static_cast<u64>(array_len), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusSetDataAsync(s32 req_id, const OrbisNpId* target_np_id, s32 slot_id, u64 total_size,
                         u64 send_size, const void* data, const OrbisNpTusDataInfo* info,
                         u64 info_struct_size, const OrbisNpId* is_last_changed_author,
                         const Rtc::OrbisRtcTick* is_last_changed_date, void* option) {
    LOG_INFO(
        Lib_NpTus,
        "req_id = {}, target_np_id = {}, slot_id = {}, total_size = {}, send_size = {}, data = {}, "
        "info = {}, info_struct_size = {}, option = {}",
        req_id, target_np_id ? target_np_id->handle.data : "", slot_id, total_size, send_size,
        fmt::ptr(data), fmt::ptr(info), info_struct_size, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!data || send_size == 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slot_id < 0 || send_size > total_size) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const std::string owner = (target_np_id && target_np_id->handle.data[0])
                                  ? std::string(target_np_id->handle.data)
                                  : self;

    std::vector<u8> blob;
    std::vector<u8> info_bytes;
    bool complete = false;
    {
        std::lock_guard lock(g_mutex);
        auto& x = req->xfer;
        if (!x.SameTarget(slot_id, 0, owner)) {
            x.Begin(slot_id, 0, owner);
            x.total_size = total_size;
            x.send_buf.reserve(total_size);
            if (info && info_struct_size >= sizeof(OrbisNpTusDataInfo)) {
                const u64 n = std::min<u64>(info->size, sizeof(info->data));
                x.info.assign(info->data, info->data + n);
            }
        }
        const u8* src = static_cast<const u8*>(data);
        x.send_buf.insert(x.send_buf.end(), src, src + send_size);
        if (x.send_buf.size() >= x.total_size) {
            blob = x.send_buf;
            info_bytes = x.info;
            complete = true;
            x = NpTusDataXfer{};
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    if (!complete) {
        ctx->SetResult(ORBIS_OK);
        return ORBIS_OK;
    }
    const bool has_author_check = is_last_changed_author != nullptr;
    const bool has_date_check = is_last_changed_date != nullptr;
    const std::string author_np_id =
        has_author_check ? std::string(is_last_changed_author->handle.data) : std::string();
    s32 submit = NpHandler::Instance().TusSetData(
        uid, static_cast<s32>(svc), owner, std::string(), 0, slot_id, blob, info_bytes,
        has_author_check, 0, author_np_id, has_date_check,
        has_date_check ? is_last_changed_date->tick : 0, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusSetDataVUserAsync(s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id,
                              s32 slot_id, u64 total_size, u64 send_size, const void* data,
                              const OrbisNpTusDataInfo* info, u64 info_struct_size,
                              const OrbisNpId* is_last_changed_author,
                              const Rtc::OrbisRtcTick* is_last_changed_date, void* option) {
    LOG_INFO(
        Lib_NpTus,
        "req_id = {}, target_virtual_user_id = {}, slot_id = {}, total_size = {}, send_size = {}, "
        "data = {}, info = {}, info_struct_size = {}, option = {}",
        req_id, target_virtual_user_id ? target_virtual_user_id->data : "", slot_id, total_size,
        send_size, fmt::ptr(data), fmt::ptr(info), info_struct_size, fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!target_virtual_user_id || !data || send_size == 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (slot_id < 0 || send_size > total_size) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const std::string virtual_user(
        target_virtual_user_id->data,
        strnlen(target_virtual_user_id->data, sizeof(target_virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid virtual user");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    std::vector<u8> blob;
    std::vector<u8> info_bytes;
    bool complete = false;
    {
        std::lock_guard lock(g_mutex);
        auto& x = req->xfer;
        if (!x.SameTarget(slot_id, 0, virtual_user)) {
            x.Begin(slot_id, 0, virtual_user);
            x.total_size = total_size;
            x.send_buf.reserve(total_size);
            if (info && info_struct_size >= sizeof(OrbisNpTusDataInfo)) {
                const u64 n = std::min<u64>(info->size, sizeof(info->data));
                x.info.assign(info->data, info->data + n);
            }
        }
        const u8* src = static_cast<const u8*>(data);
        x.send_buf.insert(x.send_buf.end(), src, src + send_size);
        if (x.send_buf.size() >= x.total_size) {
            blob = x.send_buf;
            info_bytes = x.info;
            complete = true;
            x = NpTusDataXfer{};
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    if (!complete) {
        ctx->SetResult(ORBIS_OK);
        return ORBIS_OK;
    }

    const bool has_author_check = is_last_changed_author != nullptr;
    const bool has_date_check = is_last_changed_date != nullptr;
    const std::string author_np_id =
        has_author_check ? std::string(is_last_changed_author->handle.data) : std::string();
    s32 submit = NpHandler::Instance().TusSetData(
        uid, static_cast<s32>(svc), std::string(), virtual_user, 0, slot_id, blob, info_bytes,
        has_author_check, 0, author_np_id, has_date_check,
        has_date_check ? is_last_changed_date->tick : 0, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusTryAndSetVariable(s32 req_id, const OrbisNpId* target_np_id, s32 slot_id, s32 ope_type,
                              s64 variable, const s64* compare_value,
                              const OrbisNpId* is_last_changed_author,
                              const Rtc::OrbisRtcTick* is_last_changed_date,
                              OrbisNpTusVariable* result_variable, u64 result_variable_size,
                              void* option) {
    auto ret = sceNpTusTryAndSetVariableAsync(
        req_id, target_np_id, slot_id, ope_type, variable, compare_value, is_last_changed_author,
        is_last_changed_date, result_variable, result_variable_size, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusTryAndSetVariableAsync(s32 req_id, const OrbisNpId* target_np_id, s32 slot_id,
                                   s32 ope_type, s64 variable, const s64* compare_value,
                                   const OrbisNpId* is_last_changed_author,
                                   const Rtc::OrbisRtcTick* is_last_changed_date,
                                   OrbisNpTusVariable* result_variable, u64 result_variable_size,
                                   void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_np_id = {}, slot_id = {}, ope_type = {}, variable = {}, "
             "compare_value = {}, result_variable = {}, result_variable_size = {}, option = {}",
             req_id, target_np_id ? target_np_id->handle.data : "", slot_id, ope_type, variable,
             fmt::ptr(compare_value), fmt::ptr(result_variable), result_variable_size,
             fmt::ptr(option));
    if (!target_np_id || !result_variable) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (ope_type < ORBIS_NP_TUS_OPETYPE_EQUAL || ope_type > ORBIS_NP_TUS_OPETYPE_LESS_OR_EQUAL) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (result_variable_size != sizeof(OrbisNpTusVariable)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", result_variable_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const std::string owner =
        target_np_id->handle.data[0] ? std::string(target_np_id->handle.data) : self;

    const bool has_author_check = is_last_changed_author != nullptr;
    const bool has_date_check = is_last_changed_date != nullptr;
    const std::string author_np_id =
        has_author_check ? std::string(is_last_changed_author->handle.data) : std::string();

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusTryAndSetVariable(
        uid, static_cast<s32>(svc), owner, std::string(), 0, slot_id, ope_type, variable,
        compare_value != nullptr, compare_value ? *compare_value : variable, has_author_check, 0,
        author_np_id, has_date_check, has_date_check ? is_last_changed_date->tick : 0,
        result_variable, nullptr, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusTryAndSetVariableVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s32 ope_type,
    s64 variable, const s64* compare_value, const OrbisNpId* is_last_changed_author,
    const Rtc::OrbisRtcTick* is_last_changed_date, OrbisNpTusVariable* result_variable,
    u64 result_variable_size, void* option) {
    auto ret = sceNpTusTryAndSetVariableVUserAsync(req_id, target_virtual_user_id, slot_id,
                                                   ope_type, variable, compare_value,
                                                   is_last_changed_author, is_last_changed_date,
                                                   result_variable, result_variable_size, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusTryAndSetVariableVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s32 ope_type,
    s64 variable, const s64* compare_value, const OrbisNpId* is_last_changed_author,
    const Rtc::OrbisRtcTick* is_last_changed_date, OrbisNpTusVariable* result_variable,
    u64 result_variable_size, void* option) {
    LOG_INFO(
        Lib_NpTus,
        "req_id = {}, target_virtual_user_id = {}, slot_id = {}, ope_type = {}, variable = {}, "
        "compare_value = {}, result_variable = {}, result_variable_size = {}, option = {}",
        req_id, target_virtual_user_id ? target_virtual_user_id->data : "", slot_id, ope_type,
        variable, fmt::ptr(compare_value), fmt::ptr(result_variable), result_variable_size,
        fmt::ptr(option));
    if (!target_virtual_user_id || !result_variable) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (ope_type < ORBIS_NP_TUS_OPETYPE_EQUAL || ope_type > ORBIS_NP_TUS_OPETYPE_LESS_OR_EQUAL) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (result_variable_size != sizeof(OrbisNpTusVariable)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", result_variable_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtual_user(
        target_virtual_user_id->data,
        strnlen(target_virtual_user_id->data, sizeof(target_virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid virtual user");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    const bool has_author_check = is_last_changed_author != nullptr;
    const bool has_date_check = is_last_changed_date != nullptr;
    const std::string author_np_id =
        has_author_check ? std::string(is_last_changed_author->handle.data) : std::string();

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusTryAndSetVariable(
        uid, static_cast<s32>(svc), std::string(), virtual_user, 0, slot_id, ope_type, variable,
        compare_value != nullptr, compare_value ? *compare_value : variable, has_author_check, 0,
        author_np_id, has_date_check, has_date_check ? is_last_changed_date->tick : 0,
        result_variable, nullptr, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusAddAndGetVariableA(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id,
                               s64 in_variable, const OrbisNpAccountId* is_last_changed_author,
                               const Rtc::OrbisRtcTick* is_last_changed_date,
                               OrbisNpTusVariableA* out_variable, u64 out_variable_size,
                               void* option) {
    auto ret = sceNpTusAddAndGetVariableAAsync(req_id, target_account_id, slot_id, in_variable,
                                               is_last_changed_author, is_last_changed_date,
                                               out_variable, out_variable_size, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusAddAndGetVariableAAsync(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id,
                                    s64 in_variable, const OrbisNpAccountId* is_last_changed_author,
                                    const Rtc::OrbisRtcTick* is_last_changed_date,
                                    OrbisNpTusVariableA* out_variable, u64 out_variable_size,
                                    void* option) {
    LOG_INFO(
        Lib_NpTus,
        "req_id = {}, target_account_id = {}, slot_id = {}, in_variable = {}, out_variable = {}, "
        "out_variable_size = {}, option = {}",
        req_id, target_account_id, slot_id, in_variable, fmt::ptr(out_variable), out_variable_size,
        fmt::ptr(option));
    if (option || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!out_variable) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (out_variable_size != sizeof(OrbisNpTusVariableA)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", out_variable_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const bool has_author_check = is_last_changed_author != nullptr;
    const bool has_date_check = is_last_changed_date != nullptr;

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusAddAndGetVariable(
        uid, static_cast<s32>(svc), std::string(), std::string(),
        static_cast<s64>(target_account_id), slot_id, in_variable, has_author_check,
        has_author_check ? static_cast<s64>(*is_last_changed_author) : 0, std::string(),
        has_date_check, has_date_check ? is_last_changed_date->tick : 0, nullptr, out_variable,
        ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusAddAndGetVariableAVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s64 in_variable,
    const OrbisNpAccountId* is_last_changed_author, const Rtc::OrbisRtcTick* is_last_changed_date,
    OrbisNpTusVariableA* out_variable, u64 out_variable_size, void* option) {
    auto ret = sceNpTusAddAndGetVariableAVUserAsync(
        req_id, target_virtual_user_id, slot_id, in_variable, is_last_changed_author,
        is_last_changed_date, out_variable, out_variable_size, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusAddAndGetVariableAVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s64 in_variable,
    const OrbisNpAccountId* is_last_changed_author, const Rtc::OrbisRtcTick* is_last_changed_date,
    OrbisNpTusVariableA* out_variable, u64 out_variable_size, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_virtual_user_id = {}, slot_id = {}, in_variable = {}, "
             "out_variable = {}, out_variable_size = {}, option = {}",
             req_id, target_virtual_user_id ? target_virtual_user_id->data : "", slot_id,
             in_variable, fmt::ptr(out_variable), out_variable_size, fmt::ptr(option));
    if (!target_virtual_user_id || !out_variable) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (out_variable_size != sizeof(OrbisNpTusVariableA)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", out_variable_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtual_user(
        target_virtual_user_id->data,
        strnlen(target_virtual_user_id->data, sizeof(target_virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const bool has_author_check = is_last_changed_author != nullptr;
    const bool has_date_check = is_last_changed_date != nullptr;

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusAddAndGetVariable(
        uid, static_cast<s32>(svc), std::string(), virtual_user, 0, slot_id, in_variable,
        has_author_check, has_author_check ? static_cast<s64>(*is_last_changed_author) : 0,
        std::string(), has_date_check, has_date_check ? is_last_changed_date->tick : 0, nullptr,
        out_variable, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusDeleteMultiSlotData(s32 req_id, const OrbisNpId* target_np_id, s32* slot_ids,
                                s32 array_len, void* option) {
    auto ret = sceNpTusDeleteMultiSlotDataAsync(req_id, target_np_id, slot_ids, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusDeleteMultiSlotDataAsync(s32 req_id, const OrbisNpId* target_np_id, s32* slot_ids,
                                     s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_np_id = {}, slot_ids = {}, array_len = {}, option = {}", req_id,
             target_np_id ? target_np_id->handle.data : "", fmt::ptr(slot_ids), array_len,
             fmt::ptr(option));
    if (!target_np_id || !slot_ids || array_len < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const std::string owner =
        target_np_id->handle.data[0] ? std::string(target_np_id->handle.data) : self;
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusDeleteMultiSlotData(uid, static_cast<s32>(svc), owner,
                                                              std::string(), 0, slots, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusDeleteMultiSlotDataVUser(s32 req_id,
                                     const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                     s32* slot_ids, s32 array_len, void* option) {
    auto ret = sceNpTusDeleteMultiSlotDataVUserAsync(req_id, target_virtual_user_id, slot_ids,
                                                     array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusDeleteMultiSlotDataVUserAsync(s32 req_id,
                                          const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                          s32* slot_ids, s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_virtual_user_id = {}, slot_ids = {}, array_len = {}, option = {}",
             req_id, target_virtual_user_id ? target_virtual_user_id->data : "", fmt::ptr(slot_ids),
             array_len, fmt::ptr(option));
    if (!target_virtual_user_id || !slot_ids || array_len < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const std::string virtual_user(
        target_virtual_user_id->data,
        strnlen(target_virtual_user_id->data, sizeof(target_virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusDeleteMultiSlotData(
        uid, static_cast<s32>(svc), std::string(), virtual_user, 0, slots, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusDeleteMultiSlotVariable(s32 req_id, const OrbisNpId* target_np_id, s32* slot_ids,
                                    s32 array_len, void* option) {
    auto ret =
        sceNpTusDeleteMultiSlotVariableAsync(req_id, target_np_id, slot_ids, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusDeleteMultiSlotVariableAsync(s32 req_id, const OrbisNpId* target_np_id, s32* slot_ids,
                                         s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_np_id = {}, slot_ids = {}, array_len = {}, option = {}", req_id,
             target_np_id ? target_np_id->handle.data : "", fmt::ptr(slot_ids), array_len,
             fmt::ptr(option));
    if (!target_np_id || !slot_ids || array_len < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const std::string owner =
        target_np_id->handle.data[0] ? std::string(target_np_id->handle.data) : self;
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusDeleteMultiSlotVariable(uid, static_cast<s32>(svc), owner,
                                                                  std::string(), 0, slots, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusDeleteMultiSlotVariableVUser(s32 req_id,
                                         const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                         s32* slot_ids, s32 array_len, void* option) {
    auto ret = sceNpTusDeleteMultiSlotVariableVUserAsync(req_id, target_virtual_user_id, slot_ids,
                                                         array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusDeleteMultiSlotVariableVUserAsync(s32 req_id,
                                              const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                              s32* slot_ids, s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_virtual_user_id = {}, slot_ids = {}, array_len = {}, option = {}",
             req_id, target_virtual_user_id ? target_virtual_user_id->data : "", fmt::ptr(slot_ids),
             array_len, fmt::ptr(option));
    if (!target_virtual_user_id || !slot_ids || array_len < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    const std::string virtual_user(
        target_virtual_user_id->data,
        strnlen(target_virtual_user_id->data, sizeof(target_virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusDeleteMultiSlotVariable(
        uid, static_cast<s32>(svc), std::string(), virtual_user, 0, slots, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetData(s32 req_id, const OrbisNpId* target_np_id, s32 slot_id,
                    OrbisNpTusDataStatus* data_status, u64 data_status_size, void* data,
                    u64 recv_size, void* option) {
    auto ret = sceNpTusGetDataAsync(req_id, target_np_id, slot_id, data_status, data_status_size,
                                    data, recv_size, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetDataA(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id,
                     OrbisNpTusDataStatusA* data_status, u64 data_status_size, void* data,
                     u64 recv_size, void* option) {
    auto ret = sceNpTusGetDataAAsync(req_id, target_account_id, slot_id, data_status,
                                     data_status_size, data, recv_size, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetDataAVUser(s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id,
                          s32 slot_id, OrbisNpTusDataStatusA* data_status, u64 data_status_size,
                          void* data, u64 recv_size, void* option) {
    auto ret = sceNpTusGetDataAVUserAsync(req_id, target_virtual_user_id, slot_id, data_status,
                                          data_status_size, data, recv_size, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetDataVUser(s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id,
                         s32 slot_id, OrbisNpTusDataStatus* data_status, u64 data_status_size,
                         void* data, u64 recv_size, void* option) {
    auto ret = sceNpTusGetDataVUserAsync(req_id, target_virtual_user_id, slot_id, data_status,
                                         data_status_size, data, recv_size, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiSlotDataStatusA(s32 req_id, OrbisNpAccountId target_account_id, s32* slot_ids,
                                    OrbisNpTusDataStatusA* status_array, u64 status_array_size,
                                    s32 array_len, void* option) {
    auto ret = sceNpTusGetMultiSlotDataStatusAAsync(
        req_id, target_account_id, slot_ids, status_array, status_array_size, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiSlotDataStatusAAsync(s32 req_id, OrbisNpAccountId target_account_id,
                                         s32* slot_ids, OrbisNpTusDataStatusA* status_array,
                                         u64 status_array_size, s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_account_id = {}, slot_ids = {}, status_array = {}, "
             "status_array_size = {}, array_len = {}, option = {}",
             req_id, target_account_id, fmt::ptr(slot_ids), fmt::ptr(status_array),
             status_array_size, array_len, fmt::ptr(option));
    if (!slot_ids || !status_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || array_len < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (status_array_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusDataStatusA)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", status_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiSlotDataStatus(
        uid, static_cast<s32>(svc), std::string(), std::string(),
        static_cast<s64>(target_account_id), slots, nullptr, status_array,
        static_cast<u64>(array_len), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiSlotDataStatusAVUser(s32 req_id,
                                         const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                         s32* slot_ids, OrbisNpTusDataStatusA* status_array,
                                         u64 status_array_size, s32 array_len, void* option) {
    auto ret = sceNpTusGetMultiSlotDataStatusAVUserAsync(req_id, target_virtual_user_id, slot_ids,
                                                         status_array, status_array_size, array_len,
                                                         option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiSlotDataStatusAVUserAsync(s32 req_id,
                                              const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                              s32* slot_ids, OrbisNpTusDataStatusA* status_array,
                                              u64 status_array_size, s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_virtual_user_id = {}, slot_ids = {}, status_array = {}, "
             "status_array_size = {}, array_len = {}, option = {}",
             req_id, target_virtual_user_id ? target_virtual_user_id->data : "", fmt::ptr(slot_ids),
             fmt::ptr(status_array), status_array_size, array_len, fmt::ptr(option));
    if (!target_virtual_user_id || !slot_ids || !status_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || array_len < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (status_array_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusDataStatusA)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", status_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtual_user(
        target_virtual_user_id->data,
        strnlen(target_virtual_user_id->data, sizeof(target_virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiSlotDataStatus(
        uid, static_cast<s32>(svc), std::string(), virtual_user, 0, slots, nullptr, status_array,
        static_cast<u64>(array_len), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiSlotVariableA(s32 req_id, OrbisNpAccountId target_account_id, s32* slot_ids,
                                  OrbisNpTusVariableA* variable_array, u64 variables_size,
                                  s32 array_len, void* option) {
    auto ret = sceNpTusGetMultiSlotVariableAAsync(
        req_id, target_account_id, slot_ids, variable_array, variables_size, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiUserDataStatus(s32 req_id, const OrbisNpId* target_np_id_array, s32 slot_id,
                                   OrbisNpTusDataStatus* status_array, u64 status_array_size,
                                   s32 array_len, void* option) {
    auto ret = sceNpTusGetMultiUserDataStatusAsync(
        req_id, target_np_id_array, slot_id, status_array, status_array_size, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiUserDataStatusAsync(s32 req_id, const OrbisNpId* target_np_id_array,
                                        s32 slot_id, OrbisNpTusDataStatus* status_array,
                                        u64 status_array_size, s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_np_id_array = {}, slot_id = {}, status_array = {}, "
             "status_array_size = {}, array_len = {}, option = {}",
             req_id, fmt::ptr(target_np_id_array), slot_id, fmt::ptr(status_array),
             status_array_size, array_len, fmt::ptr(option));
    if (!target_np_id_array || !status_array || array_len < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (status_array_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusDataStatus)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", status_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<std::string> npids;
    npids.reserve(array_len);
    for (s32 i = 0; i < array_len; ++i) {
        const char* h = target_np_id_array[i].handle.data;
        npids.emplace_back(
            h[0] ? std::string(h, strnlen(h, sizeof(target_np_id_array[i].handle.data))) : self);
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiUserDataStatus(
        uid, static_cast<s32>(svc), slot_id, npids, {}, {}, status_array, nullptr,
        static_cast<u64>(array_len), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiUserDataStatusVUser(s32 req_id,
                                        const OrbisNpTusVirtualUserId* target_virtual_user_id_array,
                                        s32 slot_id, OrbisNpTusDataStatus* status_array,
                                        u64 status_array_size, s32 array_len, void* option) {
    auto ret = sceNpTusGetMultiUserDataStatusVUserAsync(req_id, target_virtual_user_id_array,
                                                        slot_id, status_array, status_array_size,
                                                        array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiUserDataStatusVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id_array, s32 slot_id,
    OrbisNpTusDataStatus* status_array, u64 status_array_size, s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_virtual_user_id_array = {}, slot_id = {}, status_array = {}, "
             "status_array_size = {}, array_len = {}, option = {}",
             req_id, fmt::ptr(target_virtual_user_id_array), slot_id, fmt::ptr(status_array),
             status_array_size, array_len, fmt::ptr(option));
    if (!target_virtual_user_id_array || !status_array || array_len < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (status_array_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusDataStatus)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", status_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    std::vector<std::string> vusers;
    vusers.reserve(array_len);
    for (s32 i = 0; i < array_len; ++i) {
        const char* d = target_virtual_user_id_array[i].data;
        std::string name(d, strnlen(d, sizeof(target_virtual_user_id_array[i].data)));
        if (name.empty()) {
            LOG_ERROR(Lib_NpTus, "invalid argument");
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
        }
        vusers.emplace_back(std::move(name));
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiUserDataStatus(
        uid, static_cast<s32>(svc), slot_id, {}, vusers, {}, status_array, nullptr,
        static_cast<u64>(array_len), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiUserVariable(s32 req_id, const OrbisNpId* target_np_id_array, s32 slot_id,
                                 OrbisNpTusVariable* variable_array, u64 variable_array_size,
                                 s32 array_len, void* option) {
    auto ret =
        sceNpTusGetMultiUserVariableAsync(req_id, target_np_id_array, slot_id, variable_array,
                                          variable_array_size, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiUserVariableAsync(s32 req_id, const OrbisNpId* target_np_id_array, s32 slot_id,
                                      OrbisNpTusVariable* variable_array, u64 variable_array_size,
                                      s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_np_id_array = {}, slot_id = {}, variable_array = {}, "
             "variable_array_size = {}, array_len = {}, option = {}",
             req_id, fmt::ptr(target_np_id_array), slot_id, fmt::ptr(variable_array),
             variable_array_size, array_len, fmt::ptr(option));
    if (!target_np_id_array || !variable_array || array_len < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (variable_array_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusVariable)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", variable_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<std::string> npids;
    npids.reserve(array_len);
    for (s32 i = 0; i < array_len; ++i) {
        const char* h = target_np_id_array[i].handle.data;
        npids.emplace_back(
            h[0] ? std::string(h, strnlen(h, sizeof(target_np_id_array[i].handle.data))) : self);
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiUserVariable(
        uid, static_cast<s32>(svc), slot_id, npids, {}, {}, variable_array, nullptr,
        static_cast<u64>(array_len), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiUserVariableVUser(s32 req_id,
                                      const OrbisNpTusVirtualUserId* target_virtual_user_id_array,
                                      s32 slot_id, OrbisNpTusVariable* variable_array,
                                      u64 variable_array_size, s32 array_len, void* option) {
    auto ret = sceNpTusGetMultiUserVariableVUserAsync(req_id, target_virtual_user_id_array, slot_id,
                                                      variable_array, variable_array_size,
                                                      array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiUserVariableVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id_array, s32 slot_id,
    OrbisNpTusVariable* variable_array, u64 variable_array_size, s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_virtual_user_id_array = {}, slot_id = {}, variable_array = {}, "
             "variable_array_size = {}, array_len = {}, option = {}",
             req_id, fmt::ptr(target_virtual_user_id_array), slot_id, fmt::ptr(variable_array),
             variable_array_size, array_len, fmt::ptr(option));
    if (!target_virtual_user_id_array || !variable_array || array_len < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (variable_array_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusVariable)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", variable_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    std::vector<std::string> vusers;
    vusers.reserve(array_len);
    for (s32 i = 0; i < array_len; ++i) {
        const char* d = target_virtual_user_id_array[i].data;
        std::string name(d, strnlen(d, sizeof(target_virtual_user_id_array[i].data)));
        if (name.empty()) {
            LOG_ERROR(Lib_NpTus, "invalid argument");
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
        }
        vusers.emplace_back(std::move(name));
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiUserVariable(
        uid, static_cast<s32>(svc), slot_id, {}, vusers, {}, variable_array, nullptr,
        static_cast<u64>(array_len), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusSetData(s32 req_id, const OrbisNpId* target_np_id, s32 slot_id, u64 total_size,
                    u64 send_size, const void* data, const OrbisNpTusDataInfo* info,
                    u64 info_struct_size, const OrbisNpId* is_last_changed_author,
                    const Rtc::OrbisRtcTick* is_last_changed_date, void* option) {
    auto ret = sceNpTusSetDataAsync(req_id, target_np_id, slot_id, total_size, send_size, data,
                                    info, info_struct_size, is_last_changed_author,
                                    is_last_changed_date, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusSetDataA(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id, u64 total_size,
                     u64 send_size, const void* data, const OrbisNpTusDataInfo* info,
                     u64 info_struct_size, const OrbisNpAccountId* is_last_changed_author,
                     const Rtc::OrbisRtcTick* is_last_changed_date, void* option) {
    auto ret = sceNpTusSetDataAAsync(req_id, target_account_id, slot_id, total_size, send_size,
                                     data, info, info_struct_size, is_last_changed_author,
                                     is_last_changed_date, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusSetDataAVUser(s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id,
                          s32 slot_id, u64 total_size, u64 send_size, const void* data,
                          const OrbisNpTusDataInfo* info, u64 info_struct_size,
                          const OrbisNpAccountId* is_last_changed_author,
                          const Rtc::OrbisRtcTick* is_last_changed_date, void* option) {
    auto ret = sceNpTusSetDataAVUserAsync(req_id, target_virtual_user_id, slot_id, total_size,
                                          send_size, data, info, info_struct_size,
                                          is_last_changed_author, is_last_changed_date, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusSetDataVUser(s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id,
                         s32 slot_id, u64 total_size, u64 send_size, const void* data,
                         const OrbisNpTusDataInfo* info, u64 info_struct_size,
                         const OrbisNpId* is_last_changed_author,
                         const Rtc::OrbisRtcTick* is_last_changed_date, void* option) {
    auto ret = sceNpTusSetDataVUserAsync(req_id, target_virtual_user_id, slot_id, total_size,
                                         send_size, data, info, info_struct_size,
                                         is_last_changed_author, is_last_changed_date, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusSetMultiSlotVariableA(s32 req_id, OrbisNpAccountId target_account_id, s32* slot_ids,
                                  const s64* variables, s32 array_len, void* option) {
    auto ret = sceNpTusSetMultiSlotVariableAAsync(req_id, target_account_id, slot_ids, variables,
                                                  array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusTryAndSetVariableA(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id,
                               s32 ope_type, s64 variable, const s64* compare_value,
                               const OrbisNpAccountId* is_last_changed_author,
                               const Rtc::OrbisRtcTick* is_last_changed_date,
                               OrbisNpTusVariableA* result_variable, u64 result_variable_size,
                               void* option) {
    auto ret =
        sceNpTusTryAndSetVariableAAsync(req_id, target_account_id, slot_id, ope_type, variable,
                                        compare_value, is_last_changed_author, is_last_changed_date,
                                        result_variable, result_variable_size, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusTryAndSetVariableAAsync(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id,
                                    s32 ope_type, s64 variable, const s64* compare_value,
                                    const OrbisNpAccountId* is_last_changed_author,
                                    const Rtc::OrbisRtcTick* is_last_changed_date,
                                    OrbisNpTusVariableA* result_variable, u64 result_variable_size,
                                    void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_account_id = {}, slot_id = {}, ope_type = {}, variable = {}, "
             "compare_value = {}, result_variable = {}, result_variable_size = {}, option = {}",
             req_id, target_account_id, slot_id, ope_type, variable, fmt::ptr(compare_value),
             fmt::ptr(result_variable), result_variable_size, fmt::ptr(option));
    if (!result_variable) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (ope_type < ORBIS_NP_TUS_OPETYPE_EQUAL || ope_type > ORBIS_NP_TUS_OPETYPE_LESS_OR_EQUAL) {
        LOG_ERROR(Lib_NpTus, "invalid opeType {}", ope_type);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_TYPE;
    }
    if (result_variable_size != sizeof(OrbisNpTusVariableA)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", result_variable_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const bool has_author_check = is_last_changed_author != nullptr;
    const bool has_date_check = is_last_changed_date != nullptr;

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusTryAndSetVariable(
        uid, static_cast<s32>(svc), std::string(), std::string(),
        static_cast<s64>(target_account_id), slot_id, ope_type, variable, compare_value != nullptr,
        compare_value ? *compare_value : variable, has_author_check,
        has_author_check ? static_cast<s64>(*is_last_changed_author) : 0, std::string(),
        has_date_check, has_date_check ? is_last_changed_date->tick : 0, nullptr, result_variable,
        ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusTryAndSetVariableAVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s32 ope_type,
    s64 variable, const s64* compare_value, const OrbisNpAccountId* is_last_changed_author,
    const Rtc::OrbisRtcTick* is_last_changed_date, OrbisNpTusVariableA* result_variable,
    u64 result_variable_size, void* option) {
    auto ret = sceNpTusTryAndSetVariableAVUserAsync(req_id, target_virtual_user_id, slot_id,
                                                    ope_type, variable, compare_value,
                                                    is_last_changed_author, is_last_changed_date,
                                                    result_variable, result_variable_size, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusTryAndSetVariableAVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s32 ope_type,
    s64 variable, const s64* compare_value, const OrbisNpAccountId* is_last_changed_author,
    const Rtc::OrbisRtcTick* is_last_changed_date, OrbisNpTusVariableA* result_variable,
    u64 result_variable_size, void* option) {
    LOG_INFO(
        Lib_NpTus,
        "req_id = {}, target_virtual_user_id = {}, slot_id = {}, ope_type = {}, variable = {}, "
        "compare_value = {}, result_variable = {}, result_variable_size = {}, option = {}",
        req_id, target_virtual_user_id ? target_virtual_user_id->data : "", slot_id, ope_type,
        variable, fmt::ptr(compare_value), fmt::ptr(result_variable), result_variable_size,
        fmt::ptr(option));
    if (!target_virtual_user_id || !result_variable) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (ope_type < ORBIS_NP_TUS_OPETYPE_EQUAL || ope_type > ORBIS_NP_TUS_OPETYPE_LESS_OR_EQUAL) {
        LOG_ERROR(Lib_NpTus, "invalid opeType {}", ope_type);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_TYPE;
    }
    if (result_variable_size != sizeof(OrbisNpTusVariableA)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", result_variable_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtual_user(
        target_virtual_user_id->data,
        strnlen(target_virtual_user_id->data, sizeof(target_virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const bool has_author_check = is_last_changed_author != nullptr;
    const bool has_date_check = is_last_changed_date != nullptr;

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusTryAndSetVariable(
        uid, static_cast<s32>(svc), std::string(), virtual_user, 0, slot_id, ope_type, variable,
        compare_value != nullptr, compare_value ? *compare_value : variable, has_author_check,
        has_author_check ? static_cast<s64>(*is_last_changed_author) : 0, std::string(),
        has_date_check, has_date_check ? is_last_changed_date->tick : 0, nullptr, result_variable,
        ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusDeleteMultiSlotDataAAsync(s32 req_id, OrbisNpAccountId target_account_id, s32* slot_ids,
                                      s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_account_id = {}, slot_ids = {}, array_len = {}, option = {}",
             req_id, target_account_id, fmt::ptr(slot_ids), array_len, fmt::ptr(option));
    if (!slot_ids) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || array_len < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusDeleteMultiSlotData(
        uid, static_cast<s32>(svc), std::string(), std::string(),
        static_cast<s64>(target_account_id), slots, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusDeleteMultiSlotDataA(s32 req_id, OrbisNpAccountId target_account_id, s32* slot_ids,
                                 s32 array_len, void* option) {
    auto ret =
        sceNpTusDeleteMultiSlotDataAAsync(req_id, target_account_id, slot_ids, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusDeleteMultiSlotVariableA(s32 req_id, OrbisNpAccountId target_account_id, s32* slot_ids,
                                     s32 array_len, void* option) {
    auto ret = sceNpTusDeleteMultiSlotVariableAAsync(req_id, target_account_id, slot_ids, array_len,
                                                     option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusDeleteMultiSlotVariableAAsync(s32 req_id, OrbisNpAccountId target_account_id,
                                          s32* slot_ids, s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_account_id = {}, slot_ids = {}, array_len = {}, option = {}",
             req_id, target_account_id, fmt::ptr(slot_ids), array_len, fmt::ptr(option));
    if (!slot_ids) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || array_len < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusDeleteMultiSlotVariable(
        uid, static_cast<s32>(svc), std::string(), std::string(),
        static_cast<s64>(target_account_id), slots, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetFriendsDataStatusA(s32 req_id, s32 slot_id, s32 include_self, s32 sort_type,
                                  OrbisNpTusDataStatusA* status_array, u64 status_array_size,
                                  s32 array_len, OrbisNpTusGetFriendsDataStatusOptParam* option) {
    auto ret =
        sceNpTusGetFriendsDataStatusAAsync(req_id, slot_id, include_self, sort_type, status_array,
                                           status_array_size, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetFriendsDataStatusAAsync(s32 req_id, s32 slot_id, s32 include_self, s32 sort_type,
                                       OrbisNpTusDataStatusA* status_array, u64 status_array_size,
                                       s32 array_len,
                                       OrbisNpTusGetFriendsDataStatusOptParam* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, slot_id = {}, include_self = {}, sort_type = {}, status_array = {}, "
             "status_array_size = {}, array_len = {}, option = {}",
             req_id, slot_id, include_self, sort_type, fmt::ptr(status_array), status_array_size,
             array_len, fmt::ptr(option));
    if (!status_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (array_len < 1 || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSelectedFriends) {
        LOG_ERROR(Lib_NpTus, "too many entries {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (sort_type < ORBIS_NP_TUS_DATASTATUS_SORTTYPE_DESCENDING_DATE ||
        sort_type > ORBIS_NP_TUS_DATASTATUS_SORTTYPE_ASCENDING_DATE) {
        LOG_ERROR(Lib_NpTus, "invalid sortType {}", sort_type);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (status_array_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusDataStatusA)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", status_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    u32 start_offset = 0;
    u32* hits_out = nullptr;
    if (option) {
        if (option->size != sizeof(OrbisNpTusGetFriendsDataStatusOptParam)) {
            LOG_ERROR(Lib_NpTus, "invalid option size {}", option->size);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        if (option->start_serial_rank) {
            const u32 rank = *option->start_serial_rank;
            if (rank == 0) {
                LOG_ERROR(Lib_NpTus, "invalid startSerialRank");
                return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
            }
            start_offset = rank - 1;
        }
        hits_out = option->hits;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetFriendsDataStatus(
        uid, static_cast<s32>(svc), slot_id, include_self != 0, sort_type,
        static_cast<u32>(array_len), nullptr, status_array, static_cast<u64>(array_len), ctx,
        start_offset, hits_out);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetFriendsVariableA(s32 req_id, s32 slot_id, s32 include_self, s32 sort_type,
                                OrbisNpTusVariableA* variable_array, u64 variable_array_size,
                                s32 array_len, OrbisNpTusGetFriendsVariableOptParam* option) {
    auto ret =
        sceNpTusGetFriendsVariableAAsync(req_id, slot_id, include_self, sort_type, variable_array,
                                         variable_array_size, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetFriendsVariableAAsync(s32 req_id, s32 slot_id, s32 include_self, s32 sort_type,
                                     OrbisNpTusVariableA* variable_array, u64 variable_array_size,
                                     s32 array_len, OrbisNpTusGetFriendsVariableOptParam* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, slot_id = {}, include_self = {}, sort_type = {}, variable_array = {}, "
             "variable_array_size = {}, array_len = {}, option = {}",
             req_id, slot_id, include_self, sort_type, fmt::ptr(variable_array),
             variable_array_size, array_len, fmt::ptr(option));
    if (!variable_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (array_len < 1 || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSelectedFriends) {
        LOG_ERROR(Lib_NpTus, "too many entries {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (sort_type < ORBIS_NP_TUS_VARIABLE_SORTTYPE_DESCENDING_DATE ||
        sort_type > ORBIS_NP_TUS_VARIABLE_SORTTYPE_ASCENDING_VALUE) {
        LOG_ERROR(Lib_NpTus, "invalid sortType {}", sort_type);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (variable_array_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusVariableA)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", variable_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    u32 start_offset = 0;
    u32* hits_out = nullptr;
    if (option) {
        if (option->size != sizeof(OrbisNpTusGetFriendsVariableOptParam)) {
            LOG_ERROR(Lib_NpTus, "invalid option size {}", option->size);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        if (option->start_serial_rank) {
            const u32 rank = *option->start_serial_rank;
            if (rank == 0) {
                LOG_ERROR(Lib_NpTus, "invalid startSerialRank");
                return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
            }
            start_offset = rank - 1;
        }
        hits_out = option->hits;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetFriendsVariable(
        uid, static_cast<s32>(svc), slot_id, include_self != 0, sort_type,
        static_cast<u32>(array_len), nullptr, variable_array, static_cast<u64>(array_len), ctx,
        start_offset, hits_out);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiSlotVariableVUser(s32 req_id,
                                      const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                      s32* slot_ids, OrbisNpTusVariable* variable_array,
                                      u64 variable_array_size, s32 array_len, void* option) {
    auto ret = sceNpTusGetMultiSlotVariableVUserAsync(req_id, target_virtual_user_id, slot_ids,
                                                      variable_array, variable_array_size,
                                                      array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiSlotVariableVUserAsync(s32 req_id,
                                           const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                           s32* slot_ids, OrbisNpTusVariable* variable_array,
                                           u64 variable_array_size, s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_virtual_user_id = {}, slot_ids = {}, variable_array = {}, "
             "variable_array_size = {}, array_len = {}, option = {}",
             req_id, target_virtual_user_id ? target_virtual_user_id->data : "", fmt::ptr(slot_ids),
             fmt::ptr(variable_array), variable_array_size, array_len, fmt::ptr(option));
    if (!target_virtual_user_id || !slot_ids || !variable_array || array_len < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (variable_array_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusVariable)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", variable_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtual_user(
        target_virtual_user_id->data,
        strnlen(target_virtual_user_id->data, sizeof(target_virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiSlotVariable(
        uid, static_cast<s32>(svc), std::string(), virtual_user, slots, variable_array,
        static_cast<u64>(array_len), ctx, nullptr, 0);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiUserDataStatusA(s32 req_id, const OrbisNpAccountId* target_account_id_array,
                                    s32 slot_id, OrbisNpTusDataStatusA* status_array,
                                    u64 status_array_size, s32 array_len, void* option) {
    auto ret =
        sceNpTusGetMultiUserDataStatusAAsync(req_id, target_account_id_array, slot_id, status_array,
                                             status_array_size, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiUserDataStatusAAsync(s32 req_id,
                                         const OrbisNpAccountId* target_account_id_array,
                                         s32 slot_id, OrbisNpTusDataStatusA* status_array,
                                         u64 status_array_size, s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_account_id_array = {}, slot_id = {}, status_array = {}, "
             "status_array_size = {}, array_len = {}, option = {}",
             req_id, fmt::ptr(target_account_id_array), slot_id, fmt::ptr(status_array),
             status_array_size, array_len, fmt::ptr(option));
    if (!target_account_id_array || !status_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || array_len < 1 || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (status_array_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusDataStatusA)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", status_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s64> accounts;
    accounts.reserve(array_len);
    for (s32 i = 0; i < array_len; ++i) {
        accounts.push_back(static_cast<s64>(target_account_id_array[i]));
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiUserDataStatus(
        uid, static_cast<s32>(svc), slot_id, {}, {}, accounts, nullptr, status_array,
        static_cast<u64>(array_len), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiUserDataStatusAVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id_array, s32 slot_id,
    OrbisNpTusDataStatusA* status_array, u64 status_array_size, s32 array_len, void* option) {
    auto ret = sceNpTusGetMultiUserDataStatusAVUserAsync(req_id, target_virtual_user_id_array,
                                                         slot_id, status_array, status_array_size,
                                                         array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiUserDataStatusAVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id_array, s32 slot_id,
    OrbisNpTusDataStatusA* status_array, u64 status_array_size, s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_virtual_user_id_array = {}, slot_id = {}, status_array = {}, "
             "status_array_size = {}, array_len = {}, option = {}",
             req_id, fmt::ptr(target_virtual_user_id_array), slot_id, fmt::ptr(status_array),
             status_array_size, array_len, fmt::ptr(option));
    if (!target_virtual_user_id_array || !status_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || array_len < 1 || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (status_array_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusDataStatusA)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", status_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    std::vector<std::string> vusers;
    vusers.reserve(array_len);
    for (s32 i = 0; i < array_len; ++i) {
        const char* d = target_virtual_user_id_array[i].data;
        std::string name(d, strnlen(d, sizeof(target_virtual_user_id_array[i].data)));
        if (name.empty()) {
            LOG_ERROR(Lib_NpTus, "invalid virtual user {}", i);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
        }
        vusers.emplace_back(std::move(name));
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiUserDataStatus(
        uid, static_cast<s32>(svc), slot_id, {}, vusers, {}, nullptr, status_array,
        static_cast<u64>(array_len), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiUserVariableA(s32 req_id, const OrbisNpAccountId* target_account_id_array,
                                  s32 slot_id, OrbisNpTusVariableA* variable_array,
                                  u64 variable_array_size, s32 array_len, void* option) {
    auto ret =
        sceNpTusGetMultiUserVariableAAsync(req_id, target_account_id_array, slot_id, variable_array,
                                           variable_array_size, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiUserVariableAAsync(s32 req_id, const OrbisNpAccountId* target_account_id_array,
                                       s32 slot_id, OrbisNpTusVariableA* variable_array,
                                       u64 variable_array_size, s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_account_id_array = {}, slot_id = {}, variable_array = {}, "
             "variable_array_size = {}, array_len = {}, option = {}",
             req_id, fmt::ptr(target_account_id_array), slot_id, fmt::ptr(variable_array),
             variable_array_size, array_len, fmt::ptr(option));
    if (!target_account_id_array || !variable_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || array_len < 1 || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (variable_array_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusVariableA)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", variable_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s64> accounts;
    accounts.reserve(array_len);
    for (s32 i = 0; i < array_len; ++i) {
        accounts.push_back(static_cast<s64>(target_account_id_array[i]));
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiUserVariable(
        uid, static_cast<s32>(svc), slot_id, {}, {}, accounts, nullptr, variable_array,
        static_cast<u64>(array_len), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiUserVariableAVUser(s32 req_id,
                                       const OrbisNpTusVirtualUserId* target_virtual_user_id_array,
                                       s32 slot_id, OrbisNpTusVariableA* variable_array,
                                       u64 variable_array_size, s32 array_len, void* option) {
    auto ret = sceNpTusGetMultiUserVariableAVUserAsync(req_id, target_virtual_user_id_array,
                                                       slot_id, variable_array, variable_array_size,
                                                       array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiUserVariableAVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id_array, s32 slot_id,
    OrbisNpTusVariableA* variable_array, u64 variable_array_size, s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_virtual_user_id_array = {}, slot_id = {}, variable_array = {}, "
             "variable_array_size = {}, array_len = {}, option = {}",
             req_id, fmt::ptr(target_virtual_user_id_array), slot_id, fmt::ptr(variable_array),
             variable_array_size, array_len, fmt::ptr(option));
    if (!target_virtual_user_id_array || !variable_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || array_len < 1 || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (variable_array_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusVariableA)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", variable_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    std::vector<std::string> vusers;
    vusers.reserve(array_len);
    for (s32 i = 0; i < array_len; ++i) {
        const char* d = target_virtual_user_id_array[i].data;
        std::string name(d, strnlen(d, sizeof(target_virtual_user_id_array[i].data)));
        if (name.empty()) {
            LOG_ERROR(Lib_NpTus, "invalid virtual user {}", i);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
        }
        vusers.emplace_back(std::move(name));
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiUserVariable(
        uid, static_cast<s32>(svc), slot_id, {}, vusers, {}, nullptr, variable_array,
        static_cast<u64>(array_len), ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusAddAndGetVariableForCrossSave(s32 req_id, OrbisNpAccountId target_account_id,
                                          s32 slot_id, s64 in_variable,
                                          const OrbisNpAccountId* is_last_changed_author,
                                          const Rtc::OrbisRtcTick* is_last_changed_date,
                                          OrbisNpTusVariableForCrossSave* out_variable,
                                          u64 out_variable_size, void* option) {
    auto ret = sceNpTusAddAndGetVariableForCrossSaveAsync(
        req_id, target_account_id, slot_id, in_variable, is_last_changed_author,
        is_last_changed_date, out_variable, out_variable_size, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusAddAndGetVariableForCrossSaveAsync(s32 req_id, OrbisNpAccountId target_account_id,
                                               s32 slot_id, s64 in_variable,
                                               const OrbisNpAccountId* is_last_changed_author,
                                               const Rtc::OrbisRtcTick* is_last_changed_date,
                                               OrbisNpTusVariableForCrossSave* out_variable,
                                               u64 out_variable_size, void* option) {
    LOG_INFO(
        Lib_NpTus,
        "req_id = {}, target_account_id = {}, slot_id = {}, in_variable = {}, out_variable = {}, "
        "out_variable_size = {}, option = {}",
        req_id, target_account_id, slot_id, in_variable, fmt::ptr(out_variable), out_variable_size,
        fmt::ptr(option));
    if (option || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!out_variable) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (out_variable_size != sizeof(OrbisNpTusVariableForCrossSave)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", out_variable_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const bool has_author_check = is_last_changed_author != nullptr;
    const bool has_date_check = is_last_changed_date != nullptr;

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusAddAndGetVariable(
        uid, static_cast<s32>(svc), std::string(), std::string(),
        static_cast<s64>(target_account_id), slot_id, in_variable, has_author_check,
        has_author_check ? static_cast<s64>(*is_last_changed_author) : 0, std::string(),
        has_date_check, has_date_check ? is_last_changed_date->tick : 0, nullptr, nullptr, ctx,
        out_variable);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusAddAndGetVariableForCrossSaveVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s64 in_variable,
    const OrbisNpAccountId* is_last_changed_author, const Rtc::OrbisRtcTick* is_last_changed_date,
    OrbisNpTusVariableForCrossSave* out_variable, u64 out_variable_size, void* option) {
    auto ret = sceNpTusAddAndGetVariableForCrossSaveVUserAsync(
        req_id, target_virtual_user_id, slot_id, in_variable, is_last_changed_author,
        is_last_changed_date, out_variable, out_variable_size, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusAddAndGetVariableForCrossSaveVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s64 in_variable,
    const OrbisNpAccountId* is_last_changed_author, const Rtc::OrbisRtcTick* is_last_changed_date,
    OrbisNpTusVariableForCrossSave* out_variable, u64 out_variable_size, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_virtual_user_id = {}, slot_id = {}, in_variable = {}, "
             "out_variable = {}, out_variable_size = {}, option = {}",
             req_id, target_virtual_user_id ? target_virtual_user_id->data : "", slot_id,
             in_variable, fmt::ptr(out_variable), out_variable_size, fmt::ptr(option));
    if (!target_virtual_user_id || !out_variable) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (out_variable_size != sizeof(OrbisNpTusVariableForCrossSave)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", out_variable_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtual_user(
        target_virtual_user_id->data,
        strnlen(target_virtual_user_id->data, sizeof(target_virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const bool has_author_check = is_last_changed_author != nullptr;
    const bool has_date_check = is_last_changed_date != nullptr;

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusAddAndGetVariable(
        uid, static_cast<s32>(svc), std::string(), virtual_user, 0, slot_id, in_variable,
        has_author_check, has_author_check ? static_cast<s64>(*is_last_changed_author) : 0,
        std::string(), has_date_check, has_date_check ? is_last_changed_date->tick : 0, nullptr,
        nullptr, ctx, out_variable);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetDataForCrossSave(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id,
                                OrbisNpTusDataStatusForCrossSave* data_status, u64 data_status_size,
                                void* data, u64 recv_size, void* option) {
    auto ret = sceNpTusGetDataForCrossSaveAsync(req_id, target_account_id, slot_id, data_status,
                                                data_status_size, data, recv_size, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetDataForCrossSaveAsync(s32 req_id, OrbisNpAccountId target_account_id, s32 slot_id,
                                     OrbisNpTusDataStatusForCrossSave* data_status,
                                     u64 data_status_size, void* data, u64 recv_size,
                                     void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_account_id = {}, slot_id = {}, data_status = {}, "
             "data_status_size = {}, data = {}, recv_size = {}, option = {}",
             req_id, target_account_id, slot_id, fmt::ptr(data_status), data_status_size,
             fmt::ptr(data), recv_size, fmt::ptr(option));
    if (option || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!data_status || data_status_size == 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (data_status_size != sizeof(OrbisNpTusDataStatusForCrossSave)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", data_status_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    const s64 account = static_cast<s64>(target_account_id);
    u64 offset = 0;
    {
        std::lock_guard lock(g_mutex);
        if (!req->xfer.SameTarget(slot_id, account, std::string())) {
            req->xfer.Begin(slot_id, account, std::string());
        }
        offset = req->xfer.recv_offset;
        if (data) {
            req->xfer.recv_offset += recv_size;
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetData(
        uid, static_cast<s32>(svc), std::string(), std::string(), account, slot_id, nullptr,
        data_status_size, data, data ? recv_size : 0, offset, ctx, nullptr, data_status);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetDataForCrossSaveVUser(s32 req_id,
                                     const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                     s32 slot_id, OrbisNpTusDataStatusForCrossSave* data_status,
                                     u64 data_status_size, void* data, u64 recv_size,
                                     void* option) {
    auto ret =
        sceNpTusGetDataForCrossSaveVUserAsync(req_id, target_virtual_user_id, slot_id, data_status,
                                              data_status_size, data, recv_size, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetDataForCrossSaveVUserAsync(s32 req_id,
                                          const OrbisNpTusVirtualUserId* target_virtual_user_id,
                                          s32 slot_id,
                                          OrbisNpTusDataStatusForCrossSave* data_status,
                                          u64 data_status_size, void* data, u64 recv_size,
                                          void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_virtual_user_id = {}, slot_id = {}, data_status = {}, "
             "data_status_size = {}, data = {}, recv_size = {}, option = {}",
             req_id, target_virtual_user_id ? target_virtual_user_id->data : "", slot_id,
             fmt::ptr(data_status), data_status_size, fmt::ptr(data), recv_size, fmt::ptr(option));
    if (option || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (!target_virtual_user_id || !data_status || data_status_size == 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (data_status_size != sizeof(OrbisNpTusDataStatusForCrossSave)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", data_status_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtual_user(
        target_virtual_user_id->data,
        strnlen(target_virtual_user_id->data, sizeof(target_virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    u64 offset = 0;
    {
        std::lock_guard lock(g_mutex);
        if (!req->xfer.SameTarget(slot_id, 0, virtual_user)) {
            req->xfer.Begin(slot_id, 0, virtual_user);
        }
        offset = req->xfer.recv_offset;
        if (data) {
            req->xfer.recv_offset += recv_size;
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetData(
        uid, static_cast<s32>(svc), std::string(), virtual_user, 0, slot_id, nullptr,
        data_status_size, data, data ? recv_size : 0, offset, ctx, nullptr, data_status);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetFriendsDataStatusForCrossSave(s32 req_id, s32 slot_id, s32 include_self,
                                             s32 sort_type,
                                             OrbisNpTusDataStatusForCrossSave* status_array,
                                             u64 status_array_size, s32 array_len,
                                             OrbisNpTusGetFriendsDataStatusOptParam* option) {
    auto ret = sceNpTusGetFriendsDataStatusForCrossSaveAsync(req_id, slot_id, include_self,
                                                             sort_type, status_array,
                                                             status_array_size, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetFriendsDataStatusForCrossSaveAsync(s32 req_id, s32 slot_id, s32 include_self,
                                                  s32 sort_type,
                                                  OrbisNpTusDataStatusForCrossSave* status_array,
                                                  u64 status_array_size, s32 array_len,
                                                  OrbisNpTusGetFriendsDataStatusOptParam* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, slot_id = {}, include_self = {}, sort_type = {}, status_array = {}, "
             "status_array_size = {}, array_len = {}, option = {}",
             req_id, slot_id, include_self, sort_type, fmt::ptr(status_array), status_array_size,
             array_len, fmt::ptr(option));
    if (!status_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (array_len < 1 || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSelectedFriends) {
        LOG_ERROR(Lib_NpTus, "too many entries {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (sort_type < ORBIS_NP_TUS_DATASTATUS_SORTTYPE_DESCENDING_DATE ||
        sort_type > ORBIS_NP_TUS_DATASTATUS_SORTTYPE_ASCENDING_DATE) {
        LOG_ERROR(Lib_NpTus, "invalid sortType {}", sort_type);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (status_array_size !=
        static_cast<u64>(array_len) * sizeof(OrbisNpTusDataStatusForCrossSave)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", status_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    u32 start_offset = 0;
    u32* hits_out = nullptr;
    if (option) {
        if (option->size != sizeof(OrbisNpTusGetFriendsDataStatusOptParam)) {
            LOG_ERROR(Lib_NpTus, "invalid option size {}", option->size);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        if (option->start_serial_rank) {
            const u32 rank = *option->start_serial_rank;
            if (rank == 0) {
                LOG_ERROR(Lib_NpTus, "invalid startSerialRank");
                return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
            }
            start_offset = rank - 1;
        }
        hits_out = option->hits;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetFriendsDataStatus(
        uid, static_cast<s32>(svc), slot_id, include_self != 0, sort_type,
        static_cast<u32>(array_len), nullptr, nullptr, static_cast<u64>(array_len), ctx,
        start_offset, hits_out, status_array);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetFriendsVariableForCrossSave(s32 req_id, s32 slot_id, s32 include_self, s32 sort_type,
                                           OrbisNpTusVariableForCrossSave* variable_array,
                                           u64 variable_array_size, s32 array_len,
                                           OrbisNpTusGetFriendsVariableOptParam* option) {
    auto ret = sceNpTusGetFriendsVariableForCrossSaveAsync(req_id, slot_id, include_self, sort_type,
                                                           variable_array, variable_array_size,
                                                           array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetFriendsVariableForCrossSaveAsync(s32 req_id, s32 slot_id, s32 include_self,
                                                s32 sort_type,
                                                OrbisNpTusVariableForCrossSave* variable_array,
                                                u64 variable_array_size, s32 array_len,
                                                OrbisNpTusGetFriendsVariableOptParam* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, slot_id = {}, include_self = {}, sort_type = {}, variable_array = {}, "
             "variable_array_size = {}, array_len = {}, option = {}",
             req_id, slot_id, include_self, sort_type, fmt::ptr(variable_array),
             variable_array_size, array_len, fmt::ptr(option));
    if (!variable_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (array_len < 1 || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSelectedFriends) {
        LOG_ERROR(Lib_NpTus, "too many entries {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (sort_type < ORBIS_NP_TUS_VARIABLE_SORTTYPE_DESCENDING_DATE ||
        sort_type > ORBIS_NP_TUS_VARIABLE_SORTTYPE_ASCENDING_VALUE) {
        LOG_ERROR(Lib_NpTus, "invalid sortType {}", sort_type);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (variable_array_size !=
        static_cast<u64>(array_len) * sizeof(OrbisNpTusVariableForCrossSave)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", variable_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    u32 start_offset = 0;
    u32* hits_out = nullptr;
    if (option) {
        if (option->size != sizeof(OrbisNpTusGetFriendsVariableOptParam)) {
            LOG_ERROR(Lib_NpTus, "invalid option size {}", option->size);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        if (option->start_serial_rank) {
            const u32 rank = *option->start_serial_rank;
            if (rank == 0) {
                LOG_ERROR(Lib_NpTus, "invalid startSerialRank");
                return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
            }
            start_offset = rank - 1;
        }
        hits_out = option->hits;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetFriendsVariable(
        uid, static_cast<s32>(svc), slot_id, include_self != 0, sort_type,
        static_cast<u32>(array_len), nullptr, nullptr, static_cast<u64>(array_len), ctx,
        start_offset, hits_out, variable_array);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiSlotDataStatusForCrossSave(s32 req_id, OrbisNpAccountId target_account_id,
                                               s32* slot_ids,
                                               OrbisNpTusDataStatusForCrossSave* status_array,
                                               u64 status_array_size, s32 array_len, void* option) {
    auto ret = sceNpTusGetMultiSlotDataStatusForCrossSaveAsync(
        req_id, target_account_id, slot_ids, status_array, status_array_size, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiSlotDataStatusForCrossSaveAsync(s32 req_id, OrbisNpAccountId target_account_id,
                                                    s32* slot_ids,
                                                    OrbisNpTusDataStatusForCrossSave* status_array,
                                                    u64 status_array_size, s32 array_len,
                                                    void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_account_id = {}, slot_ids = {}, status_array = {}, "
             "status_array_size = {}, array_len = {}, option = {}",
             req_id, target_account_id, fmt::ptr(slot_ids), fmt::ptr(status_array),
             status_array_size, array_len, fmt::ptr(option));
    if (!slot_ids || !status_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || array_len < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (status_array_size !=
        static_cast<u64>(array_len) * sizeof(OrbisNpTusDataStatusForCrossSave)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", status_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiSlotDataStatus(
        uid, static_cast<s32>(svc), std::string(), std::string(),
        static_cast<s64>(target_account_id), slots, nullptr, nullptr, static_cast<u64>(array_len),
        ctx, status_array);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiSlotDataStatusForCrossSaveVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32* slot_ids,
    OrbisNpTusDataStatusForCrossSave* status_array, u64 status_array_size, s32 array_len,
    void* option) {
    auto ret = sceNpTusGetMultiSlotDataStatusForCrossSaveVUserAsync(
        req_id, target_virtual_user_id, slot_ids, status_array, status_array_size, array_len,
        option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiSlotDataStatusForCrossSaveVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32* slot_ids,
    OrbisNpTusDataStatusForCrossSave* status_array, u64 status_array_size, s32 array_len,
    void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_virtual_user_id = {}, slot_ids = {}, status_array = {}, "
             "status_array_size = {}, array_len = {}, option = {}",
             req_id, target_virtual_user_id ? target_virtual_user_id->data : "", fmt::ptr(slot_ids),
             fmt::ptr(status_array), status_array_size, array_len, fmt::ptr(option));
    if (!target_virtual_user_id || !slot_ids || !status_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || array_len < 1) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (status_array_size !=
        static_cast<u64>(array_len) * sizeof(OrbisNpTusDataStatusForCrossSave)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", status_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtual_user(
        target_virtual_user_id->data,
        strnlen(target_virtual_user_id->data, sizeof(target_virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiSlotDataStatus(
        uid, static_cast<s32>(svc), std::string(), virtual_user, 0, slots, nullptr, nullptr,
        static_cast<u64>(array_len), ctx, status_array);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiSlotVariableForCrossSave(s32 req_id, OrbisNpAccountId target_account_id,
                                             s32* slot_ids,
                                             OrbisNpTusVariableForCrossSave* variable_array,
                                             u64 variables_size, s32 array_len, void* option) {
    auto ret = sceNpTusGetMultiSlotVariableForCrossSaveAsync(
        req_id, target_account_id, slot_ids, variable_array, variables_size, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiSlotVariableForCrossSaveAsync(s32 req_id, OrbisNpAccountId target_account_id,
                                                  s32* slot_ids,
                                                  OrbisNpTusVariableForCrossSave* variable_array,
                                                  u64 variables_size, s32 array_len, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_account_id = {}, slot_ids = {}, variable_array = {}, "
             "variables_size = {}, array_len = {}, option = {}",
             req_id, target_account_id, fmt::ptr(slot_ids), fmt::ptr(variable_array),
             variables_size, array_len, fmt::ptr(option));
    if (!slot_ids || !variable_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (array_len < 1 || option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid slot id");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (variables_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusVariableForCrossSave)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", variables_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    const s32 n = array_len;
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiSlotVariable(
        uid, static_cast<s32>(svc), std::string(), std::string(), slots, nullptr,
        static_cast<u64>(n), ctx, nullptr, static_cast<s64>(target_account_id), variable_array);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiSlotVariableForCrossSaveVUser(s32 req_id,
                                                  const OrbisNpTusVirtualUserId* virtual_user_id,
                                                  s32* slot_ids,
                                                  OrbisNpTusVariableForCrossSave* variable_array,
                                                  u64 variables_size, s32 array_len, void* option) {
    auto ret = sceNpTusGetMultiSlotVariableForCrossSaveVUserAsync(
        req_id, virtual_user_id, slot_ids, variable_array, variables_size, array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiSlotVariableForCrossSaveVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* virtual_user_id, s32* slot_ids,
    OrbisNpTusVariableForCrossSave* variable_array, u64 variables_size, s32 array_len,
    void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, virtual_user_id = {}, slot_ids = {}, variable_array = {}, "
             "variables_size = {}, array_len = {}, option = {}",
             req_id, virtual_user_id ? virtual_user_id->data : "", fmt::ptr(slot_ids),
             fmt::ptr(variable_array), variables_size, array_len, fmt::ptr(option));
    if (!virtual_user_id || !slot_ids || !variable_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (array_len < 1 || option) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxSlotsPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many slots {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_SLOTID;
    }
    if (std::any_of(slot_ids, slot_ids + array_len, [](s32 id) { return id < 0; })) {
        LOG_ERROR(Lib_NpTus, "invalid slot id");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (variables_size != static_cast<u64>(array_len) * sizeof(OrbisNpTusVariableForCrossSave)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", variables_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtual_user(virtual_user_id->data,
                                   strnlen(virtual_user_id->data, sizeof(virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid virtual user");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s32> slots(slot_ids, slot_ids + array_len);
    const s32 n = array_len;
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiSlotVariable(
        uid, static_cast<s32>(svc), std::string(), virtual_user, slots, nullptr,
        static_cast<u64>(n), ctx, nullptr, 0, variable_array);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiUserDataStatusForCrossSave(s32 req_id,
                                               const OrbisNpAccountId* target_account_id_array,
                                               s32 slot_id,
                                               OrbisNpTusDataStatusForCrossSave* status_array,
                                               u64 status_array_size, s32 array_len, void* option) {
    auto ret = sceNpTusGetMultiUserDataStatusForCrossSaveAsync(
        req_id, target_account_id_array, slot_id, status_array, status_array_size, array_len,
        option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiUserDataStatusForCrossSaveAsync(s32 req_id,
                                                    const OrbisNpAccountId* target_account_id_array,
                                                    s32 slot_id,
                                                    OrbisNpTusDataStatusForCrossSave* status_array,
                                                    u64 status_array_size, s32 array_len,
                                                    void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_account_id_array = {}, slot_id = {}, status_array = {}, "
             "status_array_size = {}, array_len = {}, option = {}",
             req_id, fmt::ptr(target_account_id_array), slot_id, fmt::ptr(status_array),
             status_array_size, array_len, fmt::ptr(option));
    if (!target_account_id_array || !status_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || array_len < 1 || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (status_array_size !=
        static_cast<u64>(array_len) * sizeof(OrbisNpTusDataStatusForCrossSave)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", status_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s64> accounts;
    accounts.reserve(array_len);
    for (s32 i = 0; i < array_len; ++i) {
        accounts.push_back(static_cast<s64>(target_account_id_array[i]));
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiUserDataStatus(
        uid, static_cast<s32>(svc), slot_id, {}, {}, accounts, nullptr, nullptr,
        static_cast<u64>(array_len), ctx, status_array);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiUserDataStatusForCrossSaveVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id_array, s32 slot_id,
    OrbisNpTusDataStatusForCrossSave* status_array, u64 status_array_size, s32 array_len,
    void* option) {
    auto ret = sceNpTusGetMultiUserDataStatusForCrossSaveVUserAsync(
        req_id, target_virtual_user_id_array, slot_id, status_array, status_array_size, array_len,
        option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiUserDataStatusForCrossSaveVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id_array, s32 slot_id,
    OrbisNpTusDataStatusForCrossSave* status_array, u64 status_array_size, s32 array_len,
    void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_virtual_user_id_array = {}, slot_id = {}, status_array = {}, "
             "status_array_size = {}, array_len = {}, option = {}",
             req_id, fmt::ptr(target_virtual_user_id_array), slot_id, fmt::ptr(status_array),
             status_array_size, array_len, fmt::ptr(option));
    if (!target_virtual_user_id_array || !status_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || array_len < 1 || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (status_array_size !=
        static_cast<u64>(array_len) * sizeof(OrbisNpTusDataStatusForCrossSave)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", status_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    std::vector<std::string> vusers;
    vusers.reserve(array_len);
    for (s32 i = 0; i < array_len; ++i) {
        const char* d = target_virtual_user_id_array[i].data;
        std::string name(d, strnlen(d, sizeof(target_virtual_user_id_array[i].data)));
        if (name.empty()) {
            LOG_ERROR(Lib_NpTus, "invalid virtual user {}", i);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
        }
        vusers.emplace_back(std::move(name));
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiUserDataStatus(
        uid, static_cast<s32>(svc), slot_id, {}, vusers, {}, nullptr, nullptr,
        static_cast<u64>(array_len), ctx, status_array);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiUserVariableForCrossSave(s32 req_id,
                                             const OrbisNpAccountId* target_account_id_array,
                                             s32 slot_id,
                                             OrbisNpTusVariableForCrossSave* variable_array,
                                             u64 variable_array_size, s32 array_len, void* option) {
    auto ret = sceNpTusGetMultiUserVariableForCrossSaveAsync(
        req_id, target_account_id_array, slot_id, variable_array, variable_array_size, array_len,
        option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiUserVariableForCrossSaveAsync(s32 req_id,
                                                  const OrbisNpAccountId* target_account_id_array,
                                                  s32 slot_id,
                                                  OrbisNpTusVariableForCrossSave* variable_array,
                                                  u64 variable_array_size, s32 array_len,
                                                  void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_account_id_array = {}, slot_id = {}, variable_array = {}, "
             "variable_array_size = {}, array_len = {}, option = {}",
             req_id, fmt::ptr(target_account_id_array), slot_id, fmt::ptr(variable_array),
             variable_array_size, array_len, fmt::ptr(option));
    if (!target_account_id_array || !variable_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || array_len < 1 || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (variable_array_size !=
        static_cast<u64>(array_len) * sizeof(OrbisNpTusVariableForCrossSave)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", variable_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    std::vector<s64> accounts;
    accounts.reserve(array_len);
    for (s32 i = 0; i < array_len; ++i) {
        accounts.push_back(static_cast<s64>(target_account_id_array[i]));
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiUserVariable(
        uid, static_cast<s32>(svc), slot_id, {}, {}, accounts, nullptr, nullptr,
        static_cast<u64>(array_len), ctx, variable_array);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusGetMultiUserVariableForCrossSaveVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id_array, s32 slot_id,
    OrbisNpTusVariableForCrossSave* variable_array, u64 variable_array_size, s32 array_len,
    void* option) {
    auto ret = sceNpTusGetMultiUserVariableForCrossSaveVUserAsync(
        req_id, target_virtual_user_id_array, slot_id, variable_array, variable_array_size,
        array_len, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusGetMultiUserVariableForCrossSaveVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id_array, s32 slot_id,
    OrbisNpTusVariableForCrossSave* variable_array, u64 variable_array_size, s32 array_len,
    void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_virtual_user_id_array = {}, slot_id = {}, variable_array = {}, "
             "variable_array_size = {}, array_len = {}, option = {}",
             req_id, fmt::ptr(target_virtual_user_id_array), slot_id, fmt::ptr(variable_array),
             variable_array_size, array_len, fmt::ptr(option));
    if (!target_virtual_user_id_array || !variable_array) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || array_len < 1 || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (array_len > TusMaxUsersPerRequest) {
        LOG_ERROR(Lib_NpTus, "too many users {}", array_len);
        return ORBIS_NP_COMMUNITY_ERROR_TOO_MANY_NPID;
    }
    if (variable_array_size !=
        static_cast<u64>(array_len) * sizeof(OrbisNpTusVariableForCrossSave)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", variable_array_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    std::vector<std::string> vusers;
    vusers.reserve(array_len);
    for (s32 i = 0; i < array_len; ++i) {
        const char* d = target_virtual_user_id_array[i].data;
        std::string name(d, strnlen(d, sizeof(target_virtual_user_id_array[i].data)));
        if (name.empty()) {
            LOG_ERROR(Lib_NpTus, "invalid virtual user {}", i);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
        }
        vusers.emplace_back(std::move(name));
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusGetMultiUserVariable(
        uid, static_cast<s32>(svc), slot_id, {}, vusers, {}, nullptr, nullptr,
        static_cast<u64>(array_len), ctx, variable_array);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusTryAndSetVariableForCrossSave(s32 req_id, OrbisNpAccountId target_account_id,
                                          s32 slot_id, s32 ope_type, s64 variable,
                                          const s64* compare_value,
                                          const OrbisNpAccountId* is_last_changed_author,
                                          const Rtc::OrbisRtcTick* is_last_changed_date,
                                          OrbisNpTusVariableForCrossSave* result_variable,
                                          u64 result_variable_size, void* option) {
    auto ret = sceNpTusTryAndSetVariableForCrossSaveAsync(
        req_id, target_account_id, slot_id, ope_type, variable, compare_value,
        is_last_changed_author, is_last_changed_date, result_variable, result_variable_size,
        option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusTryAndSetVariableForCrossSaveAsync(s32 req_id, OrbisNpAccountId target_account_id,
                                               s32 slot_id, s32 ope_type, s64 variable,
                                               const s64* compare_value,
                                               const OrbisNpAccountId* is_last_changed_author,
                                               const Rtc::OrbisRtcTick* is_last_changed_date,
                                               OrbisNpTusVariableForCrossSave* result_variable,
                                               u64 result_variable_size, void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {}, target_account_id = {}, slot_id = {}, ope_type = {}, variable = {}, "
             "compare_value = {}, result_variable = {}, result_variable_size = {}, option = {}",
             req_id, target_account_id, slot_id, ope_type, variable, fmt::ptr(compare_value),
             fmt::ptr(result_variable), result_variable_size, fmt::ptr(option));
    if (!result_variable) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (ope_type < ORBIS_NP_TUS_OPETYPE_EQUAL || ope_type > ORBIS_NP_TUS_OPETYPE_LESS_OR_EQUAL) {
        LOG_ERROR(Lib_NpTus, "invalid opeType {}", ope_type);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_TYPE;
    }
    if (result_variable_size != sizeof(OrbisNpTusVariableForCrossSave)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", result_variable_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const bool has_author_check = is_last_changed_author != nullptr;
    const bool has_date_check = is_last_changed_date != nullptr;

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusTryAndSetVariable(
        uid, static_cast<s32>(svc), std::string(), std::string(),
        static_cast<s64>(target_account_id), slot_id, ope_type, variable, compare_value != nullptr,
        compare_value ? *compare_value : variable, has_author_check,
        has_author_check ? static_cast<s64>(*is_last_changed_author) : 0, std::string(),
        has_date_check, has_date_check ? is_last_changed_date->tick : 0, nullptr, nullptr, ctx,
        result_variable);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTusTryAndSetVariableForCrossSaveVUser(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s32 ope_type,
    s64 variable, const s64* compare_value, const OrbisNpAccountId* is_last_changed_author,
    const Rtc::OrbisRtcTick* is_last_changed_date, OrbisNpTusVariableForCrossSave* result_variable,
    u64 result_variable_size, void* option) {
    auto ret = sceNpTusTryAndSetVariableForCrossSaveVUserAsync(
        req_id, target_virtual_user_id, slot_id, ope_type, variable, compare_value,
        is_last_changed_author, is_last_changed_date, result_variable, result_variable_size,
        option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTusTryAndSetVariableForCrossSaveVUserAsync(
    s32 req_id, const OrbisNpTusVirtualUserId* target_virtual_user_id, s32 slot_id, s32 ope_type,
    s64 variable, const s64* compare_value, const OrbisNpAccountId* is_last_changed_author,
    const Rtc::OrbisRtcTick* is_last_changed_date, OrbisNpTusVariableForCrossSave* result_variable,
    u64 result_variable_size, void* option) {
    LOG_INFO(
        Lib_NpTus,
        "req_id = {}, target_virtual_user_id = {}, slot_id = {}, ope_type = {}, variable = {}, "
        "compare_value = {}, result_variable = {}, result_variable_size = {}, option = {}",
        req_id, target_virtual_user_id ? target_virtual_user_id->data : "", slot_id, ope_type,
        variable, fmt::ptr(compare_value), fmt::ptr(result_variable), result_variable_size,
        fmt::ptr(option));
    if (!target_virtual_user_id || !result_variable) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (option || slot_id < 0) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (ope_type < ORBIS_NP_TUS_OPETYPE_EQUAL || ope_type > ORBIS_NP_TUS_OPETYPE_LESS_OR_EQUAL) {
        LOG_ERROR(Lib_NpTus, "invalid opeType {}", ope_type);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_TYPE;
    }
    if (result_variable_size != sizeof(OrbisNpTusVariableForCrossSave)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", result_variable_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    const std::string virtual_user(
        target_virtual_user_id->data,
        strnlen(target_virtual_user_id->data, sizeof(target_virtual_user_id->data)));
    if (virtual_user.empty()) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }
    const bool has_author_check = is_last_changed_author != nullptr;
    const bool has_date_check = is_last_changed_date != nullptr;

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TusTryAndSetVariable(
        uid, static_cast<s32>(svc), std::string(), virtual_user, 0, slot_id, ope_type, variable,
        compare_value != nullptr, compare_value ? *compare_value : variable, has_author_check,
        has_author_check ? static_cast<s64>(*is_last_changed_author) : 0, std::string(),
        has_date_check, has_date_check ? is_last_changed_date->tick : 0, nullptr, nullptr, ctx,
        result_variable);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTssGetDataAsync(s32 req_id, s32 slot_id, OrbisNpTssDataStatus* data_status,
                         u64 data_status_size, void* data, u64 recv_size,
                         OrbisNpTssGetDataOptParam* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {:#x}, slot_id = {}, data_status = {}, data_status_size = {}, data = {}, "
             "recv_size = {}, option = {}",
             req_id, slot_id, fmt::ptr(data_status), data_status_size, fmt::ptr(data), recv_size,
             fmt::ptr(option));
    if (!data_status) {
        LOG_ERROR(Lib_NpTus, "invalid argument");
        return ORBIS_NP_COMMUNITY_ERROR_INSUFFICIENT_ARGUMENT;
    }
    if (data_status_size != sizeof(OrbisNpTssDataStatus)) {
        LOG_ERROR(Lib_NpTus, "invalid size {}", data_status_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
    }
    if (slot_id < 0 || slot_id > TssMaxSlotId) {
        LOG_ERROR(Lib_NpTus, "invalid slot id {}", slot_id);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    bool has_offset = false, has_last_byte = false, has_if_param = false;
    u64 offset = 0, last_byte = 0, if_last_modified = 0;
    s32 if_type = 0;
    if (option) {
        if (option->size != sizeof(OrbisNpTssGetDataOptParam)) {
            LOG_ERROR(Lib_NpTus, "invalid option size {}", option->size);
            return ORBIS_NP_COMMUNITY_ERROR_INVALID_ALIGNMENT;
        }
        if (option->offset) {
            has_offset = true;
            offset = *option->offset;
        }
        if (option->last_byte) {
            has_last_byte = true;
            last_byte = *option->last_byte;
        }
        if (option->if_param) {
            has_if_param = true;
            if_type = static_cast<s32>(option->if_param->if_type);
            if_last_modified = option->if_param->last_modified.tick;
        }
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    u64 start_offset = offset;
    {
        std::lock_guard lock(g_mutex);
        if (!req->xfer.SameTarget(slot_id, 0, std::string())) {
            req->xfer.Begin(slot_id, 0, std::string());
        } else if (!has_offset) {
            has_offset = true;
            start_offset = req->xfer.recv_offset;
        }
        if (data) {
            req->xfer.recv_offset += recv_size;
        }
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TssGetData(
        uid, static_cast<s32>(svc), slot_id, has_offset, start_offset, has_last_byte, last_byte,
        has_if_param, if_type, if_last_modified, data_status, data, data ? recv_size : 0, ctx);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTssGetData(s32 req_id, s32 slot_id, OrbisNpTssDataStatus* data_status,
                    u64 data_status_size, void* data, u64 recv_size,
                    OrbisNpTssGetDataOptParam* option) {
    auto ret = sceNpTssGetDataAsync(req_id, slot_id, data_status, data_status_size, data, recv_size,
                                    option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTssGetSmallStorageAsync(s32 req_id, void* data, u64 max_size, u64* content_length,
                                 void* option) {
    LOG_INFO(Lib_NpTus,
             "req_id = {:#x}, data = {}, max_size = {}, content_length = {}, option = {}", req_id,
             fmt::ptr(data), max_size, fmt::ptr(content_length), fmt::ptr(option));
    if (option) {
        LOG_ERROR(Lib_NpTus, "invalid option");
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }
    if (max_size > TssMaxSize) {
        LOG_ERROR(Lib_NpTus, "invalid maxSize {}", max_size);
        return ORBIS_NP_COMMUNITY_ERROR_INVALID_ARGUMENT;
    }

    NpTusRequest* req = nullptr;
    u32 svc = 0;
    s32 uid = -1;
    std::string self;
    if (auto ret = ResolveTus(req_id, &req, &svc, &uid, &self); ret < 0) {
        return ret;
    }

    auto ctx = std::make_shared<TusRequestCtx>();
    req->ctx = ctx;
    s32 submit = NpHandler::Instance().TssGetData(uid, static_cast<s32>(svc), 0, false, 0, false, 0,
                                                  false, 0, 0, nullptr, data, data ? max_size : 0,
                                                  ctx, content_length);
    if (submit < 0) {
        return submit;
    }
    return ORBIS_OK;
}

s32 sceNpTssGetSmallStorage(s32 req_id, void* data, u64 max_size, u64* content_length,
                            void* option) {
    auto ret = sceNpTssGetSmallStorageAsync(req_id, data, max_size, content_length, option);
    if (ret < 0) {
        return ret;
    }
    if (auto wait = sceNpTusWaitAsync(req_id, &ret); wait < 0) {
        return wait;
    }
    return ret;
}

s32 sceNpTssGetStorageAsync(s32 req_id) {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called, faking async completion");
    return FakeAsyncComplete(req_id);
}

s32 sceNpTssGetStorage() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpTusChangeModeForOtherSaveDataOwners() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpTusCreateTitleCtx() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpTusSetThreadParam() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

s32 sceNpTusSetTimeout() {
    LOG_ERROR(Lib_NpTus, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterHooks() {
    return RegisterNpTusHooks();
}

} // namespace Libraries::Np::NpTus
