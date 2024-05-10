// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PROTOSERVER_H
#define PROTOSERVER_H

#include "asioservice.h"
#include "session.h"

#include "server/asio/tcp_server.h"

#include <iostream>

class ProtoServer : public CppServer::Asio::TCPServer, public FBE::proto::Sender
{
public:
    using CppServer::Asio::TCPServer::TCPServer;

    //    ProtoServer();
    void setCallbacks(std::shared_ptr<SessionCallInterface> callbacks);

    void sendRequest(const std::string &target, const proto::OriginMessage &request);

protected:
    std::shared_ptr<CppServer::Asio::TCPSession> CreateSession(const std::shared_ptr<CppServer::Asio::TCPServer> &server) override;

protected:
    void onError(int error, const std::string &category, const std::string &message) override;

    // Protocol implementation
    size_t onSend(const void *data, size_t size) override;

private:
    std::shared_ptr<SessionCallInterface> _callbacks { nullptr };
};

#endif // PROTOSERVER_H
