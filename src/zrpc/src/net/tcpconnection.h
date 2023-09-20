// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZRPC_TCPCONNECTION_H
#define ZRPC_TCPCONNECTION_H

#include <memory>
#include <vector>
#include <queue>
#include <map>
#include "co/log.h"
#include "co/tcp.h"

#include "tcpbuffer.h"
#include "specodec.h"
#include "netaddress.h"

namespace zrpc {

class TcpServer;
class TcpClient;

enum TcpConnectionState {
    NotConnected = 1, // can do io
    Connected = 2,    // can do io
    HalfClosing = 3,  // server call shutdown, write half close. can read,but can't write
    Closed = 4,       // can't do io
};

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {

public:
    typedef std::shared_ptr<TcpConnection> ptr;

    TcpConnection(TcpServer *tcp_svr,
                  tcp::Connection *serv_conn,
                  int buff_size,
                  AbNetAddress::ptr peer_addr);

    TcpConnection(TcpClient *tcp_cli,
                  tcp::Client *cli_conn,
                  int buff_size,
                  AbNetAddress::ptr peer_addr);

    void setUpClient();

    ~TcpConnection();

    void initBuffer(int size);

    enum ConnectionType {
        ServerConnection = 1, // owned by tcp_server
        ClientConnection = 2, // owned by tcp_client
    };

public:
    void shutdownConnection();

    TcpConnectionState getState();

    void setState(const TcpConnectionState &state);

    TcpBuffer *getInBuffer();

    TcpBuffer *getOutBuffer();

    AbstractCodeC::ptr getCodec() const;

    bool getResPackageData(const std::string &msg_req, SpecDataStruct::pb_ptr &pb_struct);

public:
    void MainServerLoopCorFunc();

    void input();

    void execute();

    void output();

    void initServer();

private:
    ssize_t read_hook(char *buf);
    ssize_t write_hook(const void *buf, size_t count);

    void clearClient();

private:
    TcpServer *m_tcp_svr{nullptr};
    TcpClient *m_tcp_cli{nullptr};

    AbNetAddress::ptr m_peer_addr;

    tcp::Connection *m_serv_conn{nullptr};
    tcp::Client *m_cli_conn{nullptr};
    int m_trans_timeout{10000}; // max receive or send timeout, ms

    TcpConnectionState m_state{TcpConnectionState::Connected};
    ConnectionType m_connection_type{ServerConnection};

    TcpBuffer::ptr m_read_buffer;
    TcpBuffer::ptr m_write_buffer;

    AbstractCodeC::ptr m_codec;

    bool m_stop{false};

    std::map<std::string, std::shared_ptr<SpecDataStruct>> m_reply_datas;
};

} // namespace zrpc

#endif
