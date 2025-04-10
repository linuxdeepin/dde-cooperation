// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string.h>
#include "tcpconnection.h"
#include "tcpserver.h"
#include "tcpclient.h"
#include "specodec.h"
#include "specdata.h"
#include <chrono>
#include "zrpc_log.h"

namespace zrpc_ns {

TcpConnection::TcpConnection(asio::io_context& io_context,
                           TcpServer *tcp_svr,
                           int buff_size,
                           NetAddress::ptr peer_addr)
    : m_io_context(io_context)
    , m_socket(io_context)
    , m_tcp_svr(tcp_svr)
    , m_peer_addr(peer_addr) {
    m_codec = m_tcp_svr->getCodec();
    initBuffer(buff_size);
    m_temp_buffer.resize(buff_size);
}

TcpConnection::TcpConnection(asio::io_context& io_context,
                           TcpClient *tcp_cli,
                           int buff_size,
                           NetAddress::ptr peer_addr)
    : m_io_context(io_context)
    , m_socket(io_context)
    , m_tcp_cli(tcp_cli)
    , m_peer_addr(peer_addr) {
    m_codec = m_tcp_cli->getCodeC();
    initBuffer(buff_size);
    m_temp_buffer.resize(buff_size);
}

TcpConnection::~TcpConnection() {
    stop();
}

void TcpConnection::initBuffer(int size) {
    m_read_buffer = std::make_shared<TcpBuffer>(size);
    m_write_buffer = std::make_shared<TcpBuffer>(size);
}

void TcpConnection::start() {
    m_state = Connected;
    m_last_recv_time = std::chrono::system_clock::now().time_since_epoch().count();
    
    // 开始异步读取
    m_socket.async_read_some(
        asio::buffer(m_temp_buffer),
        std::bind(
            &TcpConnection::handleRead,
            shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2
        )
    );
}

void TcpConnection::stop() {
    if (m_state == Closed) {
        return;
    }
    
    m_state = Closed;
    if (m_socket.is_open()) {
        asio::error_code ec;
        m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        m_socket.close(ec);
    }
}

void TcpConnection::setCallBack(const CallBackFunc &call) {
    m_callback = call;
}

void TcpConnection::handleRead(const asio::error_code& error, size_t bytes_transferred) {
    if (!error) {
        printf("Received %zu bytes from %s\n", bytes_transferred, getRemoteIp().c_str());
        m_last_recv_time = std::chrono::system_clock::now().time_since_epoch().count();
        processRead(bytes_transferred);

        doWrite();
        
        // 继续读取
        m_socket.async_read_some(
            asio::buffer(m_temp_buffer),
            std::bind(
                &TcpConnection::handleRead,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2
            )
        );
    } else {
        if (error != asio::error::operation_aborted) {
            stop();
            if (m_callback) {
                uint16_t port = m_tcp_svr ? m_tcp_svr->getLocalAddr()->getPort() : 
                               (m_tcp_cli ? m_tcp_cli->getLocalAddr()->getPort() : 0);
                m_callback(-1, getRemoteIp(), port);
            }
        }
    }
}

void TcpConnection::processRead(size_t bytes_transferred) {
    // 将数据写入读缓冲区
    if (m_read_buffer->writeAvailable() < bytes_transferred) {
        m_read_buffer->resizeBuffer(m_read_buffer->getSize() + bytes_transferred);
    }
    m_read_buffer->writeToBuffer(m_temp_buffer.data(), bytes_transferred);
    // m_read_buffer->recycleWrite(bytes_transferred); // FIXME：这里有问题，导致readAvailable()不准确

    // 处理读取的数据
    while (m_read_buffer->readAvailable() > 0) {
        std::shared_ptr<AbstractData> data = std::make_shared<SpecDataStruct>();
        m_codec->decode(m_read_buffer.get(), data.get());
        
        if (!data->decode_succ) {
            ELOG << "decode request error";
            break;
        }

        if (m_tcp_svr) {
            m_tcp_svr->getDispatcher()->dispatch(data.get(), this);
        } else if (m_tcp_cli) {
            std::shared_ptr<SpecDataStruct> tmp = std::dynamic_pointer_cast<SpecDataStruct>(data);
            if (tmp) {
                m_reply_datas.insert(std::make_pair(tmp->msg_req, tmp));
            }
        }
    }
    m_read_buffer->clearBuffer();
}

void TcpConnection::output() {
    if (m_state != Connected) {
        return;
    }

    // 发送数据
    doWrite();
}

void TcpConnection::handleWrite(const asio::error_code& error) {
    if (!error) {
        m_write_buffer->clearBuffer();
        doWrite();
    } else {
        if (error != asio::error::operation_aborted) {
            stop();
        }
    }
}

void TcpConnection::doWrite() {
    if (m_state != Connected || m_write_buffer->readAvailable() == 0) {
        return;
    }

    printf("doWrite m_write_buffer readAvailable:%zu bytes!!\n", m_write_buffer->readAvailable());
    asio::async_write(
        m_socket,
        asio::buffer(
            m_write_buffer->getBufferVector().data(),
            m_write_buffer->readAvailable()
        ),
        std::bind(
            &TcpConnection::handleWrite,
            shared_from_this(),
            std::placeholders::_1
        )
    );
}

bool TcpConnection::getResPackageData(const std::string &msg_req, SpecDataStruct::pb_ptr &pb_struct) {
    auto it = m_reply_datas.find(msg_req);
    if (it != m_reply_datas.end()) {
        pb_struct = it->second;
        m_reply_datas.erase(it);
        return true;
    }
    return false;
}

std::string TcpConnection::getRemoteIp() const {
    try {
        return m_socket.remote_endpoint().address().to_string();
    } catch (const std::exception& e) {
        return m_peer_addr ? m_peer_addr->getIP() : "";
    }
}

bool TcpConnection::waited() const {
    if (m_last_recv_time <= 0) {
        return false;
    }
    return (std::chrono::system_clock::now().time_since_epoch().count() - m_last_recv_time) > 3000;
}

} // namespace zrpc_ns
