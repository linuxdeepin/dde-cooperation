// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//#include <sys/socket.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include "tcpserver.h"
#include "tcpconnection.h"
#include "rpcdispatcher.h"

#include "co/tcp.h"

namespace zrpc_ns {

TcpServer::TcpServer(NetAddress::ptr addr)
    : m_addr(addr) {
    m_dispatcher = std::make_shared<ZRpcDispacther>();
    m_codec = std::make_shared<ZRpcCodeC>();
}

void TcpServer::start() {
    const char *ip = m_addr.get()->getIP();
    uint16 port = m_addr.get()->getPort();
    bool ssl = m_addr.get()->isSSL();

    atomic_store(&_started, true, mo_relaxed);
    _tcp_serv.on_connection(&TcpServer::on_connection_cb, this);
    _tcp_serv.on_exit([this]() { co::del(this); });

    if (ssl) {
        const char *key_path = m_addr.get()->getKey();
        const char *ca_path = m_addr.get()->getCrt();
        _tcp_serv.start(ip, port, key_path, ca_path);
    } else {
        _tcp_serv.start(ip, port, NULL, NULL);
    }
}

bool TcpServer::started() {
    return _started;
}

void TcpServer::exit() {
    atomic_store(&_stopped, true, mo_relaxed);
    _tcp_serv.exit();
}

TcpServer::~TcpServer() {
    exit();
    DLOG << "~TcpServer";
}

void TcpServer::on_connection_cb(tcp::Connection conn) {
    DLOG << "on_connection_cb go";
    TcpConnection::ptr tconn = addClient(&conn);
    tconn->initServer();
}

bool TcpServer::registerService(std::shared_ptr<google::protobuf::Service> service) {
    LOG << "register service enter";
    if (service) {
        dynamic_cast<ZRpcDispacther *>(m_dispatcher.get())->registerService(service);
    } else {
        ELOG << "register service error, service ptr is nullptr";
        return false;
    }

    return true;
}

TcpConnection::ptr TcpServer::addClient(tcp::Connection *conntion) {
    int fd = conntion->socket();
    auto it = m_clients.find(fd);
    if (it != m_clients.end()) {
        it->second.reset();
        // set new Tcpconnection
        DLOG << "fd " << fd << " have exist, reset it";
        it->second = std::make_shared<TcpConnection>(this, conntion, 1024, getPeerAddr());
        return it->second;

    } else {
        DLOG << "fd " << fd << " did't exist, new it";
        TcpConnection::ptr conn = std::make_shared<TcpConnection>(this,
                                                                  conntion,
                                                                  1024,
                                                                  getPeerAddr());
        m_clients.insert(std::make_pair(fd, conn));
        return conn;
    }
}

NetAddress::ptr TcpServer::getPeerAddr() {
    // return m_acceptor->getPeerAddr();
    return m_addr;
}

NetAddress::ptr TcpServer::getLocalAddr() {
    return m_addr;
}

AbstractDispatcher::ptr TcpServer::getDispatcher() {
    return m_dispatcher;
}

AbstractCodeC::ptr TcpServer::getCodec() {
    return m_codec;
}

} // namespace zrpc_ns
