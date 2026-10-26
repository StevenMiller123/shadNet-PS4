// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <mutex>
#include <magic_enum/magic_enum.hpp>
#include <orbis/Http.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "common/types.h"
#include "core/libraries/network/http2/http2.h"

extern "C" {
void sceHttp2CreateRequestWithURL();
void sceHttp2CreateTemplate();
void sceHttp2DeleteRequest();
void sceHttp2GetStatusCode();
}

SHADNET_HOOK_DECLARE(Libraries::Network::Http2, sceHttp2CreateRequestWithURL);
SHADNET_HOOK_DECLARE(Libraries::Network::Http2, sceHttp2CreateTemplate);
SHADNET_HOOK_DECLARE(Libraries::Network::Http2, sceHttp2DeleteRequest);
SHADNET_HOOK_DECLARE(Libraries::Network::Http2, sceHttp2GetStatusCode);

static void RegisterHttp2Hooks() {
    SHADNET_HOOK(Libraries::Network::Http2, sceHttp2CreateRequestWithURL);
    SHADNET_HOOK(Libraries::Network::Http2, sceHttp2CreateTemplate);
    SHADNET_HOOK(Libraries::Network::Http2, sceHttp2DeleteRequest);
    SHADNET_HOOK(Libraries::Network::Http2, sceHttp2GetStatusCode);
}

namespace Libraries::Network::Http2 {

static std::mutex urls_mutex{};
static std::map<s32, std::string> request_urls{};

s32 sceHttp2CreateRequestWithURL(s32 tmpl_id, const char* method, const char* url,
                                 u64 content_length) {
    LOG_INFO(Lib_Http2, "called, method={}, url={}, content_length={}", method ? method : "(null)",
             url ? url : "(null)", content_length);
    s32 req_id =
        SHADNET_HOOK_CONTINUE(sceHttp2CreateRequestWithURL, tmpl_id, method, url, content_length);
    if (req_id > 0) {
        std::scoped_lock lk{urls_mutex};
        request_urls[req_id] = url;
    }
    return req_id;
}

s32 sceHttp2CreateTemplate(s32 ctx_id, const char* user_agent, s32 http_ver, s32 auto_proxy_conf) {
    OrbisHttp2HttpVersion enum_ver = OrbisHttp2HttpVersion(http_ver);
    LOG_INFO(Lib_Http2, "called, user_agent={}, http_ver={}", user_agent,
             magic_enum::enum_name(enum_ver));
    return SHADNET_HOOK_CONTINUE(sceHttp2CreateTemplate, ctx_id, user_agent, http_ver,
                                 auto_proxy_conf);
}

s32 sceHttp2DeleteRequest(s32 req_id) {
    s32 ret = SHADNET_HOOK_CONTINUE(sceHttp2DeleteRequest, req_id);
    if (ret == 0) {
        std::scoped_lock lk{urls_mutex};
        if (request_urls.contains(req_id)) {
            request_urls.erase(req_id);
        }
    }
    return ret;
}

s32 sceHttp2GetStatusCode(s32 req_id, s32* status_code) {
    s32 ret = SHADNET_HOOK_CONTINUE(sceHttp2GetStatusCode, req_id, status_code);
    if (ret == 0) {
        std::scoped_lock lk{urls_mutex};
        if (request_urls.contains(req_id)) {
            LOG_INFO(Lib_Http2, "called, url={}, status_code={}", request_urls[req_id],
                     *status_code);
        }
    }
    return ret;
}

void RegisterHooks() {
    return RegisterHttp2Hooks();
}

} // namespace Libraries::Network::Http2