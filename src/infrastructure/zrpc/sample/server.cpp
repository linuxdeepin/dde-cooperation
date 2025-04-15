// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <google/protobuf/service.h>
#include <sstream>
#include <atomic>
#include <unistd.h>
#include <thread>
#include <chrono>

#include "zrpc.h"
#include "sample.pb.h"

static int i = 0;

class QueryServiceImpl : public QueryService {
public:
    QueryServiceImpl() {}
    ~QueryServiceImpl() {}

    void query_name(google::protobuf::RpcController *controller,
                    const ::queryNameReq *request,
                    ::queryNameRes *response,
                    ::google::protobuf::Closure *done) {

        printf("QueryServiceImpl.query_name, req={%s}\n", request->ShortDebugString().c_str());
        response->set_id(request->id());
        response->set_name("ikerli");

        printf("QueryServiceImpl.query_name, res={%s}\n", response->ShortDebugString().c_str());

        if (done) {
            done->Run();
        }
    }

    void query_age(google::protobuf::RpcController *controller,
                   const ::queryAgeReq *request,
                   ::queryAgeRes *response,
                   ::google::protobuf::Closure *done) {

        printf("QueryServiceImpl.query_age, req={%s}\n", request->ShortDebugString().c_str());

        response->set_ret_code(0);
        response->set_res_info("OK");
        response->set_req_no(request->req_no());
        response->set_id(request->id());
        response->set_age(100100111);

        printf("begin i = %d\n", i);
        // co::sleep(1000);
        i++;
        printf("end i = %d\n", i);

        if (done) {
            done->Run();
        }

        printf("QueryServiceImpl.query_age, res={%s}\n", response->ShortDebugString().c_str());
    }
};

int main(int argc, char *argv[]) {
    printf("Start RPC server\n");

    // 禁用SSL
    char key[] = "";
    char crt[] = "";

    zrpc_ns::ZRpcServer *server = new zrpc_ns::ZRpcServer(8899, key, crt);
    printf("Registering QueryService implementation...\n");
    bool regResult = server->registerService<QueryServiceImpl>();
    printf("Service registration %s\n", regResult ? "succeeded" : "failed");
    printf("Service registered successfully\n");

    if (!server->start()) {
        printf("!!! Server failed to start !!!\n");
        exit(1);
    }
    
    printf("RPC server listening..\n");
    while (true)
    {
        sleep(5);
    }

    return 0;
}
