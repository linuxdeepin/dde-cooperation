// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZRPC_TCPSERVER_H
#define ZRPC_TCPSERVER_H

#include <map>
#include <memory>
#include <google/protobuf/service.h>
#include "netaddress.h"
#include "tcpconnection.h"
#include "abstractcodec.h"
#include "abstractdispatcher.h"
#include "zrpc_defines.h"
#include <asio.hpp>

namespace zrpc_ns {

ZRPC_API class TcpServer {
public:
    typedef std::shared_ptr<TcpServer> ptr;

    TcpServer(NetAddress::ptr addr);
    ~TcpServer();

    bool start();
    void stop();
    bool isStarted() const { return m_started; }
    bool registerService(std::shared_ptr<google::protobuf::Service> service);
    void setCallBackFunc(const CallBackFunc &callback);
    bool checkConnected();

    AbstractDispatcher::ptr getDispatcher() { return m_dispatcher; }
    AbstractCodeC::ptr getCodec() { return m_codec; }
    NetAddress::ptr getPeerAddr() { return m_addr; }
    NetAddress::ptr getLocalAddr() { return m_addr; }

private:
    void startAccept();
    void handleAccept(TcpConnection::ptr conn, const asio::error_code& error);

private:
    bool m_started{false};
    NetAddress::ptr m_addr;
    AbstractDispatcher::ptr m_dispatcher;
    AbstractCodeC::ptr m_codec;
    
    asio::io_context m_io_context;
    std::unique_ptr<asio::ip::tcp::acceptor> m_acceptor;
    std::map<std::string, TcpConnection::ptr> m_connections;
    CallBackFunc m_callback{nullptr};
};

} // namespace zrpc_ns

#endif
