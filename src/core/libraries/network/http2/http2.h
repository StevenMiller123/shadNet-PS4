// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Libraries::Network::Http2 {

enum OrbisHttp2HttpVersion : u32 {
    ORBIS_HTTP2_VERSION_1_0 = 1,
    ORBIS_HTTP2_VERSION_1_1 = 2,
    ORBIS_HTTP2_VERSION_2_0 = 3,
};

s32 sceHttp2CreateRequestWithURL(s32 tmpl_id, const char* method, const char* url,
                                 u64 content_length);
s32 sceHttp2CreateTemplate(s32 ctx_id, const char* user_agent, s32 http_ver, s32 auto_proxy_conf);
s32 sceHttp2DeleteRequest(s32 req_id);
s32 sceHttp2GetStatusCode(s32 req_id, s32* status_code);

void RegisterHooks();
}; // namespace Libraries::Network::Http2