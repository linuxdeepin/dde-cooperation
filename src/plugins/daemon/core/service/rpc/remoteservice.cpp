// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <google/protobuf/service.h>
#include <sstream>
#include <atomic>

#include "remoteservice.h"
#include "co/log.h"
#include "co/co.h"
#include "co/time.h"
#include "zrpc.h"

using namespace uniapis;

void RemoteServiceImpl::login(::google::protobuf::RpcController *controller,
                              const ::uniapis::LoginRequest *request,
                              ::uniapis::LoginResponse *response,
                              ::google::protobuf::Closure *done) {
    LOG << "req= " << request->ShortDebugString().c_str();

    LOG << "res= " << response->ShortDebugString().c_str();

    if (done) {
        done->Run();
    }
}

void RemoteServiceImpl::query_peerinfo(::google::protobuf::RpcController *controller,
                                       const ::uniapis::PeerInfo *request,
                                       ::uniapis::PeerInfo *response,
                                       ::google::protobuf::Closure *done) {
}

void RemoteServiceImpl::misc(::google::protobuf::RpcController *controller,
                             const ::uniapis::Misc *request,
                             ::uniapis::Misc *response,
                             ::google::protobuf::Closure *done) {
}

void RemoteServiceImpl::fsaction(::google::protobuf::RpcController *controller,
                                 const ::uniapis::FileAction *request,
                                 ::uniapis::FileResponse *response,
                                 ::google::protobuf::Closure *done) {
}
