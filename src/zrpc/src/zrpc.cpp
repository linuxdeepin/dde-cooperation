// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <google/protobuf/service.h>
#include "zrpc.h"
#include "co/log.h"
#include "net/tcpserver.h"

namespace zrpc {

zrpc::TcpServer::ptr gRpcServer;

#define REG_RPCSERVICE(reg, service)                                                               \
    do {                                                                                           \
        if (!reg->registerService(std::make_shared<service>())) {                                  \
            ELOG << "register protobuf service error!!!";                                          \
            return false;                                                                          \
        }                                                                                          \
    } while (0)


ZRpcClient::ZRpcClient(char *ip, uint16 port, bool ssl)
{
    zrpc::NetAddress::ptr addr = std::make_shared<zrpc::NetAddress>(ip, port, ssl);
    m_channel = std::make_shared<ZRpcChannel>(addr);

    m_controller = std::make_shared<ZRpcController>();
    // default timeout is 5 seconds
    m_controller.get()->SetTimeout(5000);
}

void ZRpcClient::setTimeout(uint32 timeout)
{
    m_controller.get()->SetTimeout(timeout);
}


ZRpcServer::ZRpcServer(uint16 port, char *key, char *crt)
{
    // ip is localhost: "0.0.0.0"
    zrpc::NetAddress::ptr addr = std::make_shared<zrpc::NetAddress>("0.0.0.0", port, key, crt);                                                               
    m_tcpserver = std::make_shared<TcpServer>(addr);
}

bool ZRpcServer::start()
{
    if (m_tcpserver == nullptr) {
        ELOG << "ZRPCServer::init failed!";
        return false;
    }

    m_tcpserver->start();
    return true;
}

} // namespace zrpc
