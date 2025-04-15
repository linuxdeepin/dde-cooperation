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
#include "zrpc_log.h"

namespace zrpc_ns {

TcpServer::TcpServer(NetAddress::ptr addr)
    : m_addr(addr) {
    m_dispatcher = std::make_shared<ZRpcDispatcher>();
    m_codec = std::make_shared<ZRpcCodeC>();
}

TcpServer::~TcpServer() {
    stop();
}

bool TcpServer::start() {
    if (m_started) {
        return true;
    }

    try {
        asio::ip::tcp::endpoint endpoint(
            asio::ip::address::from_string(m_addr->getIP()),
            m_addr->getPort()
        );

        m_acceptor = std::make_unique<asio::ip::tcp::acceptor>(m_io_context);
        m_acceptor->open(endpoint.protocol());
        m_acceptor->set_option(asio::ip::tcp::acceptor::reuse_address(true));
        m_acceptor->bind(endpoint);
        m_acceptor->listen();

        printf("TCP server listening on %s:%d\n",
            m_addr->getIP(), m_addr->getPort());
        
        startAccept();
        m_started = true;
        
        // 在单独线程中运行IO上下文
        std::thread([this](){
            try {
                m_io_context.run();
            } catch (const std::exception& e) {
                printf("IO context error: %s\n", e.what());
            }
        }).detach();
        
        return true;
    } catch (const std::exception& e) {
        printf("TcpServer start failed: %s\n", e.what());
        return false;
    }
}

void TcpServer::stop() {
    if (!m_started) {
        return;
    }

    m_started = false;
    if (m_acceptor) {
        m_acceptor->close();
    }
    m_io_context.stop();

    m_connections.clear();
}

void TcpServer::startAccept() {
    DLOG << "Start accept connection" << std::endl;
    TcpConnection::ptr new_connection = std::make_shared<TcpConnection>(
        m_io_context,
        this,
        PERPKG_MAX_LEN,
        m_addr
    );

    m_acceptor->async_accept(
        new_connection->socket(),
        std::bind(
            &TcpServer::handleAccept,
            this,
            new_connection,
            std::placeholders::_1
        )
    );
}

void TcpServer::handleAccept(TcpConnection::ptr conn, const asio::error_code& error) {
    DLOG << "Accept connection from " << conn->getRemoteIp() << std::endl;
    if (!error) {
        conn->start();
        m_connections[conn->getRemoteIp()] = conn;
        
        if (m_callback) {
            m_callback(0, conn->getRemoteIp(), m_addr->getPort());
        }
    } else {
        ELOG << "Accept failed: " << error.message();
    }

    startAccept();
}

bool TcpServer::registerService(std::shared_ptr<google::protobuf::Service> service) {
    if (service) {
        dynamic_cast<ZRpcDispatcher *>(m_dispatcher.get())->registerService(service);
        return true;
    }
    ELOG << "register service error, service ptr is nullptr";
    return false;
}

void TcpServer::setCallBackFunc(const CallBackFunc &callback) {
    m_callback = callback;
}

bool TcpServer::checkConnected() {
    for (const auto& pair : m_connections) {
        if (!pair.second->waited()) {
            return true;
        }
    }
    return false;
}

} // namespace zrpc_ns
