// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PROTOENDPOINT_H
#define PROTOENDPOINT_H

#include "session.h"
#include "asio/timer.h"

#include <atomic>
#include <iostream>
#include <mutex>

// hearbeat timeout
#define HEARTBEAT_INTERVAL 2

using NetUtil::Asio::Timer;
using RpcHandler = std::function<void(int32_t type, const std::string &response)>;

class ProtoEndpoint: public FBE::proto::FinalClient
{
public:

    void setCallbacks(std::shared_ptr<SessionCallInterface> callbacks);

    void sendDisRequest();
    proto::OriginMessage syncRequest(const std::string &target, const proto::OriginMessage &msg);

    // async call request and with 3s timeout result callback. Returns false
    // when the request could not be handed to the session at all.
    bool asyncRequestWithHandler(const std::string &target, const proto::OriginMessage &request, RpcHandler resultHandler);

    // Send a one-way frame to a specific peer. It does not create a request
    // future and must not be routed through the multicast fallback.
    void sendMessage(const std::string &target, int32_t mask, const std::string &payload);

    virtual bool hasConnected(const std::string &ip) { return false; }

protected:
    void setActiveTarget(const std::string &target) { _active_target = target; }
    void clearActiveTarget() { _active_target.clear(); }

    // Serialize access to the FBE sender buffer. The client shares one sender
    // between RPC replies, heartbeats and tunnel frames.
    template<typename T>
    void sendLocked(const T &msg)
    {
        std::lock_guard<std::mutex> locker(_send_lock);
        this->send(msg);
    }

private:
    bool asyncRequest(const std::string &target, const proto::OriginMessage &msg);
    std::string getResponse(int32_t type);

protected:
    std::shared_ptr<SessionCallInterface> _callbacks { nullptr };

    //current active request target
    std::string _active_target = { "" };

private:
    struct PendingResponse {
        std::future<proto::OriginMessage> future;
        FBE::uuid_t id;
    };

    std::mutex _lock;
    // Serializes access to the FBE sender buffer. RPC and tunnel frames can be
    // sent from different threads.
    std::mutex _send_lock;
    // type -> pending response. The request id lets a timed out request be
    // removed from the FBE table without touching other in-flight requests.
    std::unordered_map<int32_t, PendingResponse> _responses_by_type;
};

#endif // PROTOENDPOINT_H
