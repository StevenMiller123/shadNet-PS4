// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include <random>
#include <nlohmann/json.hpp>
#include <orbis/Http.h>
#include <orbis/Net.h>
#include <orbis/Ssl.h>

#include "common/logging/log.h"
#include "core/libraries/game_custom_data_dialog/game_custom_data_dialog.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/system_service/system_service.h"
#include "shadnet/config.h"

namespace Libraries::Np {

static constexpr u64 HTTP_NET_POOL_SIZE = 16 * 1024;
static constexpr u64 HTTP_SSL_POOL_SIZE = 256 * 1024;
static constexpr u64 HTTP_POOL_SIZE = 256 * 1024;
static constexpr u32 HTTP_CONNECT_TIMEOUT = 5000000;
static constexpr s32 HTTP_METHOD_GET = 0;
static constexpr s32 HTTP_METHOD_POST = 1;
static constexpr u32 HTTP_READ_CHUNK = 16 * 1024;
static constexpr u64 MESSAGE_LOOKUP_BATCH = 20;
static constexpr const char* DEFAULT_LANGUAGE = "en";

using json = nlohmann::json;
using Headers = std::vector<std::pair<std::string, std::string>>;

static std::string Dump(const json& j) {
    return j.dump(-1, ' ', false, json::error_handler_t::replace);
}

static std::string Boundary() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    return fmt::format("shadps4gcd{:016x}{:016x}", gen(), gen());
}

static s32 WebApiRequest(s32 method, const std::string& base_url, const std::string& path,
                         const Headers& headers, const std::string& body, std::string* response) {
    const s32 net_pool_id = sceNetPoolCreate("shadNet Http Pool", HTTP_NET_POOL_SIZE, 0);
    if (net_pool_id < 0) {
        LOG_ERROR(NpHandler, "sceNetPoolCreate failed with error {:#x}",
                  static_cast<u32>(net_pool_id));
        return 0;
    }
    const s32 ssl_ctx_id = sceSslInit(HTTP_SSL_POOL_SIZE);
    const s32 http_ctx_id =
        ssl_ctx_id < 0 ? ssl_ctx_id : sceHttpInit(net_pool_id, ssl_ctx_id, HTTP_POOL_SIZE);
    const s32 tmpl_id =
        http_ctx_id < 0 ? http_ctx_id : sceHttpCreateTemplate(http_ctx_id, "shadNet", 2, 0);
    const s32 conn_id =
        tmpl_id < 0 ? tmpl_id : sceHttpCreateConnectionWithURL(tmpl_id, base_url.c_str(), true);
    const std::string url = base_url + path;
    const s32 req_id = conn_id < 0
                           ? conn_id
                           : sceHttpCreateRequestWithURL(conn_id, method, url.c_str(), body.size());

    s32 status = 0;
    if (req_id < 0) {
        LOG_ERROR(NpHandler, "http setup for {} failed with error {:#x}", url,
                  static_cast<u32>(req_id));
    } else {
        sceHttpSetConnectTimeOut(req_id, HTTP_CONNECT_TIMEOUT);
        for (const auto& [name, value] : headers) {
            sceHttpAddRequestHeader(req_id, name.c_str(), value.c_str(), 0);
        }
        s32 result = sceHttpSendRequest(req_id, body.empty() ? nullptr : body.data(), body.size());
        if (result >= 0) {
            result = sceHttpGetStatusCode(req_id, &status);
        }
        if (result < 0) {
            LOG_ERROR(NpHandler, "request to {} failed with error {:#x}", url,
                      static_cast<u32>(result));
            status = 0;
        } else if (response != nullptr) {
            response->clear();
            std::vector<char> chunk(HTTP_READ_CHUNK);
            s32 read = 0;
            while ((read = sceHttpReadData(req_id, chunk.data(), chunk.size())) > 0) {
                response->append(chunk.data(), read);
            }
        }
    }

    if (req_id >= 0) {
        sceHttpDeleteRequest(req_id);
    }
    if (conn_id >= 0) {
        sceHttpDeleteConnection(conn_id);
    }
    if (tmpl_id >= 0) {
        sceHttpDeleteTemplate(tmpl_id);
    }
    if (http_ctx_id >= 0) {
        sceHttpTerm(http_ctx_id);
    }
    if (ssl_ctx_id >= 0) {
        sceSslTerm(ssl_ctx_id);
    }
    sceNetPoolDestroy(net_pool_id);
    return status;
}

