// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZRPC_ZRPC_H
#define ZRPC_ZRPC_H

#include <google/protobuf/service.h>
#include <memory>
#include <stdio.h>
#include <functional>
#include "net/tcpserver.h"
#include "rpcchannel.h"
#include "rpccontroller.h"
#include "netaddress.h"

namespace zrpc {

class ZRpcClient {

public:
    ZRpcClient(char *ip, uint16 port, bool ssl = true);

    void setTimeout(uint32 timeout);

    ZRpcChannel* getChannel() const { return m_channel.get(); }
    ZRpcController* getControler() const { return m_controller.get(); }

private:
    ZRpcChannel::ptr m_channel{nullptr};
    ZRpcController::ptr m_controller{nullptr};
};

class ZRpcServer {

    #define REG_RPCSERVICE(reg, service)                                                           \
    do {                                                                                           \
        if (!reg->registerService(std::make_shared<service>())) {                                  \
            ELOG << "register protobuf service error!!!";                                          \
            return false;                                                                          \
        }                                                                                          \
    } while (0)

public:
    ZRpcServer(uint16 port, char *key, char *crt);

    template <class T>
    bool registerService()
    {
        if (m_tcpserver == nullptr) {
            ELOG << "ZRPCServer::init failed!";
            return false;
        }
        // REG_RPCSERVICE(m_tcpserver, T);
        m_tcpserver->registerService(std::make_shared<T>());
        return true;
    }
    bool start();

private:
    TcpServer::ptr m_tcpserver{nullptr};
};

} // namespace zrpc

#endif