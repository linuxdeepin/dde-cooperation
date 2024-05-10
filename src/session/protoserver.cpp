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
//    void onConnected() override;

//    void onDisconnected() override;

//    void onError(int error, const std::string &category, const std::string &message) override;

//    // Protocol handlers
//    void onReceive(const ::proto::DisconnectRequest &request) override;
//    void onReceive(const ::proto::OriginMessage &request) override;

//    // Protocol implementation
//    void onReceived(const void *buffer, size_t size) override;
//    size_t onSend(const void *data, size_t size) override;

    void onConnected() override
    {
        std::cout << "Protocol session with Id " << id() << " connected!" << std::endl;
        std::cout << "server:" << server()->address() << ":" << server()->port() << std::endl;
        std::cout << "from:" << socket().remote_endpoint() << std::endl;
        if (_statehandler) {
            std::string addr = socket().remote_endpoint().address().to_string();
            _statehandler(RPC_CONNECTED, addr);
        }

        // Send invite notification
        proto::MessageNotify notify;
        notify.notification = "Hello from protocol server!";
        send(notify);
    }

    void onDisconnected() override
    {
        std::cout << "Protocol session with Id " << id() << " disconnected!" << std::endl;
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
        std::cout << "Received: " << request << std::endl;

        // Validate request
        if (request.json_msg.empty()) {
            // Send reject
            proto::MessageReject reject;
            reject.id = request.id;
            reject.error = "Request message is empty!";
            send(reject);
            return;
        }

//        static std::hash<std::string> hasher;

        // Send response
        proto::OriginMessage response;

        if (_msghandler) {
            _msghandler(request, &response);
        } else {
            response.id = request.id;
            response.mask = request.mask;
            response.json_msg = request.json_msg;
        }

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

void ProtoServer::sendRequest(const std::string &target, const proto::OriginMessage &request)
{
    send(request);
//    for (auto& session : _sessions)
//        session.second->SendAsync(data, size);
}

std::shared_ptr<CppServer::Asio::TCPSession>
ProtoServer::CreateSession(const std::shared_ptr<CppServer::Asio::TCPServer> &server)
{
    // data and state handle callback
    MessageHandler msg_cb([this](const proto::OriginMessage &request, proto::OriginMessage *response) {
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

//    return std::make_shared<HTTPCacheSession>(std::dynamic_pointer_cast<ProtoServer>(server));

    return session;
}

void ProtoServer::onError(int error, const std::string &category, const std::string &message)
{
    std::cout << "Protocol server caught an error with code " << error << " and category '" << category << "': " << message << std::endl;
}

// Protocol implementation
size_t ProtoServer::onSend(const void *data, size_t size)
{
    // Multicast all sessions
    for (auto& session : _sessions)
        session.second->SendAsync(data, size);
    Multicast(data, size); return size;
}
