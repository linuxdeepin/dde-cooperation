// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "protoserver.h"

using MessageHandler = std::function<void(const proto::OriginMessage &request, proto::OriginMessage *response)>;
using NotifyHandler = std::function<void(const std::string &addr)>;

class ProtoSession : public CppServer::Asio::SSLSession, public FBE::proto::FinalClient
{
public:
    using CppServer::Asio::SSLSession::SSLSession;

    void setMessageHandler(MessageHandler cb)
    {
        _msghandler = std::move(cb);
    }

    void setNotifyHandler(NotifyHandler cb)
    {
        _notifyhandler = std::move(cb);
    }

protected:
    void onHandshaked() override
    {
        // std::cout << "Proto SSL session with Id " << id() << " handshaked!" << std::endl;

        // Don't send anything message for proto at here
        // std::string message("Hello from SSL server!");
        // SendAsync(message.data(), message.size());
    }

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

    void onReceive(const ::proto::MessageNotify &notify) override
    {
        // FinalClient::onReceive(notify);
        // std::cout << "Session received notify: " << notify << std::endl;

        // Send response
        proto::MessageNotify pong;
        pong.notification = "pong";
        send(pong);

        if (_notifyhandler) {
            std::string addr = socket().remote_endpoint().address().to_string();
            _notifyhandler(addr);
        }
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
    NotifyHandler _notifyhandler { nullptr };
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

bool ProtoServer::startHeartbeat()
{
    if (!_ping_timer) {
        _ping_timer = std::make_shared<Timer>(service());
        _ping_timer->Setup([this](bool canceled) {
            if (canceled) {
                // std::cout << "Timer canceled!" << std::endl;
            } else {
                bool recheck = false;
                auto it = _ping_remotes.begin();
                while (it != _ping_remotes.end()) {
                    if (!it->second.exchange(false)) {
                        // 处理心跳超时的情况
                        std::string remote = it->first;
                        it = _ping_remotes.erase(it);

                        onHeartbeatTimeout(remote);
                    } else {
                        recheck = true;
                        ++it;
                    }
                }

                if (_session_ids.empty()) {
                    // no any connection session
                    _ping_timer->Cancel();
                    _ping_remotes.clear();
                } else if (recheck) {
                    _ping_timer->Setup(CppCommon::Timespan::seconds(HEARTBEAT_INTERVAL));
                    _ping_timer->WaitAsync();
                }
            }
        });
    }

    // wait for client ping
    _ping_timer->Setup(CppCommon::Timespan::seconds(HEARTBEAT_INTERVAL));
    return _ping_timer->WaitAsync();
}

void ProtoServer::handlePing(const std::string &remote)
{
    // std::cout << "server ping: " << remote << std::endl;
    auto pinging = _ping_remotes.find(remote);
    if (pinging != _ping_remotes.end()) {
        pinging->second.store(true);
    } else {
        if (_ping_remotes.empty()) {
            startHeartbeat(); // start hearbeat check while receive client's ping
        }
        _ping_remotes.insert(std::make_pair(remote, true));
    }
}

void ProtoServer::onHeartbeatTimeout(const std::string &remote)
{
    // std::cout << "Not receive client ping in 3 seconds: " << remote << std::endl;

    // disconenct and remove
    auto it = _session_ids.find(remote);
    if (it != _session_ids.end()) {
        auto session = FindSession(it->second);
        if (session) {
            session->Disconnect();
        }
        _session_ids.erase(it);
    }

    auto ip = remote;
    _callbacks->onStateChanged(RPC_PINGOUT, ip);
}

std::shared_ptr<CppServer::Asio::SSLSession>
ProtoServer::CreateSession(const std::shared_ptr<CppServer::Asio::SSLServer> &server)
{
    // data and state handle callback
    MessageHandler msg_cb([this](const proto::OriginMessage &request, proto::OriginMessage *response) {
        // rpc from server, notify the response to request.get()
        if (_self_request.load(std::memory_order_relaxed)) {
            _self_request.store(false, std::memory_order_relaxed);
            FinalClient::onReceiveResponse(request);
            return;
        }

        _callbacks->onReceivedMessage(request, response);
    });

    NotifyHandler nft_cb([this](const std::string &addr) {
        handlePing(addr);
    });

    auto session = std::make_shared<ProtoSession>(server);
    session->setMessageHandler(msg_cb);
    session->setNotifyHandler(nft_cb);

    return session;
}

void ProtoServer::onError(int error, const std::string &category, const std::string &message)
{
    // std::cout << "Protocol server caught an error with code " << error << " and category '" << category << "': " << message << std::endl;

    std::string err(message);
    _callbacks->onStateChanged(RPC_ERROR, err);
}

void ProtoServer::onConnected(std::shared_ptr<CppServer::Asio::SSLSession>& session)
{
    // std::cout << "onConnected from:" << session->socket().remote_endpoint() << std::endl;
    std::string addr = session->socket().remote_endpoint().address().to_string();
    std::shared_lock<std::shared_mutex> locker(_sessionids_lock);
    _session_ids.insert(std::make_pair(addr, session->id()));

    _callbacks->onStateChanged(RPC_CONNECTED, addr);
}

void ProtoServer::onDisconnected(std::shared_ptr<CppServer::Asio::SSLSession>& session)
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
    // std::cout << "FindSession:" << _session_ids[_active_traget] << std::endl;
    auto session = FindSession(_session_ids[_active_traget]);
    if (session) {
        session->SendAsync(data, size);
    }

    _active_traget = "";
    return size;
}
