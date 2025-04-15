// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <google/protobuf/service.h>
#include "rpcchannel.h"
#include "rpccontroller.h"
#include "netaddress.h"
#include "sample.pb.h"
#include "zrpc.h"
#include <unistd.h>
#include "zrpc.h"

void test_client() {
    // 使用localhost和禁用SSL
    printf("Connecting to server at 127.0.0.1:8899...\n");
    zrpc_ns::ZRpcClient *client = new zrpc_ns::ZRpcClient("127.0.0.1", 8899, false);
    printf("Connection timeout set to %d ms\n", client->getControler()->Timeout());
    client->setTimeout(1000); // 设置10秒超时
    QueryService_Stub stub(client->getChannel());
    zrpc_ns::ZRpcController *rpc_controller = client->getControler();

    queryAgeReq rpc_req;
    queryAgeRes rpc_res;

    rpc_req.set_id(555);
    rpc_req.set_req_no(666);

    std::cout << "requeset body: " << rpc_req.ShortDebugString() << std::endl;
    stub.query_age(rpc_controller, &rpc_req, &rpc_res, NULL);

    if (rpc_controller->ErrorCode() != 0) {
        std::cout << "Failed to call server, error code: " << rpc_controller->ErrorCode()
                  << ", error info: " << rpc_controller->ErrorText() << std::endl;
        return;
    }

    std::cout << "response body: " << rpc_res.ShortDebugString() << std::endl;
}

int main(int argc, char *argv[]) {

    for (int i = 0; i < 5; ++i) {
        test_client();
        sleep(1);
    }

    sleep(2);
    return 0;
}
