// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "protoserver.h"

using MessageHandler = std::function<void(const proto::OriginMessage &request, proto::OriginMessage *response)>;
using NotifyHandler = std::function<void(const std::string &addr)>;
using RealIPHandler = std::function<void(const std::string &real_ip, const std::string &remote_ip)>;
using TunnelHandler = std::function<bool(const std::string &remote, const proto::OriginMessage &request)>;

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

    void setTunnelHandler(TunnelHandler cb)
    {
        _tunnelhandler = std::move(cb);
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

        // Tunnel frames are a raw byte channel. They must be routed before the
        // JSON validation and before the RPC response matching logic.
        std::string remote;
        try {
            remote = socket().remote_endpoint().address().to_string();
        } catch (...) {
        }
        if (_tunnelhandler && _tunnelhandler(remote, request))
            return;

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
    TunnelHandler _tunnelhandler { nullptr };
};


bool ProtoServer::hasConnected(const std::string &ip)
{
    bool found = false;

    // Use consistent lock ordering to avoid deadlock: always acquire mapping_lock before session_lock
    {
        std::shared_lock<std::shared_mutex> mapping_locker(_ipmapping_lock);
        std::shared_lock<std::shared_mutex> session_locker(_sessionids_lock);

        found = _session_ids.find(ip) != _session_ids.end();

        // If not found, try via IP mapping (for NAT/Router scenarios)
        if (!found) {
            auto real_to_remote = _real_to_remote_ip.find(ip);
            if (real_to_remote != _real_to_remote_ip.end())
                found = _session_ids.find(real_to_remote->second) != _session_ids.end();
        }
    }

    if (!found) {
        std::cout << "hasConnected: " << ip << " false" << std::endl;
        return false;
    }

    // A registered session may still be completing its TLS handshake; onSend
    // reports that as a send failure, which lets the caller retry.
    std::cout << "hasConnected: " << ip << " true" << std::endl;
    return true;
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
        // A response is matched by its request id. A message that does not
        // match any pending request was initiated by the peer and is handled
        // as such, even while another RPC is still waiting for its result.
        if (FinalClient::onReceiveResponse(request))
            return;

        _callbacks->onReceivedMessage(request, response);
    });

    NotifyHandler nft_cb([this](const std::string &addr) {
        handlePing(addr);
    });

    RealIPHandler realip_cb([this](const std::string &real_ip, const std::string &remote_ip) {
        handleRealIPMapping(real_ip, remote_ip);
    });

    TunnelHandler tunnel_cb([this](const std::string &remote, const proto::OriginMessage &request) {
        return _callbacks && _callbacks->onTunnelMessage(remote, request);
    });

    auto session = std::make_shared<ProtoSession>(server);
    session->setMessageHandler(msg_cb);
    session->setNotifyHandler(nft_cb);
    session->setRealIPHandler(realip_cb);
    session->setTunnelHandler(tunnel_cb);

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
        // Keep every session: a reconnect can register a new session before
        // the old one is torn down. Routing always picks the newest one.
        _session_ids.emplace(addr, session->id());
    }

    _callbacks->onStateChanged(RPC_CONNECTED, addr);
}

void ProtoServer::onDisconnected(std::shared_ptr<NetUtil::Asio::SSLSession>& session)
{
    // std::cout << "onDisconnected from: id: " << session->id() << std::endl;

    auto search_uuid = session->id();
    std::string addr = "";
    std::string real_ip = "";
    bool replaced = false;

    // Use consistent lock ordering: mapping_lock before session_lock to avoid deadlock
    {
        std::unique_lock<std::shared_mutex> mapping_locker(_ipmapping_lock);
        std::unique_lock<std::shared_mutex> session_locker(_sessionids_lock);

        // Find and remove session
        auto it = std::find_if(_session_ids.begin(), _session_ids.end(), [search_uuid](const std::pair<std::string, BaseKit::UUID>& pair) {
            return pair.second == search_uuid;
        });

        if (it != _session_ids.end()) {
            addr = it->first;
            _session_ids.erase(it);

            // A newer session for the same peer may already be connected.
            // Keep its routing state and do not report the peer as gone.
            replaced = _session_ids.find(addr) != _session_ids.end();

            if (!replaced) {
                // Clean up IP mapping for this remote endpoint
                auto remote_to_real = _remote_to_real_ip.find(addr);
                if (remote_to_real != _remote_to_real_ip.end()) {
                    real_ip = remote_to_real->second;
                    _real_to_remote_ip.erase(real_ip);
                    _remote_to_real_ip.erase(addr);
                    std::cout << "Cleaned up IP mapping for disconnected session: " << real_ip << " -> " << addr << std::endl;
                }
            }
        } else {
            std::cout << "did not find connected id:" << search_uuid << std::endl;
            return;
        }
    }

    if (replaced) {
        std::cout << "Session superseded, keep connection state for: " << addr << std::endl;
        return;
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

    std::string target_ip = _active_target;
    _active_target.clear();

    BaseKit::UUID sessionId;
    bool found = false;

    // Use consistent lock ordering: mapping_lock before session_lock. Prefer
    // the newest session when a reconnecting peer has both an old and a new
    // one registered.
    {
        std::shared_lock<std::shared_mutex> mapping_locker(_ipmapping_lock);
        std::shared_lock<std::shared_mutex> session_locker(_sessionids_lock);

        auto newestFor = [this](const std::string &addr, BaseKit::UUID *id) {
            auto range = _session_ids.equal_range(addr);
            if (range.first == range.second)
                return false;
            auto newest = range.second;
            --newest;
            *id = newest->second;
            return true;
        };

        found = newestFor(target_ip, &sessionId);

        // If not found, try via IP mapping (for NAT/Router scenarios)
        if (!found) {
            auto real_to_remote = _real_to_remote_ip.find(target_ip);
            if (real_to_remote != _real_to_remote_ip.end())
                found = newestFor(real_to_remote->second, &sessionId);
        }
    }

    if (!found) {
        std::cout << "No session found for target: " << target_ip << std::endl;
        return 0;
    }

    auto session = FindSession(sessionId);
    if (!session || !session->IsHandshaked()) {
        std::cout << "Session not ready for target: " << target_ip << std::endl;
        return 0;
    }

    // Report failure instead of pretending the frame was delivered; the FBE
    // request then fails its future instead of hanging until timeout.
    return session->SendAsync(data, size) ? size : 0;
}
