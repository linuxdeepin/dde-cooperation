// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <google/protobuf/service.h>
#include "rpcchannel.h"
#include "rpccontroller.h"
#include "netaddress.h"
#include "sample.pb.h"
#include "co/co.h"
#include "co/time.h"

void test_client() {

    zrpc::ClientAddress::ptr addr = std::make_shared<zrpc::ClientAddress>("127.0.0.1", 7788, false);

    zrpc::ZRpcChannel channel(addr);
    QueryService_Stub stub(&channel);

    zrpc::ZRpcController rpc_controller;
    rpc_controller.SetTimeout(5000);

    queryAgeReq rpc_req;
    queryAgeRes rpc_res;

    rpc_req.set_id(555);
    rpc_req.set_req_no(666);

    std::cout << "Send to server " << addr->toString()
              << ", requeset body: " << rpc_req.ShortDebugString() << std::endl;
    stub.query_age(&rpc_controller, &rpc_req, &rpc_res, NULL);

    if (rpc_controller.ErrorCode() != 0) {
        std::cout << "Failed to call server, error code: " << rpc_controller.ErrorCode()
                  << ", error info: " << rpc_controller.ErrorText() << std::endl;
        return;
    }

    std::cout << "Success get response frrom server " << addr->toString()
              << ", response body: " << rpc_res.ShortDebugString() << std::endl;
}

co::pool pool(
    []() {
        zrpc::ClientAddress::ptr addr = std::make_shared<zrpc::ClientAddress>("127.0.0.1",
                                                                              7788,
                                                                              false);
        zrpc::ZRpcChannel channel(addr);
        return (void *)new QueryService_Stub(&channel);
    },
    [](void *p) { delete (QueryService_Stub *)p; });

void test_call() {
    while (true) {
        zrpc::ZRpcController rpc_controller;
        rpc_controller.SetTimeout(5000);
        co::pool_guard<QueryService_Stub> stub(pool);
        queryAgeReq rpc_req;
        queryAgeRes rpc_res;

        rpc_req.set_id(555);
        rpc_req.set_req_no(666);

        std::cout << ", requeset body: " << rpc_req.ShortDebugString() << std::endl;
        stub->query_age(&rpc_controller, &rpc_req, &rpc_res, NULL);

        if (rpc_controller.ErrorCode() != 0) {
            std::cout << "Failed to call server, error code: " << rpc_controller.ErrorCode()
                      << ", error info: " << rpc_controller.ErrorText() << std::endl;
            return;
        }

        std::cout << ", response body: " << rpc_res.ShortDebugString() << std::endl;

        co::sleep(100);
    }
}

int main(int argc, char *argv[]) {
    flag::parse(argc, argv);
    // go(test_call);

    for (int i = 0; i < 16; ++i) {
        go(test_client);
        co::sleep(1);
    }

    sleep::sec(2);
    return 0;
}
