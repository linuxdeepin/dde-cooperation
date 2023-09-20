// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <google/protobuf/service.h>
#include "zrpc.h"
#include "co/log.h"
#include "net/tcpserver.h"

namespace zrpc {

zrpc::TcpServer::ptr gRpcServer;

// bool RegisterService(google::protobuf::Service* service) {
//   return gRpcServer->registerService(service);
// }

TcpServer::ptr GetServer() {
    if (gRpcServer.get() == nullptr) {
        fastring key("/home/doll/public/certificates/desktop.key");
        fastring crt("/home/doll/public/certificates/ca.crt");
        zrpc::ServerAddress::ptr addr = std::make_shared<zrpc::ServerAddress>("0.0.0.0",
                                                                              7788,
                                                                              key,
                                                                              crt);
        gRpcServer = std::make_shared<TcpServer>(addr);
    }
    return gRpcServer;
}

void StartRpcServer() {
    gRpcServer->start();
}

} // namespace zrpc