static void ReadError(s32 status, const std::string& response,
                      NpHandler::GameCustomDataResult& out) {
    out.http_status = status;
    if (status == 0) {
        out.message = "no response from shadNet";
        return;
    }
    const json body = json::parse(response, nullptr, false);
    if (body.is_object() && body.contains("error") && body["error"].is_object()) {
        out.error_code = body["error"].value("code", static_cast<s64>(0));
        out.message = body["error"].value("message", std::string());
    }
    if (out.message.empty()) {
        out.message = "HTTP " + std::to_string(status);
    }
}

NpHandler::GameCustomDataResult NpHandler::SendGameCustomData(s32 user_id,
                                                              const GameCustomDataToSend& item) {
    GameCustomDataResult result;
    const std::string base_url = ShadNet::Settings::GetInstance().GetWebApiServerUrl();
    const std::string token = GetBearerToken(user_id);
    if (base_url.empty() || token.empty()) {
        result.message = "not signed in to shadNet";
        LOG_ERROR(NpHandler, "{} (user_id={})", result.message, user_id);
        return result;
    }

    json detail = {{"dataName", item.data_name},
                   {"dataDescription", item.data_description},
                   {"expireMinutes", item.expire_minutes},
                   {"availablePlatforms", item.platforms}};
    std::string languages;
    const auto localized = [&](const char* member, const char* key, const auto& list) {
        if (list.empty()) {
            return;
        }
        json arr = json::array();
        for (const auto& [lang, text] : list) {
            arr.push_back({{"npLanguage", lang}, {key, text}});
            if (languages.find(lang) == std::string::npos) {
                languages += (languages.empty() ? "" : ",") + lang;
            }
        }
        detail[member] = std::move(arr);
    };
    localized("localizedDataNames", "dataName", item.localized_names);
    localized("localizedDataDescriptions", "dataDescription", item.localized_descriptions);
    json body_item = {{"message", item.message}, {"gameDataDetail", std::move(detail)}};
    if (item.is_url) {
        body_item["attachmentDetail"] = {
            {"type", "url"}, {"value", item.url}, {"withItemId", item.with_item_id}};
    }
    const std::string message = Dump({{"toAccountIds", item.to}, {"item", std::move(body_item)}});

    const std::string boundary = Boundary();
    std::string body;
    const auto part = [&](const char* type, const char* description, const void* data, u64 size,
                          bool attachment) {
        body += "--" + boundary + "\r\n";
        body += std::string("Content-Type:") + type + "\r\n";
        body += std::string("Content-Description:") + description + "\r\n";
        if (attachment) {
            body += "Content-Disposition:attachment\r\n";
        }
        body += "Content-Length:" + std::to_string(size) + "\r\n\r\n";
        body.append(static_cast<const char*>(data), size);
        body += "\r\n";
    };
    part("application/json; charset=utf-8", "message", message.data(), message.size(), false);
    if (!item.is_url) {
        part("application/octet-stream", "game-data-binary-0", item.game_data.data(),
             item.game_data.size(), true);
    }
    const bool png =
        item.thumbnail.size() >= 4 && std::memcmp(item.thumbnail.data(), "\x89PNG", 4) == 0;
    part(png ? "image/png" : "image/jpeg", "game-data-thumbnail-0", item.thumbnail.data(),
         item.thumbnail.size(), true);
    body += "--" + boundary + "--\r\n";

    Headers headers = {{"Authorization", "Bearer " + token},
                       {"Content-Type", "multipart/mixed; boundary=" + boundary}};
    if (!languages.empty()) {
        headers.emplace_back("Accept-Language", languages);
    }
    std::string response;
    const s32 status = WebApiRequest(HTTP_METHOD_POST, base_url, "/v1/gameCustomData/items",
                                     headers, body, &response);
    if (status != 200 && status != 204) {
        ReadError(status, response, result);
        LOG_ERROR(NpHandler, "send failed: HTTP {} code {} '{}'", result.http_status,
                  result.error_code, result.message);
        return result;
    }
    result.ok = true;
    result.http_status = status;
    LOG_INFO(NpHandler, "sent '{}' to {} recipient(s)", item.data_name, item.to.size());
    return result;
}

