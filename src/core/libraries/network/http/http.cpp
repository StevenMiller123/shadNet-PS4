// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <mutex>
#include <magic_enum/magic_enum.hpp>
#include <orbis/Http.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "common/types.h"
#include "core/libraries/network/http/http.h"

SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpCreateConnection);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpCreateConnectionWithURL);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpCreateRequest);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpCreateRequest2);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpCreateRequestWithURL);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpCreateRequestWithURL2);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpCreateTemplate);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpDeleteConnection);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpDeleteRequest);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpGetStatusCode);

static void RegisterHttpHooks() {
    SHADNET_HOOK(Libraries::Network::Http, sceHttpCreateConnection);
    SHADNET_HOOK(Libraries::Network::Http, sceHttpCreateConnectionWithURL);
    SHADNET_HOOK(Libraries::Network::Http, sceHttpCreateRequest);
    SHADNET_HOOK(Libraries::Network::Http, sceHttpCreateRequest2);
    SHADNET_HOOK(Libraries::Network::Http, sceHttpCreateRequestWithURL);
    SHADNET_HOOK(Libraries::Network::Http, sceHttpCreateRequestWithURL2);
    SHADNET_HOOK(Libraries::Network::Http, sceHttpCreateTemplate);
    SHADNET_HOOK(Libraries::Network::Http, sceHttpDeleteConnection);
    SHADNET_HOOK(Libraries::Network::Http, sceHttpDeleteRequest);
    SHADNET_HOOK(Libraries::Network::Http, sceHttpGetStatusCode);
}

namespace Libraries::Network::Http {

static std::mutex urls_mutex{};
static std::map<s32, std::string> request_urls{};
static std::map<s32, std::string> connection_urls{};
static std::map<s32, std::string> connection_servers{};

s32 sceHttpCreateConnection(s32 tmpl_id, const char* server_name, const char* scheme, u16 port,
                            s32 is_enable_keepalive) {
    LOG_INFO(Lib_Http, "called, server_name={}, scheme={}, port={}, is_enable_keepalive={}",
             server_name ? server_name : "(null)", scheme ? scheme : "(null)", port,
             is_enable_keepalive);
    s32 conn_id = SHADNET_HOOK_CONTINUE(sceHttpCreateConnection, tmpl_id, server_name, scheme, port,
                                        is_enable_keepalive);
    if (conn_id > 0) {
        std::scoped_lock lk{urls_mutex};
        connection_servers[conn_id] = server_name;
    }
    return conn_id;
}

s32 sceHttpCreateConnectionWithURL(s32 tmpl_id, const char* url, bool enable_keepalive) {
    LOG_INFO(Lib_Http, "called, url={}, enable_keepalive={}", url ? url : "(null)",
             enable_keepalive);
    s32 conn_id =
        SHADNET_HOOK_CONTINUE(sceHttpCreateConnectionWithURL, tmpl_id, url, enable_keepalive);
    if (conn_id > 0) {
        std::scoped_lock lk{urls_mutex};
        connection_urls[conn_id] = url;
    }
    return conn_id;
}

s32 sceHttpCreateRequest(s32 conn_id, s32 method, const char* path, u64 content_length) {
    OrbisHttpMethod enum_method = OrbisHttpMethod(method);
    LOG_INFO(Lib_Http, "called, method={}, path={}, content_length={}",
             magic_enum::enum_name(enum_method), path ? path : "(null)", content_length);
    s32 req_id = SHADNET_HOOK_CONTINUE(sceHttpCreateRequest, conn_id, method, path, content_length);
    if (req_id > 0) {
        std::scoped_lock lk{urls_mutex};
        if (connection_servers.contains(conn_id)) {
            request_urls[req_id] = connection_servers[conn_id] + path;
        }
    }
    return req_id;
}

s32 sceHttpCreateRequest2(s32 conn_id, const char* method, const char* path, u64 content_length) {
    LOG_INFO(Lib_Http, "called, method={}, path={}, content_length={}", method ? method : "(null)",
             path ? path : "(null)", content_length);
    s32 req_id =
        SHADNET_HOOK_CONTINUE(sceHttpCreateRequest2, conn_id, method, path, content_length);
    if (req_id > 0) {
        std::scoped_lock lk{urls_mutex};
        if (connection_servers.contains(conn_id)) {
            request_urls[req_id] = connection_servers[conn_id] + path;
        }
    }
    return req_id;
}

s32 sceHttpCreateRequestWithURL(s32 conn_id, s32 method, const char* url, u64 content_length) {
    OrbisHttpMethod enum_method = OrbisHttpMethod(method);
    LOG_INFO(Lib_Http, "called, method={}, url={}, content_length={}",
             magic_enum::enum_name(enum_method), url ? url : "(null)", content_length);
    s32 req_id =
        SHADNET_HOOK_CONTINUE(sceHttpCreateRequestWithURL, conn_id, method, url, content_length);
    if (req_id > 0) {
        std::scoped_lock lk{urls_mutex};
        request_urls[req_id] = url;
    }
    return req_id;
}

s32 sceHttpCreateRequestWithURL2(s32 conn_id, const char* method, const char* url,
                                 u64 content_length) {
    LOG_INFO(Lib_Http, "called, method={}, url={}, content_length={}", method ? method : "(null)",
             url ? url : "(null)", content_length);
    s32 req_id =
        SHADNET_HOOK_CONTINUE(sceHttpCreateRequestWithURL2, conn_id, method, url, content_length);
    if (req_id > 0) {
        std::scoped_lock lk{urls_mutex};
        request_urls[req_id] = url;
    }
    return req_id;
}

s32 sceHttpCreateTemplate(s32 libhttp_ctx_id, const char* user_agent, s32 http_ver,
                          s32 is_auto_proxy_conf) {
    LOG_INFO(Lib_Http, "called, user_agent={}, http_ver={}, is_auto_proxy_conf={}",
             user_agent ? user_agent : "(null)", http_ver, is_auto_proxy_conf);
    return SHADNET_HOOK_CONTINUE(sceHttpCreateTemplate, libhttp_ctx_id, user_agent, http_ver,
                                 is_auto_proxy_conf);
}

s32 sceHttpDeleteConnection(s32 conn_id) {
    s32 ret = SHADNET_HOOK_CONTINUE(sceHttpDeleteConnection, conn_id);
    if (ret == 0) {
        std::scoped_lock lk{urls_mutex};
        if (connection_urls.contains(conn_id)) {
            connection_urls.erase(conn_id);
        }
        if (connection_servers.contains(conn_id)) {
            connection_servers.erase(conn_id);
        }
    }
    return ret;
}

s32 sceHttpDeleteRequest(s32 req_id) {
    s32 ret = SHADNET_HOOK_CONTINUE(sceHttpDeleteRequest, req_id);
    if (ret == 0) {
        std::scoped_lock lk{urls_mutex};
        if (request_urls.contains(req_id)) {
            request_urls.erase(req_id);
        }
    }
    return ret;
}

s32 sceHttpGetStatusCode(s32 req_id, s32* status_code) {
    s32 ret = SHADNET_HOOK_CONTINUE(sceHttpGetStatusCode, req_id, status_code);
    if (ret == 0) {
        std::scoped_lock lk{urls_mutex};
        if (request_urls.contains(req_id)) {
            LOG_INFO(Lib_Http, "called, url={}, status_code={}", request_urls[req_id],
                     *status_code);
        }
    }
    return ret;
}

void RegisterHooks() {
    return RegisterHttpHooks();
}

} // namespace Libraries::Network::Http
