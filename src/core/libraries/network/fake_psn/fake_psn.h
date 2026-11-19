// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "common/types.h"

namespace Libraries::Network {

bool IsPsnHost(std::string_view host);
std::string_view UrlHost(std::string_view url);
std::string_view UrlOrigin(std::string_view url);

struct FakeResponse {
    s32 status_code;
    std::string content_type;
    std::string body;
};

// A class for faking PSN requests, using shadNet where ideal.
class FakePsn {
public:
    static FakePsn& Instance();

    // Redirects a request to shadNet's URL.
    std::optional<std::string> Redirect(std::string_view url) const;

    // Return a fake response for PSN requests that can't redirect to shadNet.
    std::optional<FakeResponse> Respond(std::string_view method, std::string_view url,
                                        std::span<const u8> body) const;

private:
    // Handles asm.<env>.community.playstation.net requests
    std::optional<FakeResponse> AsmRequest(std::string_view method, std::string_view path) const;

    // Handles static-resource.<env>.community.playstation.net requests
    std::optional<FakeResponse> StaticResourceRequest(std::string_view method,
                                                      std::string_view path) const;

    // Handles filter.<region>.<env>.community.playstation.net requests
    std::optional<FakeResponse> WordFilterRequest(std::string_view method, std::string_view path,
                                                  std::span<const u8> body) const;
};

class FakeRequest {
public:
    explicit FakeRequest(FakeResponse response);

    s32 GetStatusCode() const {
        return m_response.status_code;
    }
    u64 GetContentLength() const {
        return m_response.body.size();
    }
    const std::string& GetHeaders() const {
        return m_headers;
    }

    u64 Read(void* data, u64 size);

private:
    FakeResponse m_response;
    std::string m_headers;
    std::mutex m_mutex;
    u64 m_offset = 0;
};

} // namespace Libraries::Network
