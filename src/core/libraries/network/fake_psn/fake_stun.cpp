// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include <string>
#include <orbis/Net.h>
#include <orbis/NetCtl.h>
#include <orbis/UserService.h>
#include <sys/socket.h>

#include "common/endian.h"
#include "common/logging/log.h"
#include "core/libraries/network/fake_psn/fake_psn.h"
#include "core/libraries/network/fake_psn/fake_signaling.h"
#include "core/libraries/network/fake_psn/fake_stun.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "shadnet/config.h"

namespace Libraries::Network {

namespace {

// libSceNpSignaling resolves <country>.np.stun.playstation.net.
constexpr std::string_view StunHostSuffix = "stun.playstation.net";

// RFC 3489 message and attribute types.
constexpr u16 BindingRequest = 0x0001;
constexpr u16 BindingResponse = 0x0101;
constexpr u16 MappedAddressAttribute = 0x0001;
constexpr u16 ChangeRequestAttribute = 0x0003;
constexpr u16 SourceAddressAttribute = 0x0004;
constexpr u16 ChangedAddressAttribute = 0x0005;
constexpr u16 SonyAttribute = 0x8099;

constexpr u8 AddressFamilyIpv4 = 0x01;

constexpr u32 ChangeIp = 0x4;
constexpr u32 ChangePort = 0x2;

constexpr u32 LoopbackAddr = 0x7f000001;

constexpr s32 NatType1 = 1;
constexpr s32 NatType2 = 2;

#pragma pack(push, 1)
struct StunHeader {
    u16_be type;
    u16_be length;
    std::array<u8, 16> transaction_id;
};
static_assert(sizeof(StunHeader) == 20);

struct StunAttributeHeader {
    u16_be type;
    u16_be length;
};
static_assert(sizeof(StunAttributeHeader) == 4);

struct StunAddressAttribute {
    StunAttributeHeader header;
    u8 reserved;
    u8 family;
    u16 port;
    u32 addr;
};
static_assert(sizeof(StunAddressAttribute) == 12);

// The attributes libSceNpSignaling requires in a response.
struct StunBindingResponse {
    StunHeader header;
    StunAddressAttribute mapped_address;
    StunAddressAttribute source_address;
    StunAddressAttribute changed_address;
};
static_assert(sizeof(StunBindingResponse) == 56);

// shadNet only takes STUN pings from a P2P datagram on vport 0xFFFF
constexpr u8 SignalingMagic = 0xFF;
constexpr u8 SignalingFlags = 0x83;
constexpr u16 SignalingVport = 0xFFFF;

struct ShadNetSignalingHeader {
    u8 magic;
    u8 flags;
    u16_be src_vport;
    u16_be dst_vport;

