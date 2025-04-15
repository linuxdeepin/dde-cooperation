// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZRPC_NETADDRESS_H
#define ZRPC_NETADDRESS_H

#include <unistd.h>
#include <string.h>
#include <memory>
#include <string>
#include <sstream>
#include <cstdint>

#include "zrpc_defines.h"

namespace zrpc_ns {

class ZRPC_API NetAddress {

public:
    typedef std::shared_ptr<NetAddress> ptr;

    NetAddress(const char *ip, uint16_t port, char *key, char *crt);
    NetAddress(const char *ip, uint16_t port, bool ssl);

    std::string toString() const {
        std::stringstream ss;
        ss << m_ip << ":" << m_port;
        return ss.str();
    }

    const char* getIP() const { return m_ip; }
    uint16_t getPort() const { return m_port; }
    bool isSSL() const { return m_ssl; }
    const char* getKey() const { return m_ssl_key; }
    const char* getCrt() const { return m_ssl_crt; }

private:
    char m_ip[128]{0};
    uint16_t m_port;
    bool m_ssl;
    char m_ssl_key[4096]{0};
    char m_ssl_crt[4096]{0};
};

} // namespace zrpc_ns

#endif
