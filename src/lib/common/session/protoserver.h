// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PROTOSERVER_H
#define PROTOSERVER_H

#include "asioservice.h"
#include "protoendpoint.h"

#include "asio/ssl_server.h"

class ProtoServer : public NetUtil::Asio::SSLServer, public ProtoEndpoint
{
public:
    using NetUtil::Asio::SSLServer::SSLServer;

    bool hasConnected(const std::string &ip) override;

protected:
    std::shared_ptr<NetUtil::Asio::SSLSession> CreateSession(const std::shared_ptr<NetUtil::Asio::SSLServer> &server) override;

protected:
    void onError(int error, const std::string &category, const std::string &message) override;

    void onConnected(std::shared_ptr<NetUtil::Asio::SSLSession>& session) override;

    void onDisconnected(std::shared_ptr<NetUtil::Asio::SSLSession>& session) override;

    // Protocol implementation
    size_t onSend(const void *data, size_t size) override;

private:
    bool startHeartbeat();

    void handlePing(const std::string &remote);

    void handleRealIPMapping(const std::string &real_ip, const std::string &remote_ip);

    void onHeartbeatTimeout(bool canceled);

private:
    // <ip, sessionid>
    std::shared_mutex _sessionids_lock;
    std::map<std::string, BaseKit::UUID> _session_ids;

    // IP mapping for NAT/Router scenarios: <real_ip, remote_endpoint_ip>
    std::shared_mutex _ipmapping_lock;
    std::map<std::string, std::string> _real_to_remote_ip;
    std::map<std::string, std::string> _remote_to_real_ip;

    // heartbeat: ping <-> pong
    std::shared_ptr<Timer> _ping_timer { nullptr };
    // <ip, pinged>
    std::map<std::string, std::atomic<int>> _ping_remotes;
};

#endif // PROTOSERVER_H
