// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <google/protobuf/service.h>
#include "zrpc.h"
#include "co/log.h"
#include "net/tcpserver.h"

namespace zrpc_ns {

zrpc_ns::TcpServer::ptr gRpcServer;

TcpServer::ptr GetServer() {
    return gRpcServer;
}

ZRpcClient::ZRpcClient(char *ip, uint16 port, bool ssl) {
    zrpc_ns::NetAddress::ptr addr = std::make_shared<zrpc_ns::NetAddress>(ip, port, ssl);
    m_channel = std::make_shared<ZRpcChannel>(addr);

    m_controller = std::make_shared<ZRpcController>();
    // default timeout is 5 seconds
    m_controller.get()->SetTimeout(5000);
}

void ZRpcClient::setTimeout(uint32 timeout) {
    m_controller.get()->SetTimeout(timeout);
}

class ZRpcServerImpl {
public:
    ZRpcServerImpl(uint16 port, char *key, char *crt) {
        // ip is localhost: "0.0.0.0"
        zrpc_ns::NetAddress::ptr addr = std::make_shared<zrpc_ns::NetAddress>("0.0.0.0", port, key, crt);
        _tcpserver = std::make_shared<TcpServer>(addr);
    }

    ~ZRpcServerImpl() = default;

    TcpServer::ptr getServer() { return _tcpserver; }

    bool start() {
        if (_tcpserver == nullptr) {
            ELOG << "ZRPCServer::init failed!";
            return false;
        }

        _tcpserver->start();
        return true;
    }

private:
    TcpServer::ptr _tcpserver{nullptr};
};

ZRpcServer::ZRpcServer(uint16 port, char *key, char *crt) {
    _p = co::make<ZRpcServerImpl>(port, key, crt);
}

ZRpcServer::~ZRpcServer() {
    co::del((ZRpcServerImpl *)_p);
}

bool ZRpcServer::doregister(std::shared_ptr<google::protobuf::Service> service)
{
    TcpServer::ptr tcpserver = ((ZRpcServerImpl *)_p)->getServer();
    return tcpserver->registerService(service);
}

bool ZRpcServer::start() {
    return ((ZRpcServerImpl *)_p)->start();
}

} // namespace zrpc_ns
