// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZRPC_NETADDRESS_H
#define ZRPC_NETADDRESS_H

#include <unistd.h>
#include <memory>
#include "co/fastring.h"
#include "co/fastream.h"

namespace zrpc {

class AbNetAddress {

public:
    typedef std::shared_ptr<AbNetAddress> ptr;

    virtual fastring toString() const = 0;

    virtual fastring getIP() const = 0;

    virtual int getPort() const = 0;

    virtual bool isSSL() const = 0;

    virtual fastring getKey() const = 0;

    virtual fastring getCa() const = 0;
};

class ServerAddress : public AbNetAddress {

public:
    ServerAddress(const fastring &ip, uint16 port, fastring key, fastring ca);

    ServerAddress(const fastring &addr, fastring key, fastring ca);

    ServerAddress(uint16 port, fastring key, fastring ca);

    fastring toString() const {
        fastream ss;
        ss << m_ip << ":" << m_port;
        return ss.str();
    }

    fastring getIP() const { return m_ip; }

    int getPort() const { return m_port; }

    bool isSSL() const { return m_useSSL; }

    fastring getKey() const { return m_key_path; }

    fastring getCa() const { return m_ca_path; }

private:
    fastring m_key_path;
    fastring m_ca_path;

    fastring m_ip;
    uint16 m_port;

    bool m_useSSL;
};

class ClientAddress : public AbNetAddress {

public:
    ClientAddress(const fastring &ip, uint16 port, bool ssl);

    ClientAddress(const fastring &addr, bool ssl);

    ClientAddress(uint16 port, bool ssl);

    fastring toString() const {
        fastream ss;
        ss << m_ip << ":" << m_port;
        return ss.str();
    }

    fastring getIP() const { return m_ip; }

    int getPort() const { return m_port; }

    bool isSSL() const { return m_useSSL; }

    fastring getKey() const { return ""; }

    fastring getCa() const { return ""; }

private:
    fastring m_ip;
    uint16 m_port;

    bool m_useSSL;
};

} // namespace zrpc

#endif
