// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UTILS_NET_H
#define UTILS_NET_H

#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#include <string>
#include <QDebug>

namespace Net {

inline std::string getHostname(void) {
    char hostName[_SC_HOST_NAME_MAX];
    gethostname(hostName, _SC_HOST_NAME_MAX);
    return hostName;
}

inline void tcpSocketSetKeepAliveOption(int fd) {
    //开启保活
    int enable = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
                   &enable, sizeof(enable)) != 0) {
        qWarning("fail to enable SO_KEEPALIVE");
    }

    //失连时间
    int idle = 11;
    if (setsockopt(fd, SOL_TCP, TCP_KEEPIDLE,
                   &idle, sizeof(idle)) != 0) {
        qWarning("fail to enable TCP_KEEPIDLE");
    }

    //重连时间
    int retryInterval = 2;
    if (setsockopt(fd, SOL_TCP, TCP_KEEPINTVL,
                   &retryInterval, sizeof(retryInterval)) != 0) {
        qWarning("fail to enable TCP_KEEPINTVL");
    }

    //重连次数
    int retryCnt = 5;
    if (setsockopt(fd, SOL_TCP, TCP_KEEPCNT,
                   &retryCnt, sizeof(retryCnt)) != 0) {
        qWarning("fail to enable TCP_KEEPCNT");
    }

    // 10s内收不到ack即超时处理
    unsigned int timeout = 10000;
    if (setsockopt(fd, IPPROTO_TCP, TCP_USER_TIMEOUT, &timeout, sizeof(timeout))) {
        qWarning("set TCP_USER_TIMEOUT option error");
    }
}

} // namespace Net

#endif // !UTILS_NET_H
