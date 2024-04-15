// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "protoclient.h"

//ProtoClient::ProtoClient()
//{

//}

void ProtoClient::DisconnectAndStop()
{
    _stop = true;
    DisconnectAsync();
    while (IsConnected())
        CppCommon::Thread::Yield();
}

void ProtoClient::onConnected()
{
    std::cout << "Protocol client connected a new session with Id " << id() << " ip:" << address() << std::endl;

    // Reset FBE protocol buffers
    reset();
}

void ProtoClient::onDisconnected()
{
    std::cout << "Protocol client disconnected a session with Id " << id() << std::endl;

    // Wait for a while...
    CppCommon::Thread::Sleep(1000);

    // Try to connect again
    if (!_stop)
        ConnectAsync();
}

void ProtoClient::onError(int error, const std::string &category, const std::string &message)
{
    std::cout << "Protocol client caught an error with code " << error << " and category '" << category << "': " << message << std::endl;
}

// Protocol handlers
void ProtoClient::onReceive(const ::proto::DisconnectRequest &request)
{
    Client::onReceive(request);
    std::cout << "Received: " << request << std::endl;
    DisconnectAsync();
}

void ProtoClient::onReceive(const ::proto::MessageResponse &response)
{
    Client::onReceive(response);
    std::cout << "Received: " << response << std::endl;
}

void ProtoClient::onReceive(const ::proto::MessageReject &reject)
{
    Client::onReceive(reject);
    std::cout << "Received: " << reject << std::endl;
}

void ProtoClient::onReceive(const ::proto::MessageNotify &notify)
{
    Client::onReceive(notify);
    std::cout << "Received: " << notify << std::endl;
}

// Protocol implementation
void ProtoClient::onReceived(const void *buffer, size_t size)
{
    receive(buffer, size);
}

size_t ProtoClient::onSend(const void *data, size_t size)
{
    return SendAsync(data, size) ? size : 0;
}
