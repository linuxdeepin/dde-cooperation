// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZRPC_TCPCONNECTION_H
#define ZRPC_TCPCONNECTION_H

#include <memory>
#include <vector>
#include <queue>
#include <map>
#include "tcpbuffer.h"
#include "specodec.h"
#include "netaddress.h"
#include <asio.hpp>

using CallBackFunc = std::function<void(int, const std::string &, const uint16_t)>;
namespace zrpc_ns {

class TcpServer;
class TcpClient;

// the max socket buffer len
#define PERPKG_MAX_LEN 16 * 1024

enum TcpConnectionState {
    NotConnected = 1,   // can do io
    Connected = 2,   // can do io
    HalfClosing = 3,   // server call shutdown, write half close. can read,but can't write
    Closed = 4,   // can't do io
};

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    typedef std::shared_ptr<TcpConnection> ptr;

    TcpConnection(asio::io_context& io_context,
                 TcpServer *tcp_svr,
                 int buff_size,
                 NetAddress::ptr peer_addr);

    TcpConnection(asio::io_context& io_context,
                 TcpClient *tcp_cli,
                 int buff_size,
                 NetAddress::ptr peer_addr);

    ~TcpConnection();

    void start();
    void stop();
    void setCallBack(const CallBackFunc &call);

    asio::ip::tcp::socket& socket() { return m_socket; }
    TcpConnectionState getState() const { return m_state; }
    void setState(const TcpConnectionState &state) { m_state = state; }
    
    TcpBuffer* getInBuffer() { return m_read_buffer.get(); }
    TcpBuffer* getOutBuffer() { return m_write_buffer.get(); }
    AbstractCodeC::ptr getCodec() const { return m_codec; }
    
    bool getResPackageData(const std::string &msg_req, SpecDataStruct::pb_ptr &pb_struct);
    std::string getRemoteIp() const;
    bool waited() const;
    void output();

private:
    void initBuffer(int size);
    void handleRead(const asio::error_code& error, size_t bytes_transferred);
    void handleWrite(const asio::error_code& error);
    void processRead(size_t bytes_transferred);
    void doWrite();

private:
    asio::io_context& m_io_context;
    asio::ip::tcp::socket m_socket;
    
    TcpServer *m_tcp_svr{nullptr};
    TcpClient *m_tcp_cli{nullptr};
    NetAddress::ptr m_peer_addr;
    
    TcpConnectionState m_state{NotConnected};
    TcpBuffer::ptr m_read_buffer;
    TcpBuffer::ptr m_write_buffer;
    AbstractCodeC::ptr m_codec;
    
    std::map<std::string, std::shared_ptr<SpecDataStruct>> m_reply_datas;
    CallBackFunc m_callback{nullptr};
    
    int64_t m_last_recv_time{0};
    std::vector<char> m_temp_buffer;
};

}   // namespace zrpc_ns

#endif
