// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "threads/thread.h"

#include "protoendpoint.h"

void ProtoEndpoint::setCallbacks(std::shared_ptr<SessionCallInterface> callbacks)
{
    _callbacks = callbacks;
}

void ProtoEndpoint::sendDisRequest()
{
    proto::DisconnectRequest dis;
    std::lock_guard<std::mutex> locker(_send_lock);
    this->request(dis);
}

proto::OriginMessage ProtoEndpoint::syncRequest(const std::string &target, const proto::OriginMessage &msg)
{
    std::future<proto::OriginMessage> futureResponse;
    {
        std::lock_guard<std::mutex> locker(_send_lock);
        setActiveTarget(target);
        futureResponse = this->request(msg);
        clearActiveTarget();
    }

    try {
        return futureResponse.get();
    } catch (const std::exception &ex) {
        // 捕获并处理异常
        std::cout << "syncRequest throw exception: " << ex.what() << std::endl;
    }

    return proto::OriginMessage();
}

void ProtoEndpoint::asyncRequestWithHandler(const std::string &target, const proto::OriginMessage &request, RpcHandler resultHandler)
{
    asyncRequest(target, request);  // 发送异步请求

    int32_t type = request.mask;  // 获取请求的类型
    // 启动一个新的线程来处理结果
    std::thread handlerThread([this, type, resultHandler]() {
        std::string response = getResponse(type);  // 获取响应
        resultHandler(type, response);  // 调用结果处理函数
    });

    // 在启动的新线程中处理结果，主线程可以继续执行其他任务
    handlerThread.detach();
}


void ProtoEndpoint::asyncRequest(const std::string &target, const proto::OriginMessage &msg)
{
    int32_t type = msg.mask;
    std::future<proto::OriginMessage> futureResponse;
    {
        // Do not reset pending requests here. Several independent RPCs (for
        // example a barrier update and a transfer request) can be in flight at
        // the same time; clearing the table would make their responses
        // impossible to match.
        std::lock_guard<std::mutex> locker(_send_lock);
        setActiveTarget(target);
        futureResponse = this->request(msg);
        clearActiveTarget();
    }

    std::lock_guard<std::mutex> locker(_lock);
    _responses_by_type.insert(std::make_pair(type, PendingResponse { std::move(futureResponse), msg.id }));
}

void ProtoEndpoint::sendMessage(const std::string &target, int32_t mask, const std::string &payload)
{
    proto::OriginMessage msg;
    msg.mask = mask;
    msg.json_msg = payload;

    std::lock_guard<std::mutex> locker(_send_lock);
    setActiveTarget(target);
    this->send(msg);
    clearActiveTarget();
}

std::string ProtoEndpoint::getResponse(int32_t type)
{
    PendingResponse pending;
    {
        // Take the future out of the table before waiting. Holding the lock
        // during the wait would block every new async request for up to 3s.
        std::lock_guard<std::mutex> locker(_lock);
        auto it = _responses_by_type.find(type);
        if (it == _responses_by_type.end())
            return "";

        pending = std::move(it->second);
        _responses_by_type.erase(it);
    }

    bool resolved = false;
    int wait_cout = 0;
    do {
        if (pending.future.valid()
            && pending.future.wait_for(std::chrono::milliseconds(wait_cout > 0 ? 1 : 0))
                    == std::future_status::ready) {
            // 如果有响应，立即返回
            try {
                const auto response = pending.future.get();
                resolved = true;
                return response.json_msg;
            } catch (const std::exception &ex) {
                // 捕获并处理异常
                std::cout << "future throw exception: " << ex.what() << std::endl;
                break;
            }
        }
        wait_cout++;
        BaseKit::Thread::Yield();
    } while (wait_cout < 3000); // timeout 3s

    // A request that never got an answer would otherwise stay in the FBE
    // request table forever now that new requests no longer reset it. Release
    // it through the normal response path so only this entry is removed.
    if (!resolved) {
        proto::OriginMessage expired;
        expired.id = pending.id;
        FinalClient::onReceiveResponse(expired);
    }

    return "";
}
