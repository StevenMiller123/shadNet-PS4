// SPDX-FileCopyrightText: Copyright 2019-2026 rpcs3 Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <nlohmann/json.hpp>
#include <orbis/Http.h>
#include <orbis/Net.h>
#include <orbis/Ssl.h>

#include "common/logging/log.h"
#include "core/libraries/invitation_dialog/invitation_dialog.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/system_service/system_service.h"
#include "shadnet/config.h"
#include "shadnet/dialogs/invitation_prompt.h"

namespace Libraries::Np {

static constexpr u64 HTTP_NET_POOL_SIZE = 16 * 1024;
static constexpr u64 HTTP_SSL_POOL_SIZE = 256 * 1024;
static constexpr u64 HTTP_POOL_SIZE = 256 * 1024;
static constexpr u32 HTTP_CONNECT_TIMEOUT = 5000000;

bool NpHandler::SendSessionInvitation(s32 user_id, const std::string& session_id,
                                      const std::vector<std::string>& to,
                                      const std::string& message) {
    if (session_id.empty() || to.empty()) {
        LOG_ERROR(NpHandler, "empty session_id or recipient list");
        return false;
    }
    const std::string base_url = ShadNet::Settings::GetInstance().GetWebApiServerUrl();
    if (base_url.empty()) {
        LOG_ERROR(NpHandler, "WebAPI server address is empty");
        return false;
    }
    const std::string token = GetBearerToken(user_id);
    if (token.empty()) {
        LOG_ERROR(NpHandler, "no bearer token for user_id={}", user_id);
        return false;
    }

    nlohmann::json json{{"to", to}};
    if (!message.empty()) {
        json["message"] = message;
    }

    const std::string boundary = "shadps4invite" + std::to_string(user_id);
    std::string body;
    body += "--" + boundary + "\r\n";
    body += "Content-Type: application/json; charset=utf-8\r\n";
    body += "Content-Description: invitation-request\r\n\r\n";
    body += json.dump();
    body += "\r\n--" + boundary + "--\r\n";

    const std::string url = base_url + "/v1/sessions/" + session_id + "/invitations";
    const std::string content_type = "multipart/mixed; boundary=" + boundary;
    const std::string authorization = "Bearer " + token;

    s32 net_pool_id = sceNetPoolCreate("shadNet Http Pool", HTTP_NET_POOL_SIZE, 0);
    if (net_pool_id < 0) {
        LOG_ERROR(NpHandler, "sceNetPoolCreate failed with error {:#x}",
                  static_cast<u32>(net_pool_id));
        return false;
    }
    s32 ssl_ctx_id = sceSslInit(HTTP_SSL_POOL_SIZE);
    if (ssl_ctx_id < 0) {
        LOG_ERROR(NpHandler, "sceSslInit failed with error {:#x}", static_cast<u32>(ssl_ctx_id));
        sceNetPoolDestroy(net_pool_id);
        return false;
    }
    s32 http_ctx_id = sceHttpInit(net_pool_id, ssl_ctx_id, HTTP_POOL_SIZE);
    if (http_ctx_id < 0) {
        LOG_ERROR(NpHandler, "sceHttpInit failed with error {:#x}", static_cast<u32>(http_ctx_id));
        sceSslTerm(ssl_ctx_id);
        sceNetPoolDestroy(net_pool_id);
        return false;
    }

    const s32 tmpl_id = sceHttpCreateTemplate(http_ctx_id, "shadNet", 2, 0);
    if (tmpl_id < 0) {
        LOG_ERROR(NpHandler, "sceHttpCreateTemplate failed with error {:#x}",
                  static_cast<u32>(tmpl_id));
        sceHttpTerm(http_ctx_id);
        sceSslTerm(ssl_ctx_id);
        sceNetPoolDestroy(net_pool_id);
        return false;
    }
    const s32 conn_id = sceHttpCreateConnectionWithURL(tmpl_id, base_url.c_str(), true);
    if (conn_id < 0) {
        LOG_ERROR(NpHandler, "sceHttpCreateConnectionWithURL failed with error {:#x}",
                  static_cast<u32>(conn_id));
        sceHttpDeleteTemplate(tmpl_id);
        sceHttpTerm(http_ctx_id);
        sceSslTerm(ssl_ctx_id);
        sceNetPoolDestroy(net_pool_id);
        return false;
    }
    const s32 req_id = sceHttpCreateRequestWithURL(conn_id, 1, url.c_str(), body.size());
    if (req_id < 0) {
        LOG_ERROR(NpHandler, "sceHttpCreateRequestWithURL failed with error {:#x}",
                  static_cast<u32>(req_id));
        sceHttpDeleteConnection(conn_id);
        sceHttpDeleteTemplate(tmpl_id);
        sceHttpTerm(http_ctx_id);
        sceSslTerm(ssl_ctx_id);
        sceNetPoolDestroy(net_pool_id);
        return false;
    }

    sceHttpSetConnectTimeOut(req_id, HTTP_CONNECT_TIMEOUT);
    sceHttpAddRequestHeader(req_id, "Content-Type", content_type.c_str(), 0);
    sceHttpAddRequestHeader(req_id, "Authorization", authorization.c_str(), 0);

    bool success = false;
    s32 result = sceHttpSendRequest(req_id, body.data(), body.size());
    if (result < 0) {
        LOG_ERROR(NpHandler, "sceHttpSendRequest failed with error {:#x}",
                  static_cast<u32>(result));
    } else {
        s32 status = 0;
        result = sceHttpGetStatusCode(req_id, &status);
        if (result < 0) {
            LOG_ERROR(NpHandler, "sceHttpGetStatusCode failed with error {:#x}",
                      static_cast<u32>(result));
        } else if (status != 200 && status != 204) {
            LOG_ERROR(NpHandler, "POST {} -> HTTP {}", url, status);
        } else {
            LOG_INFO(NpHandler, "sent invite to {} recipient(s) for session '{}'", to.size(),
                     session_id);
            success = true;
        }
    }

    sceHttpDeleteRequest(req_id);
    sceHttpDeleteConnection(conn_id);
    sceHttpDeleteTemplate(tmpl_id);
    sceHttpTerm(http_ctx_id);
    sceSslTerm(ssl_ctx_id);
    sceNetPoolDestroy(net_pool_id);
    return success;
}

void NpHandler::PostSessionInvitationEvent(s32 user_id, const std::string& session_id,
                                           const std::string& invitation_id,
                                           const std::string& accepter_online_id,
                                           const std::string& inviter_online_id,
                                           OrbisNpAccountId inviter_account_id) {
    SystemService::OrbisSystemServiceEvent event{};
    event.event_type = SystemService::OrbisSystemServiceEventType::SessionInvitation;

    auto* param = reinterpret_cast<OrbisNpSessionInvitationEventParam*>(event.param);
    std::strncpy(param->session_id.data, session_id.c_str(), sizeof(param->session_id.data) - 1);
    if (!invitation_id.empty()) {
        std::strncpy(param->invitation_id.data, invitation_id.c_str(),
                     sizeof(param->invitation_id.data) - 1);
        param->flag = ORBIS_NP_SESSION_INVITATION_EVENT_FLAG_INVITATION;
    }
    std::strncpy(param->online_id.data, accepter_online_id.c_str(), sizeof(param->online_id.data));
    param->user_id = user_id;
    std::strncpy(param->referral_online_id.data, inviter_online_id.c_str(),
                 sizeof(param->referral_online_id.data));
    param->referral_account_id = inviter_account_id;

    SystemService::PushSystemServiceEvent(event);
    LOG_INFO(NpHandler,
             "posted SESSION_INVITATION user_id={} session='{}' invitation='{}' flag={} "
             "online_id='{}' referral='{}'({})",
             user_id, session_id, invitation_id, param->flag, accepter_online_id, inviter_online_id,
             inviter_account_id);
}

std::vector<NpHandler::PendingInvitation> NpHandler::GetPendingInvitations(s32 user_id) const {
    std::lock_guard lk(m_mutex_pending_invites);
    const auto it = m_pending_invites.find(user_id);
    if (it == m_pending_invites.end()) {
        return {};
    }
    const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
    std::vector<PendingInvitation> out;
    out.reserve(it->second.size());
    for (const auto& p : it->second) {
        if (p.valid_until == 0 || now <= p.valid_until) {
            out.push_back(p);
        }
    }
    return out;
}

bool NpHandler::AcceptSessionInvitation(s32 user_id, const std::string& invitation_id) {
    PendingInvitation inv;
    bool found = false;
    {
        std::lock_guard lk(m_mutex_pending_invites);
        const auto mit = m_pending_invites.find(user_id);
        if (mit != m_pending_invites.end()) {
            auto& v = mit->second;
            for (auto it = v.begin(); it != v.end(); ++it) {
                if (it->invitation_id == invitation_id) {
                    inv = *it;
                    v.erase(it);
                    found = true;
                    break;
                }
            }
        }
    }
    ShadNet::InvitationPrompt::GetInstance().Dismiss(invitation_id);
    if (!found) {
        LOG_ERROR(NpHandler, "no pending invite '{}' for user_id={}", invitation_id, user_id);
        return false;
    }
    PostSessionInvitationEvent(user_id, inv.session_id, invitation_id, inv.to_npid, inv.from_npid,
                               inv.from_account_id);
    LOG_INFO(NpHandler, "accepted '{}' session='{}'", invitation_id, inv.session_id);
    return true;
}

void NpHandler::DeclineSessionInvitation(s32 user_id, const std::string& invitation_id) {
    ShadNet::InvitationPrompt::GetInstance().Dismiss(invitation_id);
    std::lock_guard lock(m_mutex_pending_invites);
    auto uit = m_pending_invites.find(user_id);
    if (uit == m_pending_invites.end()) {
        return;
    }
    auto& v = uit->second;
    v.erase(std::remove_if(
                v.begin(), v.end(),
                [&](const PendingInvitation& p) { return p.invitation_id == invitation_id; }),
            v.end());
    LOG_INFO(NpHandler, "dismissed '{}' for user_id={}", invitation_id, user_id);
}

} // namespace Libraries::Np
