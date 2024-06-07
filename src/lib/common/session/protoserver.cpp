// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "protoserver.h"

using MessageHandler = std::function<void(const proto::OriginMessage &request, proto::OriginMessage *response)>;

class ProtoSession : public CppServer::Asio::TCPSession, public FBE::proto::Sender, public FBE::proto::Receiver
{
public:
    using CppServer::Asio::TCPSession::TCPSession;

    void setMessageHandler(MessageHandler cb)
    {
        _msghandler = std::move(cb);
    }

protected:

    // Protocol handlers
    void onReceive(const ::proto::DisconnectRequest &request) override
    {
        std::cout << "DisconnectRequest: " <<  request << std::endl;

        Disconnect();
    }

    void onReceive(const ::proto::OriginMessage &request) override
    {
        //std::cout << "Server Received: " << request << std::endl;

        // Validate request
        if (request.json_msg.empty()) {
            // Send reject
            proto::MessageReject reject;
            reject.id = request.id;
            reject.error = "Request message is empty!";
            send(reject);
            return;
        }

        // Send response
        proto::OriginMessage response;

        if (_msghandler) {
            _msghandler(request, &response);
        } else {
            response.id = request.id;
            response.mask = request.mask;
            response.json_msg = request.json_msg;
        }

        if (!response.json_msg.empty())
            send(response);
    }

    // Protocol implementation
    void onReceived(const void *buffer, size_t size) override
    {
        receive(buffer, size);
    }

    size_t onSend(const void *data, size_t size) override
    {
        return SendAsync(data, size) ? size : 0;
    }

private:
    MessageHandler _msghandler { nullptr };
};


bool ProtoServer::hasConnected(const std::string &ip)
{
    // Try to find the required ip
    auto it = _session_ids.find(ip);
    if (it != _session_ids.end()) {
        return true;
    }
    return false;
}

std::shared_ptr<CppServer::Asio::TCPSession>
ProtoServer::CreateSession(const std::shared_ptr<CppServer::Asio::TCPServer> &server)
{
    // data and state handle callback
    MessageHandler msg_cb([this](const proto::OriginMessage &request, proto::OriginMessage *response) {
        // rpc from server, notify the response to request.get()
        if (_self_request.load(std::memory_order_relaxed)) {
            _self_request.store(false, std::memory_order_relaxed);
            Client::onReceiveResponse(request);
            return;
        }

        _callbacks->onReceivedMessage(request, response);
    });

    auto session = std::make_shared<ProtoSession>(server);
    session->setMessageHandler(msg_cb);

    return session;
}

void ProtoServer::onError(int error, const std::string &category, const std::string &message)
{
    std::cout << "Protocol server caught an error with code " << error << " and category '" << category << "': " << message << std::endl;

    std::string err(message);
    _callbacks->onStateChanged(RPC_ERROR, err);
}

void ProtoServer::onConnected(std::shared_ptr<CppServer::Asio::TCPSession>& session)
{
    //std::cout << "onConnected from:" << session->socket().remote_endpoint() << std::endl;
    std::string addr = session->socket().remote_endpoint().address().to_string();
    std::shared_lock<std::shared_mutex> locker(_sessionids_lock);
    _session_ids.insert(std::make_pair(addr, session->id()));

    _callbacks->onStateChanged(RPC_CONNECTED, addr);
}

void ProtoServer::onDisconnected(std::shared_ptr<CppServer::Asio::TCPSession>& session)
{
    //std::cout << "onDisconnected from: id: " << session->id() << std::endl;

    auto search_uuid = session->id();
    auto it = std::find_if(_session_ids.begin(), _session_ids.end(), [search_uuid](const std::pair<std::string, CppCommon::UUID>& pair) {
        return pair.second == search_uuid;
    });

    std::string addr = "";
    if (it != _session_ids.end()) {
        //std::cout << "find connected by uuid, ip：" << it->first << std::endl;
        addr = it->first;
        _session_ids.erase(it);
    } else {
        std::cout << "did not find connected id:" << search_uuid << std::endl;
        return;
    }

    _callbacks->onStateChanged(RPC_DISCONNECTED, addr);
}

// Protocol implementation
size_t ProtoServer::onSend(const void *data, size_t size)
{
    // Multicast all sessions
    if (_active_traget.empty()) {
        std::cout << "Multicast all sessions:" << std::endl;
        Multicast(data, size);
        return size;
    }

    std::shared_lock<std::shared_mutex> locker(_sessionids_lock);
    std::cout << "FindSession:" << _session_ids[_active_traget] << std::endl;
    auto session = FindSession(_session_ids[_active_traget]);
    if (session) {
        session->SendAsync(data, size);
    }

    _active_traget = "";
    return size;
}
