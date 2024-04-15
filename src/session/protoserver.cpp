// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "protoserver.h"

void ProtoSession::onConnected()
{
    std::cout << "Protocol session with Id " << id() << " connected!" << std::endl;
    std::cout << "server:" << server()->address() << ":" << server()->port() << std::endl;
    std::cout << "from:" << socket().remote_endpoint() << std::endl;

    // Send invite notification
    proto::MessageNotify notify;
    notify.Notification = "Hello from protocol server!";
    send(notify);
}

void ProtoSession::onDisconnected()
{
    std::cout << "Protocol session with Id " << id() << " disconnected!" << std::endl;
}

void ProtoSession::onError(int error, const std::string &category, const std::string &message)
{
    std::cout << "Protocol session caught an error with code " << error << " and category '" << category << "': " << message << std::endl;
}

// Protocol handlers
void ProtoSession::onReceive(const ::proto::DisconnectRequest &request)
{
    Disconnect();
}

void ProtoSession::onReceive(const ::proto::MessageRequest &request)
{
    std::cout << "Received: " << request << std::endl;

    // Validate request
    if (request.Message.empty()) {
        // Send reject
        proto::MessageReject reject;
        reject.id = request.id;
        reject.Error = "Request message is empty!";
        send(reject);
        return;
    }

    static std::hash<std::string> hasher;

    // Send response
    proto::MessageResponse response;
    response.id = request.id;
    response.Hash = (uint32_t)hasher(request.Message);
    response.Length = (uint32_t)request.Message.size();
    send(response);
}

// Protocol implementation
void ProtoSession::onReceived(const void *buffer, size_t size)
{
    receive(buffer, size);
}

size_t ProtoSession::onSend(const void *data, size_t size)
{
    return SendAsync(data, size) ? size : 0;
}

//ProtoServer::ProtoServer()
//{

//}

std::shared_ptr<CppServer::Asio::TCPSession>
ProtoServer::CreateSession(const std::shared_ptr<CppServer::Asio::TCPServer> &server)
{
    return std::make_shared<ProtoSession>(server);
}

void ProtoServer::onError(int error, const std::string &category, const std::string &message)
{
    std::cout << "Protocol server caught an error with code " << error << " and category '" << category << "': " << message << std::endl;
}

// Protocol implementation
size_t ProtoServer::onSend(const void *data, size_t size)
{
    Multicast(data, size); return size;
}
