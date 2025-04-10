// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <google/protobuf/service.h>
#include "zrpc.h"
#include "zrpc_log.h"
#include "net/tcpserver.h"

namespace zrpc_ns {

ZRpcClient::ZRpcClient(const char *ip, uint16_t port, bool ssl, const bool isLong) {
    zrpc_ns::NetAddress::ptr addr = std::make_shared<zrpc_ns::NetAddress>(ip, port, ssl);
    m_channel = std::make_shared<ZRpcChannel>(addr, isLong);
    m_controller = std::make_shared<ZRpcController>();
    m_controller->SetTimeout(5000); // default timeout is 5 seconds
}

ZRpcClient::~ZRpcClient() = default;

void ZRpcClient::setTimeout(uint32_t timeout) {
    m_controller->SetTimeout(static_cast<int>(timeout));
}

class ZRpcServer::Impl {
public:
    Impl(uint16_t port, char *key, char *crt) {
        zrpc_ns::NetAddress::ptr addr = std::make_shared<zrpc_ns::NetAddress>("0.0.0.0", port, key, crt);
        m_server = std::make_shared<TcpServer>(addr);
    }

    ~Impl() = default;

    TcpServer::ptr getServer() { return m_server; }

    bool start() {
        if (!m_server) {
            ELOG << "ZRPCServer::init failed!";
            return false;
        }
        return m_server->start();
    }

    bool checkConnected() {
        return m_server && m_server->checkConnected();
    }

private:
    TcpServer::ptr m_server;
};

ZRpcServer::ZRpcServer(uint16_t port, char *key, char *crt)
    : pImpl(std::make_unique<Impl>(port, key, crt)) {
}

ZRpcServer::~ZRpcServer() = default;

bool ZRpcServer::doregister(std::shared_ptr<google::protobuf::Service> service) {
    return pImpl->getServer()->registerService(service);
}

bool ZRpcServer::start() {
    return pImpl->start();
}

void ZRpcServer::setCallBackFunc(const std::function<void(int, const std::string &, const uint16_t)> &call) {
    pImpl->getServer()->setCallBackFunc(call);
}

bool ZRpcServer::checkConnected() {
    return pImpl->checkConnected();
}

} // namespace zrpc_ns
