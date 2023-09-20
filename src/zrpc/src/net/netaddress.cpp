// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "netaddress.h"
#include "co/log.h"

namespace zrpc {

ServerAddress::ServerAddress(const fastring &ip, uint16 port, fastring key, fastring ca)
    : m_ip(ip)
    , m_port(port)
    , m_key_path(key)
    , m_ca_path(ca) {
}

ServerAddress::ServerAddress(const fastring &addr, fastring key, fastring ca) {
    size_t i = addr.find_first_of(":");
    if (i == addr.npos) {
        ELOG << "invalid addr[" << addr << "]";
        return;
    }
    m_ip = addr.substr(0, i);
    m_port = std::atoi(addr.substr(i + 1, addr.size() - i - 1).c_str());
    m_key_path = key;
    m_ca_path = ca;
}

ServerAddress::ServerAddress(uint16 port, fastring key, fastring ca)
    : m_key_path(key)
    , m_ca_path(ca)
    , m_port(port) {

    if (m_key_path.empty() || m_ca_path.empty()) {
        m_useSSL = false;
    } else {
        m_useSSL = true;
    }
}

ClientAddress::ClientAddress(const fastring &ip, uint16 port, bool ssl)
    : m_ip(ip)
    , m_port(port)
    , m_useSSL(ssl) {
}

ClientAddress::ClientAddress(const fastring &addr, bool ssl) {
    size_t i = addr.find_first_of(":");
    if (i == addr.npos) {
        ELOG << "invalid addr[" << addr << "]";
        return;
    }
    m_ip = addr.substr(0, i);
    m_port = std::atoi(addr.substr(i + 1, addr.size() - i - 1).c_str());
    m_useSSL = ssl;
}

ClientAddress::ClientAddress(uint16 port, bool ssl)
    : m_port(port)
    , m_useSSL(ssl) {
}

} // namespace zrpc
