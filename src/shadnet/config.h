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
    Setting<bool> shadnet_enabled{true};
    Setting<bool> shadnet_enable_upnp{true};
    Setting<bool> shadnet_appear_offline{false};
    Setting<std::string> shadnet_server{"srv.shadps4.net:31313"};
    Setting<std::string> shadnet_webapi_server{"http://srv.shadps4.net:31315"};
    Setting<std::string> shadnet_signaling_info{""};
    Setting<std::string> shadnet_npid{""};
    Setting<std::string> shadnet_password{""};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ServerSettings, shadnet_enabled, shadnet_enable_upnp,
                                   shadnet_appear_offline, shadnet_server, shadnet_webapi_server,
                                   shadnet_signaling_info, shadnet_npid, shadnet_password)

class Settings {
public:
    static Settings& GetInstance();

    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    void Initialize();

    bool IsShadNetEnabled() {
        return m_server.shadnet_enabled.get();
    };

    bool IsUpnpEnabled() {
        return m_server.shadnet_enable_upnp.get();
    };

    bool IsAppearOfflineEnabled() {
        return m_server.shadnet_appear_offline.get();
    }

    std::string GetServerUrl() {
        return m_server.shadnet_server.get();
    };

    std::string GetWebApiServerUrl() {
        return m_server.shadnet_webapi_server.get();
    };

    std::string GetSignalingInfo() {
        return m_server.shadnet_signaling_info.get();
    };

    std::string GetNpId() {
        return m_server.shadnet_npid.get();
    };

    std::string GetPassword() {
        return m_server.shadnet_password.get();
    };

private:
    Settings() = default;
    ~Settings() = default;

    void InitialSetup();

    ServerSettings m_server{};
};

} // namespace ShadNet