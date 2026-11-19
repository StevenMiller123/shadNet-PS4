// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <ctime>
#include <format>

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/network/fake_psn/fake_psn.h"
#include "shadnet/config.h"

namespace Libraries::Network {

namespace {

// Known domains for PSN requests
constexpr std::array<std::string_view, 3> PsnDomains = {
    "playstation.net",
    "playstation.com",
    "sonyentertainmentnetwork.com",
};

// libSceNpManager requests asm.%s.community.playstation.net.
constexpr std::string_view AsmHostPrefix = "asm.";
constexpr std::string_view AsmHostSuffix = ".community.playstation.net";

// libSceNpMatching2 requests static-resource.%s.community.playstation.net.
constexpr std::string_view StaticResourceHostPrefix = "static-resource.";
constexpr std::string_view StaticResourceHostSuffix = ".community.playstation.net";

// libSceNpUtility's word filter requests filter.%s.%s.community.playstation.net.
constexpr std::string_view WordFilterHostPrefix = "filter.";
constexpr std::string_view WordFilterHostSuffix = ".community.playstation.net";

// libSceNpWebApi2 requests ps4.%s.playstation.net/api/<api group><path>.
constexpr std::string_view WebApi2HostPrefix = "ps4.";
constexpr std::string_view WebApi2PathPrefix = "/api/";

// Title tokens only ever reach requests we fake, so we don't need them to be valid.
constexpr std::string_view FakeTitleToken = "shadnet";
constexpr s64 TitleTokenLifetime = 60 * 60;

// Sizes for encryption passphrases the firmware uses.
// We're using these to provide correctly sized outputs, no encryption is needed.
constexpr u64 CommPassphraseSize = 0x80;
constexpr u64 CommPassphraseIvSize = 0x10;

std::string Base64Encode(std::span<const u8> data) {
    static constexpr std::string_view letters =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    for (u64 i = 0; i < data.size(); i += 3) {
        const u32 chunk = (data[i] << 16) | (i + 1 < data.size() ? data[i + 1] << 8 : 0) |
                          (i + 2 < data.size() ? data[i + 2] : 0);
        out += letters[(chunk >> 18) & 0x3f];
        out += letters[(chunk >> 12) & 0x3f];
        out += i + 1 < data.size() ? letters[(chunk >> 6) & 0x3f] : '=';
        out += i + 2 < data.size() ? letters[chunk & 0x3f] : '=';
    }
    return out;
}

// Expands FNV-1a hashes of the comm id into a passphrase.
std::array<u8, CommPassphraseSize> CommPassphrase(std::string_view comm_id) {
    std::array<u8, CommPassphraseSize> passphrase{};
    for (u64 block = 0; block < passphrase.size() / sizeof(u64); block++) {
        u64 hash = 0xcbf29ce484222325;
        for (const char c : comm_id) {
            hash = (hash ^ static_cast<u8>(c)) * 0x100000001b3;
        }
        hash = (hash ^ block) * 0x100000001b3;
        std::memcpy(passphrase.data() + block * sizeof(u64), &hash, sizeof(hash));
    }
    return passphrase;
}

bool EqualsIgnoreCase(std::string_view a, std::string_view b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
               return std::tolower(static_cast<u8>(x)) == std::tolower(static_cast<u8>(y));
           });
}

bool EndsWithIgnoreCase(std::string_view str, std::string_view suffix) {
    return str.size() >= suffix.size() &&
           EqualsIgnoreCase(str.substr(str.size() - suffix.size()), suffix);
}

bool StartsWithIgnoreCase(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size() && EqualsIgnoreCase(str.substr(0, prefix.size()), prefix);
}

std::string_view UrlPath(std::string_view url) {
    const std::string_view origin = UrlOrigin(url);
    const std::string_view path = url.substr(origin.size());
    return path.empty() ? "/" : path;
}

std::string WebApiServerUrl() {
    std::string url = ShadNet::Settings::GetInstance().GetWebApiServerUrl();
    while (url.ends_with('/')) {
        url.pop_back();
    }
    return url;
}

} // namespace