    bool IsValid() const {
        return magic == SignalingMagic && flags == SignalingFlags && src_vport == SignalingVport &&
               dst_vport == SignalingVport;
    }
};
static_assert(sizeof(ShadNetSignalingHeader) == 6);

struct ShadNetStunPing {
    u8 cmd = 0x01;
    char online_id[Np::ORBIS_NP_ONLINEID_MAX_LENGTH]{};
    u32 local_addr = 0;
};
static_assert(sizeof(ShadNetStunPing) == 21);

struct ShadNetStunEcho {
    u32 external_addr;
    u16 external_port;
};
static_assert(sizeof(ShadNetStunEcho) == 6);

struct FramedShadNetStunEcho {
    ShadNetSignalingHeader header;
    ShadNetStunEcho echo;
};
#pragma pack(pop)

StunAddressAttribute MakeAddressAttribute(u16 type, u32 addr, u16 port) {
    StunAddressAttribute attribute{};
    attribute.header.type = type;
    attribute.header.length = sizeof(StunAddressAttribute) - sizeof(StunAttributeHeader);
    attribute.family = AddressFamilyIpv4;
    attribute.port = port;
    attribute.addr = addr;
    return attribute;
}

// Splits "[scheme://]host[:port][/path]".
bool ParseHostPort(std::string_view url, std::string& host, u16& port) {
    if (const u64 scheme_end = url.find("://"); scheme_end != std::string_view::npos) {
        url.remove_prefix(scheme_end + 3);
    }
    url = url.substr(0, url.find('/'));
    const u64 colon = url.rfind(':');
    host = std::string(url.substr(0, colon));
    port = 0;
    if (colon != std::string_view::npos) {
        try {
            port = static_cast<u16>(std::stoi(std::string(url.substr(colon + 1))));
        } catch (const std::exception&) {
            return false;
        }
    }
    return !host.empty();
}

// Resolves a host to an IPv4 address in network byte order, 0 on failure.
u32 ResolveHost(const std::string& host) {
    const s32 pool = sceNetPoolCreate("FakeStun Pool", 0x1000, 0);
    if (pool < 0) {
        return 0;
    }
    u32 addr = 0;
    const s32 resolver = sceNetResolverCreate("FakeStun Resolver", pool, 0);
    if (resolver >= 0) {
        OrbisNetInAddr in_addr{};
        if (sceNetResolverStartNtoa(resolver, host.c_str(), &in_addr, 0, 0, 0) == 0) {
            addr = in_addr.s_addr;
        }
        sceNetResolverDestroy(resolver);
    }
    sceNetPoolDestroy(pool);
    return addr;
}

// The online id of the first signed in shadNet user, which shadNet keys the address by.
Np::OrbisNpOnlineId SignedInOnlineId() {
    OrbisUserServiceLoginUserIdList users{};
    if (sceUserServiceGetLoginUserIdList(&users) == 0) {
        for (const s32 user_id : users.userId) {
            if (user_id != ORBIS_USER_SERVICE_USER_ID_INVALID &&
                Np::NpHandler::Instance().IsSignedIn(user_id)) {
                return Np::NpHandler::Instance().GetNpId(user_id).handle;
            }
        }
    }
    return {};
}

s32 SocketType(s32 socket) {
    s32 type = 0;
    u32 size = sizeof(type);
    if (sceNetGetsockopt(socket, SOL_SOCKET, SO_TYPE, &type, &size) < 0) {
        return -1;
    }
    return type;
}

u32 LocalAddr() {
    OrbisNetCtlInfo info{};
    u32 addr = 0;
    if (sceNetCtlGetInfo(ORBIS_NET_CTL_INFO_IP_ADDRESS, &info) == 0) {
        sceNetInetPton(ORBIS_NET_AF_INET, info.ip_address, &addr);
    }
    return addr;
}

} // namespace

FakeStun& FakeStun::Instance() {
    static FakeStun instance;
    return instance;
}

bool FakeStun::IsStunHost(std::string_view host) {
    while (host.ends_with('.')) {
        host.remove_suffix(1);
    }
    return IsPsnHost(host) && host.ends_with(StunHostSuffix);
}

u32 FakeStun::GetServerAddr() {
    std::scoped_lock lk{m_mutex};
    return ResolveServers() ? m_server_addr : 0;
}

bool FakeStun::ResolveServers() {
    if (m_resolved) {
        return m_server_addr != 0 && m_echo_addr != 0;
    }
    m_resolved = true;

    auto& config = ShadNet::Settings::GetInstance();
    std::string host;
    u16 port = 0;
    if (ParseHostPort(config.GetWebApiServerUrl(), host, port)) {
        m_server_addr = ResolveHost(host);
    }
    if (ParseHostPort(config.GetServerUrl(), host, port) && port != 0) {
        m_echo_addr = ResolveHost(host);
        m_echo_port = sceNetHtons(static_cast<u16>(port + 1));
    }
    if (m_server_addr == 0 || m_echo_addr == 0) {
        LOG_ERROR(Lib_Net, "couldn't resolve the shadnet servers for stun");
        return false;
    }
    return true;
}

bool FakeStun::PassesNat(u32 change_flags) {
    OrbisNetCtlNatInfo info{};
    info.size = sizeof(info);
    // Without a known NAT type, assume the common type 2.
    s32 nat_type = NatType2;
    if (sceNetCtlGetNatInfo(&info) == 0 && info.nat_type >= NatType1) {
        nat_type = info.nat_type;
    }

    if ((change_flags & ChangeIp) != 0) {
        return nat_type == NatType1;
    }
    if ((change_flags & ChangePort) != 0) {
        return nat_type <= NatType2;
    }
    return true;
}

std::optional<s32> FakeStun::OnSendTo(s32 socket, const void* data, u64 size,
                                      const OrbisNetSockaddr* to, u32 to_size) {
    if (data == nullptr || to == nullptr || to_size < sizeof(OrbisNetSockaddrIn) ||
        to->sa_family != ORBIS_NET_AF_INET || size < sizeof(StunHeader)) {
        return std::nullopt;
    }
    const auto* to_in = reinterpret_cast<const OrbisNetSockaddrIn*>(to);
    const u16 to_port = sceNetNtohs(to_in->sin_port);
    if (to_port != StunPort && to_port != ChangedPort) {
        return std::nullopt;
    }

    const auto* bytes = static_cast<const u8*>(data);
    StunHeader header{};
    std::memcpy(&header, bytes, sizeof(header));
    const u64 message_size = sizeof(StunHeader) + header.length;
    if (header.type != BindingRequest || message_size > size) {
        return std::nullopt;
    }

    std::unique_lock lk{m_mutex};
    if (!ResolveServers() || to_in->sin_addr != m_server_addr) {
        return std::nullopt;
    }

    PendingRequest request{};
    request.transaction_id = header.transaction_id;
    u32 change_flags = 0;
    for (u64 offset = sizeof(StunHeader); offset + sizeof(StunAttributeHeader) <= message_size;) {
        StunAttributeHeader attribute{};
        std::memcpy(&attribute, bytes + offset, sizeof(attribute));
        const u8* value = bytes + offset + sizeof(attribute);
        if (offset + sizeof(attribute) + attribute.length > message_size) {
            break;
        }
        if (attribute.type == ChangeRequestAttribute && attribute.length == sizeof(u32_be)) {
            u32_be flags;
            std::memcpy(&flags, value, sizeof(flags));
            change_flags = flags;
        } else if (attribute.type == SonyAttribute) {
            request.sony_attribute.assign(value, value + attribute.length);
        }
        offset += sizeof(attribute) + attribute.length;
    }

    if (!PassesNat(change_flags)) {
        LOG_INFO(Lib_Net, "stun request with change flags {:#x} doesn't pass our nat, dropping",
                 change_flags);
        return static_cast<s32>(size);
    }
    request.source_port = sceNetHtons((change_flags & ChangePort) != 0 ? ChangedPort : StunPort);

    // shadNet peers reach the console at the address shadNet sees, which has to be the P2P port.
    // Use loopback to return data to the appropriate library's sockets.
    const s32 socket_type = SocketType(socket);
    request.forwarded = socket_type == ORBIS_NET_SOCK_DGRAM;
    m_pending[socket] = std::move(request);

    OrbisNetSockaddrIn echo{};
    echo.sin_len = sizeof(echo);
    echo.sin_family = ORBIS_NET_AF_INET;
    echo.sin_port = m_echo_port;
    echo.sin_addr = m_echo_addr;
    lk.unlock();

    // Fill shadNet's STUN data
    ShadNetStunPing ping{};
    const Np::OrbisNpOnlineId online_id = SignedInOnlineId();
    if (online_id.data[0] == '\0') {
        LOG_WARNING(Lib_Net, "no signed in shadnet user, shadnet will ignore the stun ping");
    }
    std::memcpy(ping.online_id, online_id.data, sizeof(ping.online_id));
    ping.local_addr = LocalAddr();

    s32 ret = 0;
    if (socket_type == ORBIS_NET_SOCK_DGRAM) {
        const auto* bytes = reinterpret_cast<const u8*>(&ping);
        ret = FakeSignaling::Instance().SendToShadNet({bytes, sizeof(ping)}, echo);
    } else {
        echo.sin_vport = sceNetHtons(SignalingVport);
        ret = sceNetSendto(socket, &ping, sizeof(ping), 0,
                           reinterpret_cast<const OrbisNetSockaddr*>(&echo), sizeof(echo));
    }
    LOG_DEBUG(Lib_Net,
              "stun request with change flags {:#x} sent to shadnet from socket type {}, ret {:#x}",
              change_flags, socket_type, ret);
    return ret < 0 ? ret : static_cast<s32>(size);
}

s32 FakeStun::OnRecvFrom(s32 socket, void* buf, u64 capacity, s32 size, OrbisNetSockaddr* from,
                         u32* from_size) {
    if (buf == nullptr || from == nullptr || from_size == nullptr ||
        *from_size < sizeof(OrbisNetSockaddrIn)) {
        return size;
    }

    // The echo keeps shadNet's P2P framing on plain UDP sockets, P2P sockets strip it.
    ShadNetStunEcho echo{};
    if (size == static_cast<s32>(sizeof(FramedShadNetStunEcho))) {
        FramedShadNetStunEcho framed{};
        std::memcpy(&framed, buf, sizeof(framed));
        if (!framed.header.IsValid()) {
            return size;
        }
        echo = framed.echo;
    } else if (size == static_cast<s32>(sizeof(ShadNetStunEcho))) {
        std::memcpy(&echo, buf, sizeof(echo));
    } else {
        return size;
    }
    auto* from_in = reinterpret_cast<OrbisNetSockaddrIn*>(from);

    std::scoped_lock lk{m_mutex};
    const auto it = m_pending.find(socket);
    if (it == m_pending.end()) {
        return size;
    }
    const bool from_echo = from_in->sin_addr == m_echo_addr && from_in->sin_port == m_echo_port;
    const bool from_loopback = it->second.forwarded && m_loopback_port != 0 &&
                               from_in->sin_addr == sceNetHtonl(LoopbackAddr) &&
                               from_in->sin_port == m_loopback_port;
    if (!from_echo && !from_loopback) {
        return size;
    }

    const PendingRequest& request = it->second;

    StunBindingResponse response{};
    response.header.type = BindingResponse;
    response.header.transaction_id = request.transaction_id;
    response.mapped_address =
        MakeAddressAttribute(MappedAddressAttribute, echo.external_addr, echo.external_port);
    response.source_address =
        MakeAddressAttribute(SourceAddressAttribute, m_server_addr, request.source_port);
    response.changed_address =
        MakeAddressAttribute(ChangedAddressAttribute, m_server_addr, sceNetHtons(ChangedPort));

    StunAttributeHeader sony_header{};
    u64 response_size = sizeof(response);
    if (!request.sony_attribute.empty()) {
        sony_header.type = SonyAttribute;
        sony_header.length = static_cast<u16>(request.sony_attribute.size());
        response_size += sizeof(sony_header) + request.sony_attribute.size();
    }
    response.header.length = static_cast<u16>(response_size - sizeof(StunHeader));

    if (response_size > capacity) {
        LOG_ERROR(Lib_Net, "stun response doesn't fit the receive buffer");
        return size;
    }
    auto* out = static_cast<u8*>(buf);
    std::memcpy(out, &response, sizeof(response));
    if (!request.sony_attribute.empty()) {
        std::memcpy(out + sizeof(response), &sony_header, sizeof(sony_header));
        std::memcpy(out + sizeof(response) + sizeof(sony_header), request.sony_attribute.data(),
                    request.sony_attribute.size());
    }

    // The response comes from our STUN server.
    from_in->sin_addr = m_server_addr;
    from_in->sin_port = request.source_port;
    from_in->sin_vport = 0;
    *from_size = sizeof(OrbisNetSockaddrIn);
    m_pending.erase(it);
    LOG_DEBUG(Lib_Net, "stun response sent, mapped address {:#x}:{}", echo.external_addr,
              sceNetNtohs(echo.external_port));
    return static_cast<s32>(response_size);
}

s32 FakeStun::LoopbackSocket() {
    if (m_loopback_socket >= 0) {
        return m_loopback_socket;
    }
    const s32 socket =
        sceNetSocket("shadNetStunLoopback", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    if (socket < 0) {
        LOG_ERROR(Lib_Net, "could not create the stun loopback socket, {:#x}",
                  static_cast<u32>(socket));
        return socket;
    }
    OrbisNetSockaddrIn addr{};
    addr.sin_len = sizeof(addr);
    addr.sin_family = ORBIS_NET_AF_INET;
    addr.sin_addr = sceNetHtonl(LoopbackAddr);
    OrbisNetSocklen_t addr_size = sizeof(addr);
    s32 ret = sceNetBind(socket, reinterpret_cast<const OrbisNetSockaddr*>(&addr), sizeof(addr));
    if (ret >= 0) {
        ret = sceNetGetsockname(socket, reinterpret_cast<OrbisNetSockaddr*>(&addr), &addr_size);
    }
    if (ret < 0) {
        LOG_ERROR(Lib_Net, "could not bind the stun loopback socket, {:#x}", static_cast<u32>(ret));
        sceNetSocketClose(socket);
        return ret;
    }
    m_loopback_socket = socket;
    m_loopback_port = addr.sin_port;
    return socket;
}

bool FakeStun::OnShadNetEcho(std::span<const u8> data, const OrbisNetSockaddrIn& from) {
    if (data.size() != sizeof(ShadNetStunEcho)) {
        return false;
    }
    std::scoped_lock lk{m_mutex};
    if (from.sin_addr != m_echo_addr || from.sin_port != m_echo_port) {
        return false;
    }
    const s32 loopback = LoopbackSocket();
    if (loopback < 0) {
        return true;
    }

    for (const auto& [socket, request] : m_pending) {
        if (!request.forwarded) {
            continue;
        }
        OrbisNetSockaddrIn to{};
        OrbisNetSocklen_t to_size = sizeof(to);
        if (sceNetGetsockname(socket, reinterpret_cast<OrbisNetSockaddr*>(&to), &to_size) < 0) {
            continue;
        }
        to.sin_addr = sceNetHtonl(LoopbackAddr);
        to.sin_vport = 0;
        const s32 ret = sceNetSendto(loopback, data.data(), data.size(), 0,
                                     reinterpret_cast<const OrbisNetSockaddr*>(&to), sizeof(to));
        LOG_DEBUG(Lib_Net, "stun answer forwarded to socket {} on port {}, ret {:#x}", socket,
                  sceNetNtohs(to.sin_port), static_cast<u32>(ret));
    }
    return true;
}

void FakeStun::OnSocketClose(s32 socket) {
    std::scoped_lock lk{m_mutex};
    m_pending.erase(socket);
}

} // namespace Libraries::Network
