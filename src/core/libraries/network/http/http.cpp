// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include <format>
#include <map>
#include <memory>
#include <mutex>
#include <magic_enum/magic_enum.hpp>
#include <orbis/Http.h>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "common/types.h"
#include "core/libraries/network/fake_psn/fake_psn.h"
#include "core/libraries/network/http/http.h"
#include "core/libraries/network/http/http_error.h"

SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpCreateConnection);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpCreateConnectionWithURL);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpCreateRequest);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpCreateRequest2);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpCreateRequestWithURL);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpCreateRequestWithURL2);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpCreateTemplate);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpDeleteConnection);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpDeleteRequest);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpSendRequest);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpGetStatusCode);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpGetResponseContentLength);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpGetAllResponseHeaders);
SHADNET_HOOK_DECLARE(Libraries::Network::Http, sceHttpReadData);

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
    SHADNET_HOOK(Libraries::Network::Http, sceHttpSendRequest);
    SHADNET_HOOK(Libraries::Network::Http, sceHttpGetStatusCode);
    SHADNET_HOOK(Libraries::Network::Http, sceHttpGetResponseContentLength);
    SHADNET_HOOK(Libraries::Network::Http, sceHttpGetAllResponseHeaders);
    SHADNET_HOOK(Libraries::Network::Http, sceHttpReadData);
}

