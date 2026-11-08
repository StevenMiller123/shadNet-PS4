// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <orbis/NpUtility.h>

#include "common/shadnet_hook.h"
#include "core/libraries/np/np_utility/np_utility.h"

extern "C" {
void sceNpBandwidthTestDownloadOnlyInitStart();
void sceNpBandwidthTestInitStartDownload();
void sceNpBandwidthTestInitStartUpload();
void sceNpBandwidthTestShutdownWithDetailedInfo();
void sceNpBandwidthTestUploadOnlyInitStart();
void sceNpLookupNetAbortRequest();
void sceNpLookupNetCensorComment();
void sceNpLookupNetConvertJidToNpId();
void sceNpLookupNetConvertNpIdToJid();
void sceNpLookupNetCreateRequest();
void sceNpLookupNetCreateTitleCtx();
void sceNpLookupNetDeleteRequest();
void sceNpLookupNetDeleteTitleCtx();
void sceNpLookupNetInit();
void sceNpLookupNetInitWithFunctionPointer();
void sceNpLookupNetInitWithMemoryPool();
void sceNpLookupNetIsInit();
void sceNpLookupNetNpId();
void sceNpLookupNetSanitizeComment();
void sceNpLookupNetSetTimeout();
void sceNpLookupNetTerm();
}

SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpBandwidthTestAbort);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpBandwidthTestDownloadOnlyInitStart);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpBandwidthTestGetStatus);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpBandwidthTestInitStart);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpBandwidthTestInitStartDownload);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpBandwidthTestInitStartUpload);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpBandwidthTestShutdown);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpBandwidthTestShutdownWithDetailedInfo);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpBandwidthTestUploadOnlyInitStart);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupAbortRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupCreateAsyncRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupCreateRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupCreateTitleCtx);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupCreateTitleCtxA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupDeleteRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupDeleteTitleCtx);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupNetAbortRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupNetCensorComment);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupNetConvertJidToNpId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupNetConvertNpIdToJid);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupNetCreateRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupNetCreateTitleCtx);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupNetDeleteRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupNetDeleteTitleCtx);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupNetInit);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupNetInitWithFunctionPointer);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupNetInitWithMemoryPool);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupNetIsInit);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupNetNpId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupNetSanitizeComment);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupNetSetTimeout);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupNetTerm);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupNpId);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupPollAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupSetTimeout);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpLookupWaitAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpUtilityInit);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpUtilityTerm);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterAbortRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterCensorComment);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterCreateAsyncRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterCreateRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterCreateTitleCtx);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterCreateTitleCtxA);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterDeleteRequest);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterDeleteTitleCtx);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterPollAsync);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterSanitizeComment);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterSetTimeout);
SHADNET_HOOK_DECLARE(Libraries::Np::NpUtility, sceNpWordFilterWaitAsync);

static void RegisterNpUtilityHooks() {
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpBandwidthTestAbort);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpBandwidthTestDownloadOnlyInitStart);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpBandwidthTestGetStatus);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpBandwidthTestInitStart);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpBandwidthTestInitStartDownload);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpBandwidthTestInitStartUpload);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpBandwidthTestShutdown);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpBandwidthTestShutdownWithDetailedInfo);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpBandwidthTestUploadOnlyInitStart);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupAbortRequest);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupCreateAsyncRequest);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupCreateRequest);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupCreateTitleCtx);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupCreateTitleCtxA);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupDeleteRequest);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupDeleteTitleCtx);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupNetAbortRequest);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupNetCensorComment);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupNetConvertJidToNpId);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupNetConvertNpIdToJid);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupNetCreateRequest);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupNetCreateTitleCtx);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupNetDeleteRequest);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupNetDeleteTitleCtx);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupNetInit);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupNetInitWithFunctionPointer);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupNetInitWithMemoryPool);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupNetIsInit);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupNetNpId);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupNetSanitizeComment);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupNetSetTimeout);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupNetTerm);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupNpId);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupPollAsync);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupSetTimeout);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpLookupWaitAsync);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpUtilityInit);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpUtilityTerm);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterAbortRequest);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterCensorComment);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterCreateAsyncRequest);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterCreateRequest);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterCreateTitleCtx);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterCreateTitleCtxA);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterDeleteRequest);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterDeleteTitleCtx);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterPollAsync);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterSanitizeComment);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterSetTimeout);
    SHADNET_HOOK(Libraries::Np::NpUtility, sceNpWordFilterWaitAsync);
}

namespace Libraries::Np::NpUtility {

s32 sceNpBandwidthTestAbort() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpBandwidthTestDownloadOnlyInitStart() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpBandwidthTestGetStatus() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpBandwidthTestInitStart() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpBandwidthTestInitStartDownload() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpBandwidthTestInitStartUpload() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpBandwidthTestShutdown() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpBandwidthTestShutdownWithDetailedInfo() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpBandwidthTestUploadOnlyInitStart() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpLookupNetAbortRequest() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpLookupNetCensorComment() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpLookupNetConvertJidToNpId() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpLookupNetConvertNpIdToJid() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpLookupNetCreateRequest() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpLookupNetCreateTitleCtx() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpLookupNetDeleteRequest() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpLookupNetDeleteTitleCtx() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpLookupNetInit() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpLookupNetInitWithFunctionPointer() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpLookupNetInitWithMemoryPool() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpLookupNetIsInit() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpLookupNetNpId() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpLookupNetSanitizeComment() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpLookupNetSetTimeout() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpLookupNetTerm() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpUtilityInit() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

s32 sceNpUtilityTerm() {
    LOG_ERROR(Lib_NpUtility, "(STUBBED) called");
    return 0;
}

void RegisterHooks() {
    return RegisterNpUtilityHooks();
}

} // namespace Libraries::Np::NpUtility