NpHandler::GameCustomDataResult NpHandler::ListReceivedGameCustomData(
    s32 user_id, const std::string& np_language, std::vector<ReceivedGameCustomData>& out) {
    GameCustomDataResult result;
    out.clear();
    const std::string base_url = ShadNet::Settings::GetInstance().GetWebApiServerUrl();
    const std::string token = GetBearerToken(user_id);
    if (base_url.empty() || token.empty()) {
        result.message = "not signed in to shadNet";
        return result;
    }
    const std::string lang = np_language.empty() ? DEFAULT_LANGUAGE : np_language;
    const Headers headers = {{"Authorization", "Bearer " + token}, {"Accept-Language", lang}};

    std::string response;
    const s32 status = WebApiRequest(HTTP_METHOD_GET, base_url,
                                     "/v1/users/me/gameCustomData/items?fields=@default,fromUser,"
                                     "attachmentDetail,gameDataDetail&limit=100",
                                     headers, {}, &response);
    if (status != 200) {
        ReadError(status, response, result);
        LOG_ERROR(NpHandler, "list failed: HTTP {} '{}'", result.http_status, result.message);
        return result;
    }
    const json list = json::parse(response, nullptr, false);
    const json items = list.is_object() ? list.value("items", json::array()) : json::array();
    for (const json& j : items) {
        ReceivedGameCustomData r;
        r.item_id = j.value("itemId", static_cast<s64>(0));
        if (r.item_id == 0) {
            continue;
        }
        r.received_date = j.value("receivedDate", std::string());
        r.used = j.value("dataUsedFlag", false);
        if (j.contains("fromUser")) {
            r.from_online_id = j["fromUser"].value("onlineId", std::string());
            r.from_account_id = std::strtoull(
                j["fromUser"].value("accountId", std::string("0")).c_str(), nullptr, 10);
        }
        if (j.contains("attachmentDetail")) {
            r.is_url = j["attachmentDetail"].value("type", std::string()) == "url";
            r.url = j["attachmentDetail"].value("value", std::string());
        }
        if (j.contains("gameDataDetail")) {
            r.expired_date = j["gameDataDetail"].value("expiredDate", std::string());
            r.platforms =
                j["gameDataDetail"].value("availablePlatforms", std::vector<std::string>());
        }
        out.push_back(std::move(r));
    }

    for (u64 start = 0; start < out.size(); start += MESSAGE_LOOKUP_BATCH) {
        std::string ids;
        for (u64 i = start; i < std::min<u64>(out.size(), start + MESSAGE_LOOKUP_BATCH); i++) {
            ids += (ids.empty() ? "" : ",") + std::to_string(out[i].item_id);
        }
        const s32 msg_status =
            WebApiRequest(HTTP_METHOD_GET, base_url,
                          "/v1/users/me/gameCustomData/items/@items?itemIds=" + ids +
                              "&fields=@default,message,gameDataDetail&npLanguage=" + lang,
                          headers, {}, &response);
        if (msg_status != 200) {
            LOG_WARNING(NpHandler, "message lookup failed (HTTP {})", msg_status);
            continue;
        }
        const json messages = json::parse(response, nullptr, false);
        if (!messages.is_object()) {
            continue;
        }
        for (const json& j : messages.value("items", json::array())) {
            const s64 id = j.value("itemId", static_cast<s64>(0));
            for (auto& r : out) {
                if (r.item_id != id) {
                    continue;
                }
                r.message = j.value("message", std::string());
                if (j.contains("gameDataDetail")) {
                    r.data_name = j["gameDataDetail"].value("dataName", std::string());
                    r.data_description =
                        j["gameDataDetail"].value("dataDescription", std::string());
                }
            }
        }
    }

    result.ok = true;
    result.http_status = 200;
    LOG_INFO(NpHandler, "{} received item(s) for user_id={}", out.size(), user_id);
    return result;
}

void NpHandler::PostGameCustomDataEvent(s32 user_id, s64 item_id) {
    using GameCustomDataDialog::OrbisNpGameCustomDataEventParam;

    SystemService::OrbisSystemServiceEvent event{};
    event.event_type = SystemService::OrbisSystemServiceEventType::GameCustomData;
    auto* param = reinterpret_cast<OrbisNpGameCustomDataEventParam*>(event.param);
    std::memset(param, 0, sizeof(*param));
    param->item_id = static_cast<GameCustomDataDialog::OrbisNpGameCustomDataId>(item_id);
    param->online_id = GetNpId(user_id).handle;
    param->user_id = user_id;
    SystemService::PushSystemServiceEvent(event);
    LOG_INFO(NpHandler, "user_id={} item_id={}", user_id, item_id);
}

} // namespace Libraries::Np
