// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "frontendservice.h"
#include "common/constant.h"
#include "common/commonstruct.h"
#include "ipc/proto/comstruct.h"

#include "co/co.h"
#include "co/time.h"

using namespace ipc;

FrontendService::FrontendService(QObject *parent)
    : QObject(parent)
{
    // 发送请求，长度为10，300ms超时
    _bridge_chan = new co::chan<BridgeJsonData>(10, 300);
    // 读取结果，长度为1，100ms超时
    _bridge_result = new co::chan<BridgeJsonData>(1, 100);
}

FrontendService::~FrontendService()
{
    if (_bridge_chan) {
        _bridge_chan->close();
    }
    if (_bridge_result) {
        _bridge_result->close();
    }
}

co::chan<BridgeJsonData>* FrontendService::bridgeChan()
{
    return _bridge_chan;
}

co::chan<BridgeJsonData>* FrontendService::bridgeResult()
{
    return _bridge_result;
}


void FrontendImpl::ping(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = IPC_PING;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // wait for result
    BridgeJsonData result;
    _interface->bridgeResult()->operator>>(result);
    bool ok = _interface->bridgeResult()->done();

    res = {
        { "result", ok },
        { "msg", result.json }
    };
}

void FrontendImpl::cbPeerInfo(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = FRONT_PEER_CB;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "ok" }
    };
}

void FrontendImpl::cbConnect(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = FRONT_CONNECT_CB;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "" }
    };
}

void FrontendImpl::cbMiscMessage(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = MISC_MSG;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "" }
    };
}

void FrontendImpl::cbTransStatus(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = FRONT_TRANS_STATUS_CB;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "" }
    };
}

void FrontendImpl::cbFsPull(co::Json &req, co::Json &res)
{

}

void FrontendImpl::cbFsAction(co::Json &req, co::Json &res)
{

}

void FrontendImpl::notifyFileStatus(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = FRONT_NOTIFY_FILE_STATUS;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "" }
    };
}

void FrontendImpl::applyTransFiles(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = FRONT_APPLY_TRANS_FILE;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "" }
    };
}

void FrontendImpl::notifySendStatus(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = FRONT_SEND_STATUS;
    // 使用结构体 SendStatus
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "" }
    };
}

void FrontendImpl::backendServerOnline(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = FRONT_SERVER_ONLINE;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "" }
    };
}

void FrontendImpl::shareEvents(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    ShareEvents event;
    event.from_json(req);
    bridge.type = event.eventType;
    bridge.json = event.data;
    _interface->bridgeChan()->operator<<(bridge);

    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "" }
    };
}

void FrontendImpl::cbDisConnect(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = FRONT_DISCONNECT_CB;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);
    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "" }
    };
}
