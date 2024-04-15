// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PROTOSERVER_H
#define PROTOSERVER_H

#include "asioservice.h"

#include "server/asio/tcp_server.h"

#include "proto_protocol.h"

#include <iostream>

class ProtoSession : public CppServer::Asio::TCPSession, public FBE::proto::Sender, public FBE::proto::Receiver
{
public:
    using CppServer::Asio::TCPSession::TCPSession;

protected:
    void onConnected() override;

    void onDisconnected() override;

    void onError(int error, const std::string &category, const std::string &message) override;

    // Protocol handlers
    void onReceive(const ::proto::DisconnectRequest &request) override;
    void onReceive(const ::proto::MessageRequest &request) override;

    // Protocol implementation
    void onReceived(const void *buffer, size_t size) override;
    size_t onSend(const void *data, size_t size) override;
};

class ProtoServer : public CppServer::Asio::TCPServer, public FBE::proto::Sender
{
public:
    using CppServer::Asio::TCPServer::TCPServer;

    //    ProtoServer();

protected:
    std::shared_ptr<CppServer::Asio::TCPSession> CreateSession(const std::shared_ptr<CppServer::Asio::TCPServer> &server) override;

protected:
    void onError(int error, const std::string &category, const std::string &message) override;

    // Protocol implementation
    size_t onSend(const void *data, size_t size) override;
};

#endif // PROTOSERVER_H
