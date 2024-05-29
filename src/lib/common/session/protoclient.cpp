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
    _connect_replay = false;
    DisconnectAsync();
    while (IsConnected())
        CppCommon::Thread::Yield();
}

bool ProtoClient::hasConnected(const std::string &ip)
{
    return ip == _connected_host;
}

bool ProtoClient::connectReplyed()
{
    return _connect_replay;
}

proto::OriginMessage ProtoClient::sendRequest(const proto::OriginMessage &msg)
{
    _self_request.store(true, std::memory_order_relaxed);
    auto response = Client::request(msg).get();
    //std::cout << "ProtoClient::sendRequest response: " << response << std::endl;
    return response;
}

void ProtoClient::onConnected()
{
//    std::cout << "Protocol client connected a new session with Id " << id() << " ip:" << address() << std::endl;
    _connect_replay = true;

    // Reset FBE protocol buffers
    reset();

    _connected_host = socket().remote_endpoint().address().to_string();
    if (_callbacks) {
        _callbacks->onStateChanged(RPC_CONNECTED, _connected_host);
    }
}

void ProtoClient::onDisconnected()
{
//    std::cout << "Protocol client disconnected a session with Id " << id() << std::endl;
    _connect_replay = true;

    bool retry = true;
    if (_callbacks) {
        //can not get the remote address if has not connected yet.
         retry = _callbacks->onStateChanged(RPC_DISCONNECTED, _connected_host);
    }
    _connected_host = "";

    if (retry) {
        // Wait for a while...
        CppCommon::Thread::Sleep(1000);

        // Try to connect again
        if (!_stop)
            ConnectAsync();
    }
}

void ProtoClient::onError(int error, const std::string &category, const std::string &message)
{
    std::cout << "Protocol client caught an error with code " << error << " and category '" << category << "': " << message << std::endl;
    _connect_replay = true;
    if (_callbacks) {
        std::string err = std::to_string(error);
        _callbacks->onStateChanged(RPC_ERROR, err);
    }
}

// Protocol handlers
void ProtoClient::onReceive(const ::proto::DisconnectRequest &request)
{
    Client::onReceive(request);
    //std::cout << "Received disconnect: " << request << std::endl;
    if (_callbacks) {
        std::string addr = socket().remote_endpoint().address().to_string();
        _callbacks->onStateChanged(RPC_DISCONNECTING, addr);
    }
    DisconnectAsync();
}

void ProtoClient::onReceive(const ::proto::OriginMessage &response)
{
    // notify response if the request from myself, request.get()
    if (_self_request.load(std::memory_order_relaxed)) {
        _self_request.store(false, std::memory_order_relaxed);
        Client::onReceiveResponse(response);
        return;
    }

    // Send response
    proto::OriginMessage reply;

    if (_callbacks) {
        _callbacks->onReceivedMessage(response, &reply);
    } else {
        reply.id = response.id;
        reply.mask = response.mask;
        reply.json_msg = response.json_msg;
    }

    if (!reply.json_msg.empty())
        send(reply);
}

void ProtoClient::onReceive(const ::proto::MessageReject &reject)
{
    Client::onReceive(reject);
    //std::cout << "Received reject: " << reject << std::endl;
}

void ProtoClient::onReceive(const ::proto::MessageNotify &notify)
{
    Client::onReceive(notify);
    //std::cout << "Received notify: " << notify << std::endl;
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
