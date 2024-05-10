// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "protoclient.h"

//ProtoClient::ProtoClient()
//{

//}

void ProtoClient::setCallbacks(std::shared_ptr<SessionCallInterface> callbacks) {
    _callbacks = callbacks;
}

void ProtoClient::DisconnectAndStop()
{
    _stop = true;
    DisconnectAsync();
    while (IsConnected())
        CppCommon::Thread::Yield();
}

void ProtoClient::onConnected()
{
//    std::cout << "Protocol client connected a new session with Id " << id() << " ip:" << address() << std::endl;

    // Reset FBE protocol buffers
    reset();

    if (_callbacks) {
        std::string addr = socket().remote_endpoint().address().to_string();
        _callbacks->onStateChanged(RPC_CONNECTED, addr);
    }
}

void ProtoClient::onDisconnected()
{
//    std::cout << "Protocol client disconnected a session with Id " << id() << std::endl;

    if (_callbacks) {
        //can not get the remote address if has not connected yet.
         _callbacks->onStateChanged(RPC_DISCONNECTED, id().string());
    }

    // Wait for a while...
    CppCommon::Thread::Sleep(1000);

    // Try to connect again
    if (!_stop)
        ConnectAsync();
}

void ProtoClient::onError(int error, const std::string &category, const std::string &message)
{
    std::cout << "Protocol client caught an error with code " << error << " and category '" << category << "': " << message << std::endl;
    if (_callbacks) {
        std::string err = std::to_string(error);
        _callbacks->onStateChanged(RPC_ERROR, err);
    }
}

// Protocol handlers
void ProtoClient::onReceive(const ::proto::DisconnectRequest &request)
{
    Client::onReceive(request);
    std::cout << "Received: " << request << std::endl;
    if (_callbacks) {
        std::string addr = socket().remote_endpoint().address().to_string();
        _callbacks->onStateChanged(RPC_DISCONNECTING, addr);
    }
    DisconnectAsync();
}

void ProtoClient::onReceive(const ::proto::OriginMessage &response)
{
    Client::onReceive(response);
    std::cout << "Received: " << response << std::endl;
    // Send response
    proto::OriginMessage reply;

    if (_callbacks) {
        _callbacks->onReceivedMessage(response, &reply);
    } else {
        reply.id = response.id;
        reply.mask = response.mask;
        reply.json_msg = response.json_msg;
    }

    send(reply);
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
