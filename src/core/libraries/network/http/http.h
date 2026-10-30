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

s32 sceHttpCreateConnection(s32 tmpl_id, const char* server_name, const char* scheme, u16 port,
                            s32 is_enable_keepalive);
s32 sceHttpCreateConnectionWithURL(s32 tmpl_id, const char* url, bool enable_keepalive);
s32 sceHttpCreateRequest(s32 conn_id, s32 method, const char* path, u64 content_length);
s32 sceHttpCreateRequest2(s32 conn_id, const char* method, const char* path, u64 content_length);
s32 sceHttpCreateRequestWithURL(s32 conn_id, s32 method, const char* url, u64 content_length);
s32 sceHttpCreateRequestWithURL2(s32 conn_id, const char* method, const char* url,
                                 u64 content_length);
s32 sceHttpCreateTemplate(s32 libhttp_ctx_id, const char* user_agent, s32 http_ver,
                          s32 is_auto_proxy_conf);
s32 sceHttpDeleteConnection(s32 conn_id);
s32 sceHttpDeleteRequest(s32 req_id);
s32 sceHttpGetStatusCode(s32 req_id, s32* status_code);

void RegisterHooks();
}; // namespace Libraries::Network::Http
