// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "protoserver.h"

using MessageHandler = std::function<void(const proto::OriginMessage &request, proto::OriginMessage *response)>;
using NotifyHandler = std::function<void(const std::string &addr)>;
using RealIPHandler = std::function<void(const std::string &real_ip, const std::string &remote_ip)>;

class ProtoSession : public NetUtil::Asio::SSLSession, public FBE::proto::FinalClient
{
public:
    using NetUtil::Asio::SSLSession::SSLSession;

    void setMessageHandler(MessageHandler cb)
    {
        _msghandler = std::move(cb);
    }

    void setNotifyHandler(NotifyHandler cb)
    {
        _notifyhandler = std::move(cb);
    }

    void setRealIPHandler(RealIPHandler cb)
    {
        _realip_handler = std::move(cb);
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

        // Check if this is a real IP notification (format: "real_ip:<ip_address>")
        if (notify.notification.substr(0, 8) == "real_ip:") {
            std::string real_ip = notify.notification.substr(8);
            std::string remote_ip = socket().remote_endpoint().address().to_string();

            // std::cout << "Received real IP notification: " << real_ip << " from remote: " << remote_ip << std::endl;

            if (_realip_handler) {
                _realip_handler(real_ip, remote_ip);
            }

            // Send acknowledgment
            proto::MessageNotify ack;
            ack.notification = "real_ip_ack";
            send(ack);
            return;
        }

        // Send response for ping
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
    RealIPHandler _realip_handler { nullptr };
};


bool ProtoServer::hasConnected(const std::string &ip)
{
    // std::cout << "check hasConnected: " << ip << std::endl;

    // Use consistent lock ordering to avoid deadlock: always acquire mapping_lock before session_lock
    std::shared_lock<std::shared_mutex> mapping_locker(_ipmapping_lock);
    std::shared_lock<std::shared_mutex> session_locker(_sessionids_lock);

    // print all session ids
    // for (auto &session : _session_ids) {
    //     std::cout << "session ip: " << session.first << " id: " << session.second << std::endl;
    // }

    // First try to find by direct IP match
    auto direct_it = _session_ids.find(ip);
    if (direct_it != _session_ids.end()) {
        std::cout << "hasConnected: " << ip << " true (direct match)" << std::endl;
        return true;
    }

    // If not found, try to find via IP mapping (for NAT/Router scenarios)
    // Check if this IP is a real IP that maps to a remote endpoint IP
    auto real_to_remote = _real_to_remote_ip.find(ip);
    if (real_to_remote != _real_to_remote_ip.end()) {
        auto session_it = _session_ids.find(real_to_remote->second);
        if (session_it != _session_ids.end()) {
            std::cout << "hasConnected: " << ip << " true (via IP mapping: " << real_to_remote->second << ")" << std::endl;
            return true;
        }
    }

    std::cout << "hasConnected: " << ip << " false" << std::endl;
    return false;
}

bool ProtoServer::startHeartbeat()
{
    if (!_ping_timer) {
        _ping_timer = std::make_shared<Timer>(service());

        std::function<void(bool)> _action = std::bind(&ProtoServer::onHeartbeatTimeout, this, std::placeholders::_1);
        _ping_timer->Setup(_action);
    }

    // wait for client ping
    _ping_timer->Setup(BaseKit::Timespan::seconds(HEARTBEAT_INTERVAL));
    return _ping_timer->WaitAsync();
}

void ProtoServer::handlePing(const std::string &remote)
{
    // std::cout << "server ping: " << remote << std::endl;
    auto pinging = _ping_remotes.find(remote);
    if (pinging != _ping_remotes.end()) {
        pinging->second.store(0);
    } else {
        if (_ping_remotes.empty()) {
            startHeartbeat(); // start hearbeat check while receive client's ping
        }
        _ping_remotes.insert(std::make_pair(remote, 0));
    }
}

void ProtoServer::handleRealIPMapping(const std::string &real_ip, const std::string &remote_ip)
{
    // std::cout << "Setting up IP mapping: real_ip=" << real_ip << " -> remote_ip=" << remote_ip << std::endl;

    std::unique_lock<std::shared_mutex> locker(_ipmapping_lock);

    // Remove old mapping if exists
    auto old_remote = _real_to_remote_ip.find(real_ip);
    if (old_remote != _real_to_remote_ip.end()) {
        _remote_to_real_ip.erase(old_remote->second);
    }

    auto old_real = _remote_to_real_ip.find(remote_ip);
    if (old_real != _remote_to_real_ip.end()) {
        _real_to_remote_ip.erase(old_real->second);
    }

    // Add new mapping
    _real_to_remote_ip[real_ip] = remote_ip;
    _remote_to_real_ip[remote_ip] = real_ip;

    std::cout << "IP mapping established successfully" << std::endl;
}

void ProtoServer::onHeartbeatTimeout(bool canceled)
{
    if (_session_ids.empty() || canceled) {
        // no any connection session
        _ping_timer->Cancel();
        _ping_remotes.clear();
        return;
    }

    std::string outip = "";
    bool recheck = false;
    auto it = _ping_remotes.begin();
    while (it != _ping_remotes.end()) {
        auto count = it->second.load();
        if (count < 3) {
            recheck = true;
            ++it;
        } else {
            outip = it->first;
            it = _ping_remotes.erase(it);
            std::cout << "Not receive client ping in 3 times: " << outip << std::endl;

            if (_callbacks) {
                _callbacks->onStateChanged(RPC_PINGOUT, outip);
            }
        }
    }

    if (recheck) {
        _ping_timer->Setup(BaseKit::Timespan::seconds(HEARTBEAT_INTERVAL));
        _ping_timer->WaitAsync();
    }
}

std::shared_ptr<NetUtil::Asio::SSLSession>
ProtoServer::CreateSession(const std::shared_ptr<NetUtil::Asio::SSLServer> &server)
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