bool IsPsnHost(std::string_view host) {
    while (host.ends_with('.')) {
        host.remove_suffix(1);
    }
    while (host.starts_with('.')) {
        host.remove_prefix(1);
    }
    return std::any_of(PsnDomains.begin(), PsnDomains.end(), [&](std::string_view domain) {
        if (EqualsIgnoreCase(host, domain)) {
            return true;
        }
        return host.size() > domain.size() && EndsWithIgnoreCase(host, domain) &&
               host[host.size() - domain.size() - 1] == '.';
    });
}

std::string_view UrlOrigin(std::string_view url) {
    const u64 scheme_end = url.find("://");
    const u64 host_start = scheme_end == std::string_view::npos ? 0 : scheme_end + 3;
    const u64 path_start = url.find_first_of("/?#", host_start);
    return path_start == std::string_view::npos ? url : url.substr(0, path_start);
}

std::string_view UrlHost(std::string_view url) {
    std::string_view host = UrlOrigin(url);
    if (const u64 scheme_end = host.find("://"); scheme_end != std::string_view::npos) {
        host.remove_prefix(scheme_end + 3);
    }
    if (const u64 at = host.rfind('@'); at != std::string_view::npos) {
        host.remove_prefix(at + 1);
    }
    if (const u64 colon = host.rfind(':'); colon != std::string_view::npos) {
        host = host.substr(0, colon);
    }
    return host;
}

FakePsn& FakePsn::Instance() {
    static FakePsn instance;
    return instance;
}

std::optional<std::string> FakePsn::Redirect(std::string_view url) const {
    const std::string_view host = UrlHost(url);
    const std::string_view path = UrlPath(url);

    // Check for requests from libSceNpWebApi2, reformat these to match shadNet's URLs.
    if (StartsWithIgnoreCase(host, WebApi2HostPrefix) && IsPsnHost(host) &&
        path.starts_with(WebApi2PathPrefix)) {
        std::string_view rest = path.substr(WebApi2PathPrefix.size());
        const u64 group_end = rest.find_first_of("/?");
        rest = group_end == std::string_view::npos ? std::string_view{} : rest.substr(group_end);
        return std::format("{}{}", WebApiServerUrl(), rest.empty() ? "/" : rest);
    }
    return std::nullopt;
}

std::optional<FakeResponse> FakePsn::Respond(std::string_view method, std::string_view url,
                                             std::span<const u8> body) const {
    const std::string_view host = UrlHost(url);
    if (StartsWithIgnoreCase(host, AsmHostPrefix) && EndsWithIgnoreCase(host, AsmHostSuffix)) {
        return AsmRequest(method, UrlPath(url));
    }
    if (StartsWithIgnoreCase(host, StaticResourceHostPrefix) &&
        EndsWithIgnoreCase(host, StaticResourceHostSuffix)) {
        return StaticResourceRequest(method, UrlPath(url));
    }
    if (StartsWithIgnoreCase(host, WordFilterHostPrefix) &&
        EndsWithIgnoreCase(host, WordFilterHostSuffix)) {
        return WordFilterRequest(method, UrlPath(url), body);
    }
    return std::nullopt;
}

std::optional<FakeResponse> FakePsn::WordFilterRequest(std::string_view method,
                                                       std::string_view path,
                                                       std::span<const u8> body) const {
    // /basic_view/func/censor_message
    // Used by sceNpWordFilterCensorComment and sceNpWordFilterSanitizeComment to censor comments.
    if (method != "POST" || path != "/basic_view/func/censor_message") {
        return std::nullopt;
    }

    // The request contains <message ...>text</message>, and expects a censored result.
    // The text is still XML escaped, which the library unescapes when parsing the response.
    const std::string_view request{reinterpret_cast<const char*>(body.data()), body.size()};
    std::string_view message;
    if (const u64 open = request.find("<message"); open != std::string_view::npos) {
        if (const u64 start = request.find('>', open); start != std::string_view::npos) {
            const u64 end = request.find("</message>", start);
            if (end != std::string_view::npos) {
                message = request.substr(start + 1, end - start - 1);
            }
        }
    }

    return FakeResponse{
        200, "text/xml",
        std::format("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                    "<npfilter result=\"00\"><message check=\"ok\">{}</message></npfilter>",
                    message)};
}