namespace Libraries::Network::Http {

namespace {

constexpr s32 ContentLengthExists = 0;

struct RequestInfo {
    std::string method;
    std::string url;
    s32 conn_id = 0;
    std::string path;
};

struct HostCapture {
    bool active;
    u32 host_size;
    char host[256]; // hostnames are at most 255 characters
};
thread_local HostCapture t_host_capture{};

std::mutex g_mutex{};
std::map<s32, std::string> g_connection_origins{};
std::map<s32, RequestInfo> g_requests{};
std::map<s32, std::shared_ptr<FakeRequest>> g_fake_requests{};

std::string_view MethodName(s32 method) {
    switch (method) {
    case ORBIS_HTTP_METHOD_GET:
        return "GET";
    case ORBIS_HTTP_METHOD_POST:
        return "POST";
    case ORBIS_HTTP_METHOD_HEAD:
        return "HEAD";
    case ORBIS_HTTP_METHOD_OPTIONS:
        return "OPTIONS";
    case ORBIS_HTTP_METHOD_PUT:
        return "PUT";
    case ORBIS_HTTP_METHOD_DELETE:
        return "DELETE";
    case ORBIS_HTTP_METHOD_TRACE:
        return "TRACE";
    case ORBIS_HTTP_METHOD_CONNECT:
        return "CONNECT";
    default:
        return "CUSTOM";
    }
}

// Returns the URL to create with, the shadnet server's when the PSN URL is redirected.
std::string RedirectUrl(const char* url) {
    if (auto redirect = FakePsn::Instance().Redirect(url)) {
        LOG_INFO(Lib_Http, "redirecting {} to {}", url, *redirect);
        return *redirect;
    }
    return url;
}

void TrackRequest(s32 req_id, std::string_view method, std::string url) {
    if (req_id > 0) {
        std::scoped_lock lk{g_mutex};
        g_requests[req_id] = {std::string(method), std::move(url)};
    }
}

void TrackRequestOnConnection(s32 req_id, s32 conn_id, std::string_view method, const char* path) {
    if (req_id <= 0) {
        return;
    }
    const std::string request_path = path ? path : "";
    std::scoped_lock lk{g_mutex};
    std::string origin;
    if (auto it = g_connection_origins.find(conn_id); it != g_connection_origins.end()) {
        origin = it->second;
    }
    g_requests[req_id] = {std::string(method), origin + request_path, conn_id, request_path};
}

std::shared_ptr<FakeRequest> FindFakeRequest(s32 req_id) {
    std::scoped_lock lk{g_mutex};
    auto it = g_fake_requests.find(req_id);
    return it == g_fake_requests.end() ? nullptr : it->second;
}

} // namespace

s32 sceHttpCreateConnection(s32 tmpl_id, const char* server_name, const char* scheme, u16 port,
                            s32 is_enable_keepalive) {
    LOG_INFO(Lib_Http, "called, server_name={}, scheme={}, port={}, is_enable_keepalive={}",
             server_name ? server_name : "(null)", scheme ? scheme : "(null)", port,
             is_enable_keepalive);
    s32 conn_id = SHADNET_HOOK_CONTINUE(sceHttpCreateConnection, tmpl_id, server_name, scheme, port,
                                        is_enable_keepalive);
    if (conn_id > 0 && server_name != nullptr) {
        std::scoped_lock lk{g_mutex};
        g_connection_origins[conn_id] =
            std::format("{}://{}:{}", scheme ? scheme : "http", server_name, port);
    }
    return conn_id;
}

s32 sceHttpCreateConnectionWithURL(s32 tmpl_id, const char* url, bool enable_keepalive) {
    LOG_INFO(Lib_Http, "called, url={}, enable_keepalive={}", url ? url : "(null)",
             enable_keepalive);
    if (url == nullptr) {
        return SHADNET_HOOK_CONTINUE(sceHttpCreateConnectionWithURL, tmpl_id, url,
                                     enable_keepalive);
    }
    const std::string target = RedirectUrl(url);
    s32 conn_id = SHADNET_HOOK_CONTINUE(sceHttpCreateConnectionWithURL, tmpl_id, target.c_str(),
                                        enable_keepalive);
    if (conn_id > 0) {
        std::scoped_lock lk{g_mutex};
        g_connection_origins[conn_id] = std::string(UrlOrigin(target));
    }
    return conn_id;
}

s32 sceHttpCreateRequest(s32 conn_id, s32 method, const char* path, u64 content_length) {
    LOG_INFO(Lib_Http, "called, method={}, path={}, content_length={}", MethodName(method),
             path ? path : "(null)", content_length);
    s32 req_id = SHADNET_HOOK_CONTINUE(sceHttpCreateRequest, conn_id, method, path, content_length);
    TrackRequestOnConnection(req_id, conn_id, MethodName(method), path);
    return req_id;
}

s32 sceHttpCreateRequest2(s32 conn_id, const char* method, const char* path, u64 content_length) {
    LOG_INFO(Lib_Http, "called, method={}, path={}, content_length={}", method ? method : "(null)",
             path ? path : "(null)", content_length);
    s32 req_id =
        SHADNET_HOOK_CONTINUE(sceHttpCreateRequest2, conn_id, method, path, content_length);
    TrackRequestOnConnection(req_id, conn_id, method ? method : "", path);
    return req_id;
}

s32 sceHttpCreateRequestWithURL(s32 conn_id, s32 method, const char* url, u64 content_length) {
    LOG_INFO(Lib_Http, "called, method={}, url={}, content_length={}", MethodName(method),
             url ? url : "(null)", content_length);
    if (url == nullptr) {
        return SHADNET_HOOK_CONTINUE(sceHttpCreateRequestWithURL, conn_id, method, url,
                                     content_length);
    }
    std::string target = RedirectUrl(url);
    s32 req_id = SHADNET_HOOK_CONTINUE(sceHttpCreateRequestWithURL, conn_id, method, target.c_str(),
                                       content_length);
    TrackRequest(req_id, MethodName(method), std::move(target));
    return req_id;
}

s32 sceHttpCreateRequestWithURL2(s32 conn_id, const char* method, const char* url,
                                 u64 content_length) {
    LOG_INFO(Lib_Http, "called, method={}, url={}, content_length={}", method ? method : "(null)",
             url ? url : "(null)", content_length);
    if (url == nullptr) {
        return SHADNET_HOOK_CONTINUE(sceHttpCreateRequestWithURL2, conn_id, method, url,
                                     content_length);
    }
    std::string target = RedirectUrl(url);
    s32 req_id = SHADNET_HOOK_CONTINUE(sceHttpCreateRequestWithURL2, conn_id, method,
                                       target.c_str(), content_length);
    TrackRequest(req_id, method ? method : "", std::move(target));
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
        std::scoped_lock lk{g_mutex};
        g_connection_origins.erase(conn_id);
    }
    return ret;
}

s32 sceHttpDeleteRequest(s32 req_id) {
    s32 ret = SHADNET_HOOK_CONTINUE(sceHttpDeleteRequest, req_id);
    if (ret == 0) {
        std::scoped_lock lk{g_mutex};
        g_requests.erase(req_id);
        g_fake_requests.erase(req_id);
    }
    return ret;
}

