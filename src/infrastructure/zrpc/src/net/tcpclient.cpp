// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//#include <sys/socket.h>
//#include <arpa/inet.h>
#include <sstream>
#include "netaddress.h"
#include "tcpclient.h"
#include "specodec.h"
#include <thread>
#include <chrono>
#include "../include/zrpc_log.h"

namespace zrpc_ns {

TcpClient::TcpClient(NetAddress::ptr addr)
    : m_peer_addr(addr) {
    m_codec = std::make_shared<ZRpcCodeC>();
    m_work = std::make_unique<asio::io_context::work>(m_io_context);
    m_io_thread = std::thread([this]() { m_io_context.run(); });
}

TcpClient::~TcpClient() {
    disconnect();
    if (m_work) {
        m_work.reset();
    }
    if (m_io_thread.joinable()) {
        m_io_thread.join();
    }
}

bool TcpClient::tryConnect() {
    for (int i = 0; i < m_try_count && !m_connected; ++i) {
        if (connect()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}

bool TcpClient::connect() {
    if (m_connected) {
        return true;
    }
    return doConnect();
}

void TcpClient::disconnect() {
    m_connected = false;
    if (m_connection) {
        m_connection->stop();
        m_connection.reset();
    }
}

bool TcpClient::doConnect() {
    try {
        asio::ip::tcp::endpoint endpoint(
            asio::ip::address::from_string(m_peer_addr->getIP()),
            m_peer_addr->getPort()
        );

        m_connection = std::make_shared<TcpConnection>(
            m_io_context,
            this,
            PERPKG_MAX_LEN,
            m_peer_addr
        );

        asio::error_code ec;
        m_connection->socket().connect(endpoint, ec);

        if (!ec) {
            m_connected = true;
            m_connection->start();
            return true;
        }

        std::stringstream ss;
        ss << "connect to " << m_peer_addr->toString() << " failed: " << ec.message();
        m_err_info = ss.str();
        return false;
    } catch (const std::exception& e) {
        std::stringstream ss;
        ss << "connect to " << m_peer_addr->toString() << " exception: " << e.what();
        m_err_info = ss.str();
        return false;
    }
}

int TcpClient::sendAndRecvData(const std::string &msg_no, SpecDataStruct::pb_ptr &res) {
    printf("Sending request with msg_no: %s, timeout: %dms\n", msg_no.c_str(), m_timeout);
    if (!m_connected || !m_connection) {
        printf("Not connected to server\n");
        m_err_info = "not connected";
        return -1;
    }

    // 2. 编码数据到发送缓冲区
    m_connection->output();

    // 3. 启动连接读写循环
    m_connection->start();
    printf("Started connection I/O loop\n");

    auto start_time = std::chrono::steady_clock::now();
    while (!m_connection->getResPackageData(msg_no, res)) {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
        
        if (duration.count() >= m_timeout) {
            m_err_info = "timeout waiting for response";
            return -1;
        }

        if (m_connection->getState() == Closed) {
            m_err_info = "connection closed";
            disconnect();
            return -1;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    printf("Received response for msg_no: %s\n", msg_no.c_str());
    m_err_info.clear();
    return 0;
}

} // namespace zrpc_ns
