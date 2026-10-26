// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

enum OrbisHttpMethod : s32 {
    ORBIS_HTTP_METHOD_GET = 0,
    ORBIS_HTTP_METHOD_POST = 1,
    ORBIS_HTTP_METHOD_HEAD = 2,
    ORBIS_HTTP_METHOD_OPTIONS = 3,
    ORBIS_HTTP_METHOD_PUT = 4,
    ORBIS_HTTP_METHOD_DELETE = 5,
    ORBIS_HTTP_METHOD_TRACE = 6,
    ORBIS_HTTP_METHOD_CONNECT = 7,
    ORBIS_HTTP_METHOD_CUSTOM = 8,
};

namespace Libraries::Network::Http {

s32 sceHttpCreateConnection(s32 tmplId, const char* serverName, const char* scheme, u16 port,
                            s32 isEnableKeepalive);
s32 sceHttpCreateConnectionWithURL(s32 tmplId, const char* url, bool enableKeepalive);
s32 sceHttpCreateRequest(s32 connId, s32 method, const char* path, u64 contentLength);
s32 sceHttpCreateRequest2(s32 connId, const char* method, const char* path, u64 contentLength);
s32 sceHttpCreateRequestWithURL(s32 connId, s32 method, const char* url, u64 contentLength);
s32 sceHttpCreateRequestWithURL2(s32 connId, const char* method, const char* url,
                                 u64 contentLength);
s32 sceHttpCreateTemplate(s32 libhttpCtxId, const char* userAgent, s32 httpVer,
                          s32 isAutoProxyConf);
s32 sceHttpDeleteConnection(s32 connId);
s32 sceHttpDeleteRequest(s32 reqId);
s32 sceHttpGetStatusCode(s32 reqId, s32* statusCode);

void RegisterHooks();
}; // namespace Libraries::Network::Http