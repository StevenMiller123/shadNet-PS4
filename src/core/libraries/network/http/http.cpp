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

s32 sceHttpCreateConnection(s32 tmplId, const char* serverName, const char* scheme, u16 port,
                            s32 isEnableKeepalive) {
    LOG_INFO(Lib_Http, "called, serverName={}, scheme={}, port={}, isEnableKeepalive={}",
             serverName ? serverName : "(null)", scheme ? scheme : "(null)", port,
             isEnableKeepalive);
    s32 conn_id = SHADNET_HOOK_CONTINUE(sceHttpCreateConnection, tmplId, serverName, scheme, port,
                                        isEnableKeepalive);
    if (conn_id > 0) {
        std::scoped_lock lk{urls_mutex};
        connection_servers[conn_id] = serverName;
    }
    return conn_id;
}

s32 sceHttpCreateConnectionWithURL(s32 tmplId, const char* url, bool enableKeepalive) {
    LOG_INFO(Lib_Http, "called, url={}, enableKeepalive={}", url ? url : "(null)", enableKeepalive);
    s32 conn_id =
        SHADNET_HOOK_CONTINUE(sceHttpCreateConnectionWithURL, tmplId, url, enableKeepalive);
    if (conn_id > 0) {
        std::scoped_lock lk{urls_mutex};
        connection_urls[conn_id] = url;
    }
    return conn_id;
}

s32 sceHttpCreateRequest(s32 connId, s32 method, const char* path, u64 contentLength) {
    OrbisHttpMethod enum_method = OrbisHttpMethod(method);
    LOG_INFO(Lib_Http, "called, method={}, path={}, contentLength={}",
             magic_enum::enum_name(enum_method), path ? path : "(null)", contentLength);
    s32 req_id = SHADNET_HOOK_CONTINUE(sceHttpCreateRequest, connId, method, path, contentLength);
    if (req_id > 0) {
        std::scoped_lock lk{urls_mutex};
        if (connection_servers.contains(connId)) {
            request_urls[req_id] = connection_servers[connId] + path;
        }
    }
    return req_id;
}

s32 sceHttpCreateRequest2(s32 connId, const char* method, const char* path, u64 contentLength) {
    LOG_INFO(Lib_Http, "called, method={}, path={}, contentLength={}", method ? method : "(null)",
             path ? path : "(null)", contentLength);
    s32 req_id = SHADNET_HOOK_CONTINUE(sceHttpCreateRequest2, connId, method, path, contentLength);
    if (req_id > 0) {
        std::scoped_lock lk{urls_mutex};
        if (connection_servers.contains(connId)) {
            request_urls[req_id] = connection_servers[connId] + path;
        }
    }
    return req_id;
}

s32 sceHttpCreateRequestWithURL(s32 connId, s32 method, const char* url, u64 contentLength) {
    OrbisHttpMethod enum_method = OrbisHttpMethod(method);
    LOG_INFO(Lib_Http, "called, method={}, url={}, contentLength={}",
             magic_enum::enum_name(enum_method), url ? url : "(null)", contentLength);
    s32 req_id =
        SHADNET_HOOK_CONTINUE(sceHttpCreateRequestWithURL, connId, method, url, contentLength);
    if (req_id > 0) {
        std::scoped_lock lk{urls_mutex};
        request_urls[req_id] = url;
    }
    return req_id;
}

s32 sceHttpCreateRequestWithURL2(s32 connId, const char* method, const char* url,
                                 u64 contentLength) {
    LOG_INFO(Lib_Http, "called, method={}, url={}, contentLength={}", method ? method : "(null)",
             url ? url : "(null)", contentLength);
    s32 req_id =
        SHADNET_HOOK_CONTINUE(sceHttpCreateRequestWithURL2, connId, method, url, contentLength);
    if (req_id > 0) {
        std::scoped_lock lk{urls_mutex};
        request_urls[req_id] = url;
    }
    return req_id;
}

s32 sceHttpCreateTemplate(s32 libhttpCtxId, const char* userAgent, s32 httpVer,
                          s32 isAutoProxyConf) {
    LOG_INFO(Lib_Http, "called, userAgent={}, httpVer={}, isAutoProxyConf={}",
             userAgent ? userAgent : "(null)", httpVer, isAutoProxyConf);
    return SHADNET_HOOK_CONTINUE(sceHttpCreateTemplate, libhttpCtxId, userAgent, httpVer,
                                 isAutoProxyConf);
}

s32 sceHttpDeleteConnection(s32 connId) {
    s32 ret = SHADNET_HOOK_CONTINUE(sceHttpDeleteConnection, connId);
    if (ret == 0) {
        std::scoped_lock lk{urls_mutex};
        if (connection_urls.contains(connId)) {
            connection_urls.erase(connId);
        }
        if (connection_servers.contains(connId)) {
            connection_servers.erase(connId);
        }
    }
    return ret;
}

s32 sceHttpDeleteRequest(s32 reqId) {
    s32 ret = SHADNET_HOOK_CONTINUE(sceHttpDeleteRequest, reqId);
    if (ret == 0) {
        std::scoped_lock lk{urls_mutex};
        if (request_urls.contains(reqId)) {
            request_urls.erase(reqId);
        }
    }
    return ret;
}

s32 sceHttpGetStatusCode(s32 reqId, s32* statusCode) {
    s32 ret = SHADNET_HOOK_CONTINUE(sceHttpGetStatusCode, reqId, statusCode);
    if (ret == 0) {
        std::scoped_lock lk{urls_mutex};
        if (request_urls.contains(reqId)) {
            LOG_INFO(Lib_Http, "called, url={}, statusCode={}", request_urls[reqId], *statusCode);
        }
    }
    return ret;
}

void RegisterHooks() {
    return RegisterHttpHooks();
}

} // namespace Libraries::Network::Http