    RealIPHandler realip_cb([this](const std::string &real_ip, const std::string &remote_ip) {
        handleRealIPMapping(real_ip, remote_ip);
    });

    auto session = std::make_shared<ProtoSession>(server);
    session->setMessageHandler(msg_cb);
    session->setNotifyHandler(nft_cb);
    session->setRealIPHandler(realip_cb);

    return session;
}

void ProtoServer::onError(int error, const std::string &category, const std::string &message)
{
    // std::cout << "Protocol server caught an error with code " << error << " and category '" << category << "': " << message << std::endl;

    std::string err(message);
    _callbacks->onStateChanged(RPC_ERROR, err);
}

void ProtoServer::onConnected(std::shared_ptr<NetUtil::Asio::SSLSession>& session)
{
    // std::cout << "Server onConnected from:" << session->socket().remote_endpoint() << std::endl;
    std::string addr = session->socket().remote_endpoint().address().to_string();

    {
        std::unique_lock<std::shared_mutex> locker(_sessionids_lock);
        // std::cout << "Server onConnected insert session ip: " << addr << " id: " << session->id() << std::endl;
        _session_ids.insert(std::make_pair(addr, session->id()));
    }

    _callbacks->onStateChanged(RPC_CONNECTED, addr);
}

void ProtoServer::onDisconnected(std::shared_ptr<NetUtil::Asio::SSLSession>& session)
{
    // std::cout << "onDisconnected from: id: " << session->id() << std::endl;

    auto search_uuid = session->id();
    std::string addr = "";
    std::string real_ip = "";
    
    // Use consistent lock ordering: mapping_lock before session_lock to avoid deadlock
    {
        std::unique_lock<std::shared_mutex> mapping_locker(_ipmapping_lock);
        std::unique_lock<std::shared_mutex> session_locker(_sessionids_lock);
        
        // Find and remove session
        auto it = std::find_if(_session_ids.begin(), _session_ids.end(), [search_uuid](const std::pair<std::string, BaseKit::UUID>& pair) {
            return pair.second == search_uuid;
        });

        if (it != _session_ids.end()) {
            //std::cout << "find connected by uuid, ip：" << it->first << std::endl;
            addr = it->first;
            _session_ids.erase(it);
            
            // Clean up IP mapping for this remote endpoint
            auto remote_to_real = _remote_to_real_ip.find(addr);
            if (remote_to_real != _remote_to_real_ip.end()) {
                real_ip = remote_to_real->second;
                _real_to_remote_ip.erase(real_ip);
                _remote_to_real_ip.erase(addr);
                std::cout << "Cleaned up IP mapping for disconnected session: " << real_ip << " -> " << addr << std::endl;
            }
        } else {
            std::cout << "did not find connected id:" << search_uuid << std::endl;
            return;
        }
    }

    _callbacks->onStateChanged(RPC_DISCONNECTED, addr);
}

// Protocol implementation
size_t ProtoServer::onSend(const void *data, size_t size)
{
    // Multicast all sessions
    if (_active_target.empty()) {
        std::cout << "Multicast all sessions:" << std::endl;
        Multicast(data, size);
        return size;
    }

    std::shared_ptr<NetUtil::Asio::SSLSession> session = nullptr;
    std::string target_ip = _active_target;

    // First try direct session lookup
    {
        std::shared_lock<std::shared_mutex> session_locker(_sessionids_lock);
        auto session_it = _session_ids.find(target_ip);
        if (session_it != _session_ids.end()) {
            session = FindSession(session_it->second);
        }
    }

    // If not found, try via IP mapping (for NAT/Router scenarios)
    if (!session) {
        // Use consistent lock ordering: mapping_lock before session_lock
        std::shared_lock<std::shared_mutex> mapping_locker(_ipmapping_lock);
        auto real_to_remote = _real_to_remote_ip.find(target_ip);
        if (real_to_remote != _real_to_remote_ip.end()) {
            std::string remote_ip = real_to_remote->second;
            std::shared_lock<std::shared_mutex> session_locker(_sessionids_lock);
            auto session_it = _session_ids.find(remote_ip);
            if (session_it != _session_ids.end()) {
                session = FindSession(session_it->second);
                std::cout << "Found session via IP mapping: " << target_ip << " -> " << remote_ip << std::endl;
            }
        }
    }

    if (session) {
        session->SendAsync(data, size);
    } else {
        std::cout << "No session found for target: " << target_ip << std::endl;
    }

    _active_target = "";
    return size;
}
