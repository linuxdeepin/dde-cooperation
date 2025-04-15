// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZRPC_TCPCLIENT_H
#define ZRPC_TCPCLIENT_H

#include <memory>
#include <google/protobuf/service.h>
#include "netaddress.h"
#include "tcpconnection.h"
#include <asio.hpp>

namespace zrpc_ns {

class TcpClient {
public:
    typedef std::shared_ptr<TcpClient> ptr;

    TcpClient(NetAddress::ptr addr);
    ~TcpClient();

    bool tryConnect();
    bool connect();
    void disconnect();
    bool isConnected() const { return m_connected; }
    
    int sendAndRecvData(const std::string &msg_no, SpecDataStruct::pb_ptr &res);
    void setTimeout(const int v) { m_timeout = v; }
    void setTryCount(const int v) { m_try_count = v; }
    const std::string &getErrInfo() const { return m_err_info; }

    NetAddress::ptr getPeerAddr() const { return m_peer_addr; }
    NetAddress::ptr getLocalAddr() const { return m_local_addr; }
    AbstractCodeC::ptr getCodeC() const { return m_codec; }
    TcpConnection::ptr getConnection() const { return m_connection; }

private:
    bool doConnect();

private:
    int m_try_count{3};      // max try reconnect times
    int m_timeout{10000};    // max connect timeout, ms
    bool m_connected{false};
    std::string m_err_info;  // error info of client

    NetAddress::ptr m_local_addr;
    NetAddress::ptr m_peer_addr;
    TcpConnection::ptr m_connection;
    AbstractCodeC::ptr m_codec;

    asio::io_context m_io_context;
    std::unique_ptr<asio::io_context::work> m_work;
    std::thread m_io_thread;
};

} // namespace zrpc_ns

#endif
