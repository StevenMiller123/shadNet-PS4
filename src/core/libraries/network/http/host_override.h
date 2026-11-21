// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include <algorithm>
#include "common/logging/log.h"
#include "http.h"

namespace Libraries::Network::Http {

// JSON shape: flat object mapping endpoint -> replacement URL. Example:
//   {
//     "api.something.dev":                    "http://localhost:8080",
//     "discovery.something.com:5300":         "http://localhost:8080",
//     "https://api.example.com:443":          "http://localhost:8081",
//     "*":                                    "http://localhost:8080"
//   }
//
// Keys are tried in order of specificity, most-specific first:
//   1. "scheme://host:port" - matches that exact endpoint
//   2. "host:port"          - matches host+port on any scheme
//   3. "host"               - matches host on any scheme/port (most common)
//   4. "*"                  - catch-all fallback
//
// Replacement value is a URL with scheme + host + optional port. When port is
// omitted the default for the scheme is used (80 for http, 443 for https).
// When scheme is omitted the connection's original scheme is preserved.
struct HostOverrideTarget {
    std::string scheme; // "http", "https", or "" to mean "preserve original"
    std::string host;
    u16 port = 0; // 0 means "preserve original (or default-for-scheme if scheme changed)"
};

struct HttpResponse {
    int status_code = 0;
    u64 content_length = 0;
    int content_length_result = ORBIS_HTTP_ERROR_NO_CONTENT_LENGTH;
    std::vector<u8> body;
    u64 read_cursor = 0;
    std::string all_headers_blob; // Pre-formatted "Name: Value\r\n..." string.
};

// Parse a JSON string into a hostname to target map
std::unordered_map<std::string, HostOverrideTarget> ParseHostOverridesJson(
    const std::string& json_text);

bool ApplyHostOverride(std::string& scheme, std::string& host, u16& port, bool& is_secure);

// Populate a response object with the shape a transport-level failure produces:
// no status line, no headers, no body. Used by the no-internet path.
void SynthesizeTransportFailureResponse(HttpResponse& res) {
    res.status_code = 0;
    res.content_length = 0;
    res.content_length_result = ORBIS_HTTP_ERROR_NO_CONTENT_LENGTH;
    res.body.clear();
    res.read_cursor = 0;
    res.all_headers_blob.clear();
}

// Parse a single replacement value into a HostOverrideTarget. Accepts forms:
//   "host"                   preserve scheme, preserve port
//   "host:port"              preserve scheme, set port
//   "http://host"            set scheme to http, port defaults to 80
//   "https://host"           set scheme to https, port defaults to 443
//   "http://host:port"       set scheme + port explicitly
//   "https://host:port"      set scheme + port explicitly
// Bad ports fall back to port=0
static HostOverrideTarget ParseHostOverrideTarget(std::string_view value) {
    HostOverrideTarget out;

    // Pull off scheme prefix if present.
    if (const auto scheme_end = value.find("://"); scheme_end != std::string_view::npos) {
        std::string scheme(value.substr(0, scheme_end));
        std::transform(scheme.begin(), scheme.end(), scheme.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (scheme == "http" || scheme == "https") {
            out.scheme = std::move(scheme);
        }
        // Unknown scheme,leave out.scheme empty
        value = value.substr(scheme_end + 3);
    }

    // Now value is "host" or "host:port".
    if (const auto colon = value.find(':'); colon != std::string_view::npos) {
        out.host.assign(value.substr(0, colon));
        try {
            const unsigned long p = std::stoul(std::string(value.substr(colon + 1)));
            if (p > 0 && p <= 65535) {
                out.port = static_cast<u16>(p);
            }
        } catch (...) {
            // ignore - port stays 0
        }
    } else {
        out.host.assign(value);
    }
    return out;
}

std::unordered_map<std::string, HostOverrideTarget> ParseHostOverridesJson(
    const std::string& json_text) {
    std::unordered_map<std::string, HostOverrideTarget> out;
    if (json_text.empty()) {
        return out;
    }
    nlohmann::json root;
    try {
        root = nlohmann::json::parse(json_text);
    } catch (const std::exception& e) {
        LOG_ERROR(Lib_Http, "host overrides JSON parse failed: {}", e.what());
        return out;
    }
    if (!root.is_object()) {
        LOG_ERROR(Lib_Http, "host overrides JSON root must be an object");
        return out;
    }
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (!it.key().empty() && it.key().front() == '_') {
            continue;
        }
        if (!it.value().is_string()) {
            LOG_ERROR(Lib_Http, "host overrides JSON: value for '{}' is not a string; skipped",
                      it.key());
            continue;
        }
        const std::string value = it.value().get<std::string>();
        if (value.empty()) {
            LOG_ERROR(Lib_Http, "host overrides JSON: value for '{}' is empty; skipped", it.key());
            continue;
        }
        out.emplace(it.key(), ParseHostOverrideTarget(value));
    }
    return out;
}

struct HostOverrideState {
    bool loaded = false;
    std::unordered_map<std::string, HostOverrideTarget> entries;
};

static HostOverrideState LoadHostOverrideState() {
    HostOverrideState s;
    s.loaded = true;
    std::filesystem::path path;
    if (const char* path_env = std::getenv("SHADPS4_HTTP_HOST_OVERRIDES_JSON");
        path_env && path_env[0]) {
        // Explicit override path - useful for dev / testing.
        path = path_env;
    } else {
        path = "/data/shadnet/host_overrides.json";
    }
    std::ifstream f(path);
    if (!f.is_open()) {
        return s;
    }
    std::stringstream buf;
    buf << f.rdbuf();
    s.entries = ParseHostOverridesJson(buf.str());
    LOG_INFO(Lib_Http, "loaded {} host override entries from {}", s.entries.size(), path.string());
    return s;
}

static const HostOverrideState& GetHostOverrideState() {
    static const HostOverrideState s = LoadHostOverrideState();
    return s;
}

bool ApplyHostOverride(std::string& scheme, std::string& host, u16& port, bool& is_secure) {
    const auto& state = GetHostOverrideState();
    if (state.entries.empty()) {
        return false;
    }
    // Look up most-specific match first. Keys can be:
    //   "scheme://host:port"  - matches that exact endpoint
    //   "host:port"           - matches host+port on any scheme
    //   "host"                - matches host on any scheme/port
    //   "*"                   - catch-all fallback
    const std::string full_key = scheme + "://" + host + ":" + std::to_string(port);
    const std::string host_port_key = host + ":" + std::to_string(port);

    auto it = state.entries.find(full_key);
    if (it == state.entries.end()) {
        it = state.entries.find(host_port_key);
    }
    if (it == state.entries.end()) {
        it = state.entries.find(host);
    }
    if (it == state.entries.end()) {
        it = state.entries.find("*");
    }
    if (it == state.entries.end()) {
        return false;
    }

    const std::string orig_scheme = scheme;
    const std::string orig_host = host;
    const u16 orig_port = port;
    const HostOverrideTarget& target = it->second;

    host = target.host;

    // Scheme handling: explicit scheme in JSON wins; otherwise preserve.
    if (!target.scheme.empty()) {
        scheme = target.scheme;
        is_secure = (target.scheme == "https");
    }

    if (target.port != 0) {
        port = target.port;
    } else if (!target.scheme.empty() && target.scheme != orig_scheme) {
        if (orig_scheme == "https" && port == 443) {
            port = 80;
        } else if (orig_scheme == "http" && port == 80) {
            port = 443;
        }
    }

    LOG_INFO(Lib_Http, "host override active: {}://{}:{} -> {}://{}:{} (matched key '{}')",
             orig_scheme, orig_host, orig_port, scheme, host, port, it->first);
    return true;
}

} // namespace Libraries::Network::Http