// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// From https://github.com/GoldHEN/GoldHEN_Plugins_Repository/blob/main/common/plugin_common.c

#include <Common.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "plugin_common.h"

// Thanks to OSM
// https://github.com/OSM-Made/PS4-Notify/blob/c6d259bc5bd4aa519f5b0ce4f5e27ef7cb01ffdd/Notify.cpp

// For pre formatted strings
void NotifyStatic(const char* IconUri, const char* text) {
    OrbisNotificationRequest Buffer;
    final_printf("Notify text:\n%s\n", text);
    Buffer.type = NotificationRequest;
    Buffer.unk3 = 0;
    Buffer.useIconImageUri = 1;
    Buffer.targetId = -1;
    strcpy(Buffer.message, text);
    strcpy(Buffer.iconUri, IconUri);
    sceKernelSendNotificationRequest(0, &Buffer, sizeof(Buffer), 0);
}

// For formatted strings
void Notify(const char* IconUri, const char* FMT, ...) {
    OrbisNotificationRequest Buffer;
    va_list args;
    va_start(args, FMT);
    vsprintf(Buffer.message, FMT, args);
    va_end(args);
    final_printf("Notify message:\n%s\n", Buffer.message);
    Buffer.type = NotificationRequest;
    Buffer.unk3 = 0;
    Buffer.useIconImageUri = 1;
    Buffer.targetId = -1;
    strcpy(Buffer.iconUri, IconUri);
    sceKernelSendNotificationRequest(0, &Buffer, sizeof(Buffer), 0);
}

namespace {

constexpr uint16_t JailbreakPort = 9028;
constexpr uint16_t RejailPort = 9030;
constexpr size_t JailbreakPacketSize = 2576;
constexpr uint32_t RejailCommand = 0xBEEEEEEF;
constexpr long DaemonTimeoutSeconds = 15;

int ConnectToDaemon(uint16_t port) {
    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }

    timeval timeout{};
    timeout.tv_sec = DaemonTimeoutSeconds;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    sockaddr_in addr{};
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        close(sock);
        return -1;
    }
    return sock;
}

bool RejailAvailable() {
    const int sock = ConnectToDaemon(RejailPort);
    if (sock < 0) {
        return false;
    }
    close(sock);
    return true;
}

bool SendToDaemon(uint16_t port, const void* data, size_t size) {
    const int sock = ConnectToDaemon(port);
    if (sock < 0) {
        return false;
    }

    size_t sent = 0;
    while (sent < size) {
        const ssize_t count = send(sock, reinterpret_cast<const char*>(data) + sent, size - sent, 0);
        if (count <= 0) {
            break;
        }
        sent += static_cast<size_t>(count);
    }

    bool ok = false;
    if (sent == size) {
        shutdown(sock, SHUT_WR);
        uint32_t status = 0;
        size_t received = 0;
        while (received < sizeof(status)) {
            const ssize_t count = recv(sock, reinterpret_cast<char*>(&status) + received,
                                       sizeof(status) - received, 0);
            if (count <= 0) {
                break;
            }
            received += static_cast<size_t>(count);
        }
        ok = received == sizeof(status) && ntohl(status) == 0;
    }

    close(sock);
    return ok;
}

} // namespace

namespace Common {

bool JailbreakViaDaemon() {
    if (!RejailAvailable()) {
        return false;
    }
    const uint8_t packet[JailbreakPacketSize]{};
    return SendToDaemon(JailbreakPort, packet, sizeof(packet));
}

void RejailViaDaemon() {
    const uint32_t command = htonl(RejailCommand);
    SendToDaemon(RejailPort, &command, sizeof(command));
}

bool DebugFlagSet(const char* name) {
    char path[160];
    snprintf(path, sizeof(path), "/data/shadnet/%s", name);
    OrbisKernelStat st{};
    return sceKernelStat(path, &st) == 0;
}

bool IsPs5() {
    static int cached = -1;
    if (cached >= 0) {
        return cached == 1;
    }

    OrbisKernelStat st{};
    char path[160];
    bool found = false;

    const char* word = sceKernelGetFsSandboxRandomWord();
    if (word && word[0]) {
        snprintf(path, sizeof(path), "/%s/common/lib/libSceNotification.sprx", word);
        found = sceKernelStat(path, &st) == 0;
    }

    cached = found ? 1 : 0;
    return found;
}

} // namespace Common