s32 sceHttpSendRequest(s32 req_id, const void* post_data, u64 size) {
    RequestInfo info;
    {
        std::scoped_lock lk{g_mutex};
        auto it = g_requests.find(req_id);
        if (it != g_requests.end()) {
            info = it->second;
        }
    }

    // A request on a connection created before our hooks
    // Start by sending it, where libSceHttp's resolver calls expose the URL through Net hooks.
    if (UrlHost(info.url).empty() && info.conn_id != 0) {
        t_host_capture.active = true;
        t_host_capture.host_size = 0;
        const s32 ret = SHADNET_HOOK_CONTINUE(sceHttpSendRequest, req_id, post_data, size);
        const std::string host{t_host_capture.host, t_host_capture.host_size};
        t_host_capture.active = false;
        if (host.empty()) {
            return ret;
        }

        // The scheme and port aren't known, faking a request only needs the host and path.
        const std::string origin = "https://" + host;
        info.url = origin + info.path;
        {
            std::scoped_lock lk{g_mutex};
            g_connection_origins[info.conn_id] = origin;
            if (auto it = g_requests.find(req_id); it != g_requests.end()) {
                it->second.url = info.url;
            }
        }
        LOG_DEBUG(Lib_Http, "Unhooked connection {:#x} has host {}", info.conn_id, host);
        if (!IsPsnHost(host)) {
            return ret;
        }
    } else if (!IsPsnHost(UrlHost(info.url))) {
        // Allow non PSN requests to go through unaffected
        return SHADNET_HOOK_CONTINUE(sceHttpSendRequest, req_id, post_data, size);
    }

    // PSN request, return a faked request.
    const std::span<const u8> body{static_cast<const u8*>(post_data),
                                   post_data != nullptr ? size : 0};
    auto response = FakePsn::Instance().Respond(info.method, info.url, body);
    if (!response) {
        LOG_ERROR(Lib_Http, "blocked {} {}, it has no fake response", info.method, info.url);
        return ORBIS_HTTP_ERROR_RESOLVER_ENOHOST;
    }

    LOG_INFO(Lib_Http, "faking {} {} with status {}", info.method, info.url, response->status_code);
    std::scoped_lock lk{g_mutex};
    g_fake_requests[req_id] = std::make_shared<FakeRequest>(std::move(*response));
    return 0;
}

s32 sceHttpGetStatusCode(s32 req_id, s32* status_code) {
    if (auto fake = FindFakeRequest(req_id)) {
        if (status_code == nullptr) {
            return ORBIS_HTTP_ERROR_INVALID_VALUE;
        }
        *status_code = fake->GetStatusCode();
        return 0;
    }
    s32 ret = SHADNET_HOOK_CONTINUE(sceHttpGetStatusCode, req_id, status_code);
    if (ret == 0) {
        std::scoped_lock lk{g_mutex};
        if (auto it = g_requests.find(req_id); it != g_requests.end()) {
            LOG_INFO(Lib_Http, "called, url={}, status_code={}", it->second.url, *status_code);
        }
    }
    return ret;
}

s32 sceHttpGetResponseContentLength(s32 req_id, s32* result, u64* content_length) {
    if (auto fake = FindFakeRequest(req_id)) {
        if (result != nullptr) {
            *result = ContentLengthExists;
        }
        if (content_length != nullptr) {
            *content_length = fake->GetContentLength();
        }
        return 0;
    }
    return SHADNET_HOOK_CONTINUE(sceHttpGetResponseContentLength, req_id, result, content_length);
}

s32 sceHttpGetAllResponseHeaders(s32 req_id, char** header, u64* header_size) {
    if (auto fake = FindFakeRequest(req_id)) {
        if (header != nullptr) {
            *header = const_cast<char*>(fake->GetHeaders().data());
        }
        if (header_size != nullptr) {
            *header_size = fake->GetHeaders().size();
        }
        return 0;
    }
    return SHADNET_HOOK_CONTINUE(sceHttpGetAllResponseHeaders, req_id, header, header_size);
}

s32 sceHttpReadData(s32 req_id, void* data, u32 size) {
    if (auto fake = FindFakeRequest(req_id)) {
        return static_cast<s32>(fake->Read(data, size));
    }
    return SHADNET_HOOK_CONTINUE(sceHttpReadData, req_id, data, size);
}

void NoteResolvedHost(std::string_view host) {
    if (t_host_capture.active && t_host_capture.host_size == 0) {
        const u64 size = std::min(host.size(), sizeof(t_host_capture.host));
        std::memcpy(t_host_capture.host, host.data(), size);
        t_host_capture.host_size = static_cast<u32>(size);
    }
}

void RegisterHooks() {
    return RegisterHttpHooks();
}

} // namespace Libraries::Network::Http
