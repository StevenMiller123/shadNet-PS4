// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <memory>
#include <mutex>
#include <magic_enum/magic_enum.hpp>

#include "common/logging/log.h"
#include "common/shadnet_hook.h"
#include "common/types.h"
#include "core/libraries/network/fake_psn/fake_psn.h"
#include "core/libraries/network/http2/http2.h"
#include "core/libraries/network/net/net_error.h"

extern "C" {
void sceHttp2CreateRequestWithURL();
void sceHttp2CreateTemplate();
void sceHttp2DeleteRequest();
void sceHttp2SendRequest();
void sceHttp2GetStatusCode();
void sceHttp2GetAllResponseHeaders();
void sceHttp2ReadData();
}

SHADNET_HOOK_DECLARE(Libraries::Network::Http2, sceHttp2CreateRequestWithURL);
SHADNET_HOOK_DECLARE(Libraries::Network::Http2, sceHttp2CreateTemplate);
SHADNET_HOOK_DECLARE(Libraries::Network::Http2, sceHttp2DeleteRequest);
SHADNET_HOOK_DECLARE(Libraries::Network::Http2, sceHttp2SendRequest);
SHADNET_HOOK_DECLARE(Libraries::Network::Http2, sceHttp2GetStatusCode);
SHADNET_HOOK_DECLARE(Libraries::Network::Http2, sceHttp2GetAllResponseHeaders);
SHADNET_HOOK_DECLARE(Libraries::Network::Http2, sceHttp2ReadData);

static void RegisterHttp2Hooks() {
    SHADNET_HOOK(Libraries::Network::Http2, sceHttp2CreateRequestWithURL);
    SHADNET_HOOK(Libraries::Network::Http2, sceHttp2CreateTemplate);
    SHADNET_HOOK(Libraries::Network::Http2, sceHttp2DeleteRequest);
    SHADNET_HOOK(Libraries::Network::Http2, sceHttp2SendRequest);
    SHADNET_HOOK(Libraries::Network::Http2, sceHttp2GetStatusCode);
    SHADNET_HOOK(Libraries::Network::Http2, sceHttp2GetAllResponseHeaders);
    SHADNET_HOOK(Libraries::Network::Http2, sceHttp2ReadData);
}

