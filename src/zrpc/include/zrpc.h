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

namespace zrpc {

#define REGISTER_RPCSERVICE(service)                                                               \
    do {                                                                                           \
        if (!zrpc::GetServer()->registerService(std::make_shared<service>())) {                    \
            printf("Start RPC server error, because register protobuf service error, please "  \
                   "look up rpc log get more details!\n");                                         \
            _exit(0);                                                                              \
        }                                                                                          \
    } while (0)

// bool RegisterService(google::protobuf::Service* service);

void StartRpcServer();

TcpServer::ptr GetServer();

} // namespace zrpc

#endif