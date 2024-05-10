// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "protoserver.h"

using MessageHandler = std::function<void(const proto::OriginMessage &request, proto::OriginMessage *response)>;
using StateHandler = std::function<void(int state, std::string msg)>;

class ProtoSession : public CppServer::Asio::TCPSession, public FBE::proto::Sender, public FBE::proto::Receiver
{
public:
    using CppServer::Asio::TCPSession::TCPSession;

    void setMessageHandler(MessageHandler cb)
    {
        _msghandler = std::move(cb);
    }

    void setStateHandler(StateHandler cb)
    {
        _statehandler = std::move(cb);
    }

protected:
    void onConnected() override
    {
        //std::cout << "Protocol session with Id " << id() << " connected!" << std::endl;
        //std::cout << "server:" << server()->address() << ":" << server()->port() << std::endl;
        //std::cout << "from:" << socket().remote_endpoint() << std::endl;
        if (_statehandler) {
            std::string addr = socket().remote_endpoint().address().to_string();
            _statehandler(RPC_CONNECTED, addr);
        }

        // Send invite notification
//        proto::MessageNotify notify;
//        notify.notification = "Hello from protocol server!";
//        send(notify);
    }

    void onDisconnected() override
    {
        //std::cout << "Protocol session with Id " << id() << " disconnected!" << std::endl;
        if (_statehandler) {
            std::string addr = socket().remote_endpoint().address().to_string();
            _statehandler(RPC_DISCONNECTED, addr);
        }
    }

    void onError(int error, const std::string &category, const std::string &message) override
    {
        std::cout << "Protocol session caught an error with code " << error << " and category '" << category << "': " << message << std::endl;
        if (_statehandler) {
            std::string err(message);
            _statehandler(RPC_ERROR, err);
        }
    }

    // Protocol handlers
    void onReceive(const ::proto::DisconnectRequest &request) override
    {
        if (_statehandler) {
            std::string addr = socket().remote_endpoint().address().to_string();
            _statehandler(RPC_DISCONNECTING, addr);
        }
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
    StateHandler _statehandler { nullptr };
};

//ProtoServer::ProtoServer()
//{

//}

void ProtoServer::setCallbacks(std::shared_ptr<SessionCallInterface> callbacks)
{
    _callbacks = callbacks;
}

bool ProtoServer::hasConnected(const std::string &ip)
{
    // Try to find the required ip
    auto it = _session_ids.find(ip);
    if (it != _session_ids.end()) {
        return true;
    }
    return false;
}

proto::OriginMessage ProtoServer::sendRequest(const std::string &target, const proto::OriginMessage &msg)
{
    _active_traget = target;
    _self_request.store(true, std::memory_order_relaxed);
    auto response = Client::request(msg).get();
    //std::cout << "ProtoServer::sendRequest response: " << response << std::endl;
    return response;
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
        if (_callbacks)
            _callbacks->onReceivedMessage(request, response);
    });
    StateHandler state_cb([this](int state, std::string msg) {
        if (_callbacks)
            _callbacks->onStateChanged(state, msg);
    });

    auto session = std::make_shared<ProtoSession>(server);
    session->setMessageHandler(msg_cb);
    session->setStateHandler(state_cb);

    return session;
}

void ProtoServer::onError(int error, const std::string &category, const std::string &message)
{
    std::cout << "Protocol server caught an error with code " << error << " and category '" << category << "': " << message << std::endl;
}

void ProtoServer::onConnected(std::shared_ptr<CppServer::Asio::TCPSession>& session)
{
    //std::cout << "onConnected from:" << session->socket().remote_endpoint() << std::endl;
    std::string addr = session->socket().remote_endpoint().address().to_string();
    std::shared_lock<std::shared_mutex> locker(_sessionids_lock);
    _session_ids.insert(std::make_pair(addr, session->id()));
}

void ProtoServer::onDisconnected(std::shared_ptr<CppServer::Asio::TCPSession>& session)
{
    //std::cout << "onDisconnected from:" << session->socket().remote_endpoint() << std::endl;
    std::string addr = session->socket().remote_endpoint().address().to_string();
    std::shared_lock<std::shared_mutex> locker(_sessionids_lock);
    auto it = _session_ids.find(addr);
    if (it != _session_ids.end())
    {
        _session_ids.erase(it);
    }
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