namespace Libraries::Network::Http2 {

namespace {

struct RequestInfo {
    std::string method;
    std::string url;
};

std::mutex g_mutex{};
std::map<s32, RequestInfo> g_requests{};
std::map<s32, std::shared_ptr<FakeRequest>> g_fake_requests{};

std::shared_ptr<FakeRequest> FindFakeRequest(s32 req_id) {
    std::scoped_lock lk{g_mutex};
    auto it = g_fake_requests.find(req_id);
    return it == g_fake_requests.end() ? nullptr : it->second;
}

} // namespace

s32 sceHttp2CreateRequestWithURL(s32 tmpl_id, const char* method, const char* url,
                                 u64 content_length) {
    LOG_INFO(Lib_Http2, "called, method={}, url={}, content_length={}", method ? method : "(null)",
             url ? url : "(null)", content_length);
    if (url == nullptr) {
        return SHADNET_HOOK_CONTINUE(sceHttp2CreateRequestWithURL, tmpl_id, method, url,
                                     content_length);
    }

    // libSceNpWebApi2 requests go to the shadnet server.
    std::string target = url;
    if (auto redirect = FakePsn::Instance().Redirect(url)) {
        LOG_INFO(Lib_Http2, "redirecting {} to {}", url, *redirect);
        target = std::move(*redirect);
    }
    s32 req_id = SHADNET_HOOK_CONTINUE(sceHttp2CreateRequestWithURL, tmpl_id, method,
                                       target.c_str(), content_length);
    if (req_id > 0) {
        std::scoped_lock lk{g_mutex};
        g_requests[req_id] = {method ? method : "", std::move(target)};
    }
    return req_id;
}

s32 sceHttp2CreateTemplate(s32 ctx_id, const char* user_agent, s32 http_ver, s32 auto_proxy_conf) {
    OrbisHttp2HttpVersion enum_ver = OrbisHttp2HttpVersion(http_ver);
    LOG_INFO(Lib_Http2, "called, user_agent={}, http_ver={}", user_agent ? user_agent : "(null)",
             magic_enum::enum_name(enum_ver));
    return SHADNET_HOOK_CONTINUE(sceHttp2CreateTemplate, ctx_id, user_agent, http_ver,
                                 auto_proxy_conf);
}

s32 sceHttp2DeleteRequest(s32 req_id) {
    s32 ret = SHADNET_HOOK_CONTINUE(sceHttp2DeleteRequest, req_id);
    if (ret == 0) {
        std::scoped_lock lk{g_mutex};
        g_requests.erase(req_id);
        g_fake_requests.erase(req_id);
    }
    return ret;
}

s32 sceHttp2SendRequest(s32 req_id, const void* post_data, u64 size) {
    RequestInfo info;
    {
        std::scoped_lock lk{g_mutex};
        auto it = g_requests.find(req_id);
        if (it != g_requests.end()) {
            info = it->second;
        }
    }

    // Allow non PSN requests to go through unaffected
    if (!IsPsnHost(UrlHost(info.url))) {
        return SHADNET_HOOK_CONTINUE(sceHttp2SendRequest, req_id, post_data, size);
    }

    // Fake PSN requests
    const std::span<const u8> body{static_cast<const u8*>(post_data),
                                   post_data != nullptr ? size : 0};
    auto response = FakePsn::Instance().Respond(info.method, info.url, body);
    if (!response) {
        LOG_ERROR(Lib_Http2, "blocked {} {}, it has no fake response", info.method, info.url);
        return ORBIS_NET_ERROR_RESOLVER_ENOHOST;
    }

    LOG_INFO(Lib_Http2, "faking {} {} with status {}", info.method, info.url,
             response->status_code);
    std::scoped_lock lk{g_mutex};
    g_fake_requests[req_id] = std::make_shared<FakeRequest>(std::move(*response));
    return 0;
}

s32 sceHttp2GetStatusCode(s32 req_id, s32* status_code) {
    // If we faked a request, return our fake's status code.
    if (auto fake = FindFakeRequest(req_id)) {
        if (status_code != nullptr) {
            *status_code = fake->GetStatusCode();
        }
        return 0;
    }

    // Not a faked request, pass to the real function.
    s32 ret = SHADNET_HOOK_CONTINUE(sceHttp2GetStatusCode, req_id, status_code);
    if (ret == 0) {
        std::scoped_lock lk{g_mutex};
        if (auto it = g_requests.find(req_id); it != g_requests.end()) {
            LOG_INFO(Lib_Http2, "called, url={}, status_code={}", it->second.url, *status_code);
        }
    }
    return ret;
}

s32 sceHttp2GetAllResponseHeaders(s32 req_id, char** header, u64* header_size) {
    // If we faked the request, return our fake response headers
    if (auto fake = FindFakeRequest(req_id)) {
        if (header != nullptr) {
            *header = const_cast<char*>(fake->GetHeaders().data());
        }
        if (header_size != nullptr) {
            *header_size = fake->GetHeaders().size();
        }
        return 0;
    }

    return SHADNET_HOOK_CONTINUE(sceHttp2GetAllResponseHeaders, req_id, header, header_size);
}

s32 sceHttp2ReadData(s32 req_id, void* data, u64 size) {
    // If this is a faked request, return our fake's data
    if (auto fake = FindFakeRequest(req_id)) {
        return static_cast<s32>(fake->Read(data, size));
    }

    return SHADNET_HOOK_CONTINUE(sceHttp2ReadData, req_id, data, size);
}

void RegisterHooks() {
    return RegisterHttp2Hooks();
}

} // namespace Libraries::Network::Http2