std::optional<FakeResponse> FakePsn::StaticResourceRequest(std::string_view method,
                                                           std::string_view path) const {
    // /np/resource/ps4-title/<comm id>/matching/<comm id>-matching.xml
    // Used by libSceNpMatching2 to retrieve matchmaking servers.
    constexpr std::string_view MatchingPrefix = "/np/resource/ps4-title/";
    constexpr std::string_view MatchingSuffix = "-matching.xml";
    if (method != "GET" || !path.starts_with(MatchingPrefix) || !path.ends_with(MatchingSuffix)) {
        return std::nullopt;
    }
    const std::string_view comm_id = path.substr(
        MatchingPrefix.size(), path.find('/', MatchingPrefix.size()) - MatchingPrefix.size());

    const std::string server_url = ShadNet::Settings::GetInstance().GetServerUrl();
    const u64 colon = server_url.rfind(':');
    if (colon == std::string::npos) {
        LOG_ERROR(Lib_Http, "invalid shadnet server url {}", server_url);
        return std::nullopt;
    }
    const std::string_view host = std::string_view{server_url}.substr(0, colon);
    const std::string_view port = std::string_view{server_url}.substr(colon + 1);

    return FakeResponse{
        200, "text/xml",
        std::format("<!--Sce-Np-Signature: {:0>40}-->\r\n"
                    "<matching2 status=\"alive\" titleid=\"{}\" channel=\"plain\">"
                    "<agent-fqdn id=\"1\" status=\"alive\" port=\"{}\">{}</agent-fqdn>"
                    "</matching2>",
                    "", comm_id, port, host)};
}

std::optional<FakeResponse> FakePsn::AsmRequest(std::string_view method,
                                                std::string_view path) const {
    // /asm/v1/apps/<np title id>/npTitleToken
    // This returns the NpTitleToken for use in other APIs. Return a fake token.
    if (method == "POST" && path.starts_with("/asm/") && path.ends_with("/npTitleToken")) {
        const s64 expiry = static_cast<s64>(std::time(nullptr)) + TitleTokenLifetime;
        return FakeResponse{
            200, "application/json",
            std::format("{{\"npTitleToken\":\"{}\",\"exp\":{}}}", FakeTitleToken, expiry)};
    }

    // /asm/v1/apps/<np title id>/npComId/<service>[?revision=<n>]
    // This returns the NP communication ID for libraries to use.
    // Encryption is ignored since it's hooked through the SceNpService IPMI server.
    if (method == "GET" && path.starts_with("/asm/") &&
        path.find("/npComId/") != std::string_view::npos) {
        static const std::array<u8, CommPassphraseIvSize> iv{};
        std::string entries;
        for (const std::string& comm_id : Common::ElfInfo::Instance().GetNpCommIds()) {
            if (!entries.empty()) {
                entries += ',';
            }
            entries +=
                std::format("{{\"npComId\":\"{}\",\"encryptedNpPassPhrase\":\"{}\","
                            "\"npPassPhraseIV\":\"{}\",\"npServiceLabel\":\"0\"}}",
                            comm_id, Base64Encode(CommPassphrase(comm_id)), Base64Encode(iv));
        }
        return FakeResponse{200, "application/json",
                            std::format("{{\"npComInfo\":[{}]}}", entries)};
    }

    // /asm/v1/apps/<np title id>/baseUrls/<api group>
    // This is the path formed by libSceNpWebApi's requests.
    if (method == "GET" && path.starts_with("/asm/") &&
        path.find("/baseUrls/") != std::string_view::npos) {
        return FakeResponse{200, "application/json",
                            std::format("{{\"url\":\"{}\"}}", WebApiServerUrl())};
    }
    return std::nullopt;
}

FakeRequest::FakeRequest(FakeResponse response) : m_response(std::move(response)) {
    m_headers = std::format("Content-Type: {}\r\nContent-Length: {}\r\n", m_response.content_type,
                            m_response.body.size());
}

u64 FakeRequest::Read(void* data, u64 size) {
    std::scoped_lock lk{m_mutex};
    const u64 count = std::min(size, m_response.body.size() - m_offset);
    if (count != 0) {
        std::memcpy(data, m_response.body.data() + m_offset, count);
        m_offset += count;
    }
    return count;
}

} // namespace Libraries::Network
