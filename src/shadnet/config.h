// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <nlohmann/json.hpp>
#include "common/types.h"

namespace ShadNet {
template <typename T>
struct Setting {
    T default_value{};
    T value{};

    Setting() = default;
    Setting(T init) : default_value(std::move(init)), value(default_value) {}

    T get() const {
        return value;
    }

    void set(const T& v) {
        value = v;
    }
};

template <typename T>
void to_json(nlohmann::json& j, const Setting<T>& s) {
    j = s.value;
}

template <typename T>
void from_json(const nlohmann::json& j, Setting<T>& s) {
    s.value = j.get<T>();
}

struct ServerSettings {
    Setting<bool> shadnet_enable_upnp{true};
    Setting<s32> shadnet_p2p_port{3658};
    Setting<std::string> shadnet_server{"srv.shadps4.net:31313"};
    Setting<std::string> shadnet_webapi_server{"http://srv.shadps4.net:31315"};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ServerSettings, shadnet_enable_upnp, shadnet_p2p_port,
                                   shadnet_server, shadnet_webapi_server)

struct UserSettings {
    Setting<bool> shadnet_appear_offline{false};
    Setting<bool> shadnet_enabled{false};
    Setting<std::string> shadnet_npid{""};
    Setting<std::string> shadnet_password{""};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserSettings, shadnet_appear_offline, shadnet_enabled,
                                   shadnet_npid, shadnet_password)

class Settings {
public:
    static Settings& GetInstance();

    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    void Initialize();
    bool Save(s32 user_id, const UserSettings& user, const ServerSettings& server);

    bool IsUpnpEnabled() {
        return m_server.shadnet_enable_upnp.get();
    };

    s32 GetP2PPort() {
        return m_server.shadnet_p2p_port.get();
    };

    std::string GetServerUrl() {
        return m_server.shadnet_server.get();
    };

    std::string GetWebApiServerUrl() {
        return m_server.shadnet_webapi_server.get();
    };

    bool IsAppearOfflineEnabled(s32 user_id) {
        return m_user[user_id].shadnet_appear_offline.get();
    }

    bool IsShadNetEnabled(s32 user_id) {
        return m_user[user_id].shadnet_enabled.get();
    };

    std::string GetNpId(s32 user_id) {
        return m_user[user_id].shadnet_npid.get();
    };

    std::string GetPassword(s32 user_id) {
        return m_user[user_id].shadnet_password.get();
    };

    bool IsFirstRun() {
        return m_first_run;
    }

    UserSettings GetUserSettings(s32 user_id) {
        auto it = m_user.find(user_id);
        return it != m_user.end() ? it->second : UserSettings{};
    }

    ServerSettings GetServerSettings() {
        return m_server;
    }

private:
    Settings() = default;
    ~Settings() = default;

    void InitialSetup();

    ServerSettings m_server{};
    std::map<s32, UserSettings> m_user{};
    bool m_first_run{};
};

} // namespace ShadNet