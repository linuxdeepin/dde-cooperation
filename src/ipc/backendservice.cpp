// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "backendservice.h"
#include "common/constant.h"
#include "utils/config.h"

#include "co/co.h"
#include "co/time.h"

using namespace ipc;

BackendService::BackendService(QObject *parent)
    : QObject(parent)
{
    // 发送请求，长度为10，300ms超时
    _bridge_chan = new co::chan<BridgeJsonData>(10, 300);
    // 读取结果，长度为1，100ms超时
    _bridge_result = new co::chan<BridgeJsonData>(1, 100);
}

BackendService::~BackendService()
{
    if (_bridge_chan) {
        _bridge_chan->close();
    }
    if (_bridge_result) {
        _bridge_result->close();
    }
}

co::chan<BridgeJsonData>* BackendService::bridgeChan()
{
    return _bridge_chan;
}

co::chan<BridgeJsonData>* BackendService::bridgeResult()
{
    return _bridge_result;
}

fastring BackendService::getSettingPin() const
{
    return DaemonConfig::instance()->getPin();
}

void BackendService::setSettingPin(fastring password)
{
    if (password.empty()) {
        //refresh as random password
        DaemonConfig::instance()->refreshPin();
    } else {
        DaemonConfig::instance()->setPin(password);
    }
}

fastring BackendService::getOneAppConfig(fastring &app, fastring &key) const
{
    return DaemonConfig::instance()->getAppConfig(app, key);
}

void BackendService::setOneAppConfig(fastring &app, fastring &key, fastring &value)
{
    DaemonConfig::instance()->setAppConfig(app, key, value);
}

void BackendImpl::ping(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = PING;
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

void BackendImpl::getDiscovery(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = BACK_GET_DISCOVERY;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // wait for result:{ PeerList }
    BridgeJsonData result;
    _interface->bridgeResult()->operator>>(result);
    bool ok = _interface->bridgeResult()->done();

    res = {
        { "result", ok },
        { "msg", result.json }
    };
}

void BackendImpl::getPeerInfo(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = BACK_GET_PEER;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "" }
    };
}

void BackendImpl::getPassword(co::Json &req, co::Json &res)
{
    fastring pin = _interface->getSettingPin();
    res = {
        { "password", pin }
    };
}

void BackendImpl::setPassword(co::Json &req, co::Json &res)
{
    // { "password", "pin" }
    fastring pin = req.get("password").as_string();
    _interface->setSettingPin(pin);
    res = {
        { "result", true},
        { "msg", ""}
    };
}

void BackendImpl::tryConnect(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = BACK_TRY_CONNECT;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "" }
    };
}

void BackendImpl::setAppConfig(co::Json &req, co::Json &res)
{
    fastring app = req.get("appname").as_string();
    fastring key = req.get("key").as_string();
    fastring value = req.get("value").as_string();
    _interface->setOneAppConfig(app, key, value);

    res = {
        { "result", true },
        { "msg", "" }
    };
}

void BackendImpl::getAppConfig(co::Json &req, co::Json &res)
{
    fastring app = req.get("appname").as_string();
    fastring key = req.get("key").as_string();
    fastring value = _interface->getOneAppConfig(app, key);

    res = {
        { "result", true },
        { "msg", value }
    };
}

void BackendImpl::miscMessage(co::Json &req, co::Json &res)
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

void BackendImpl::tryTransFiles(co::Json &req, co::Json &res)
{   
    BridgeJsonData bridge;
    bridge.type = BACK_TRY_TRANS_FILES;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "" }
    };
}

void BackendImpl::resumeTransJob(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = BACK_RESUME_JOB;
    bridge.json = req.str(); //TransJobParam
    _interface->bridgeChan()->operator<<(bridge);

    // wait for result:{ CallResult }
    BridgeJsonData result;
    _interface->bridgeResult()->operator>>(result);
    bool ok = _interface->bridgeResult()->done();
    co::Json call;
    if (call.parse_from(result.json)) {
        bool callok = call.get("result").as_bool();
        res.add_member("result", ok && callok);
        res.add_member("msg", call.get("msg").as_string());
    } else {
        res.add_member("result", false);
        res.add_member("msg", "");
    }
}

void BackendImpl::cancelTransJob(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = BACK_CANCEL_JOB;
    bridge.json = req.str(); //TransJobParam
    _interface->bridgeChan()->operator<<(bridge);

    // wait for result:{ CallResult }
    BridgeJsonData result;
    _interface->bridgeResult()->operator>>(result);
    bool ok = _interface->bridgeResult()->done();
    co::Json call;
    if (call.parse_from(result.json)) {
        bool callok = call.get("result").as_bool();
        res.add_member("result", ok && callok);
        res.add_member("msg", call.get("msg").as_string());
    } else {
        res.add_member("result", false);
        res.add_member("msg", "");
    }
}

void BackendImpl::fsCreate(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = BACK_FS_CREATE;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "" }
    };
}

void BackendImpl::fsDelete(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = BACK_FS_DELETE;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "" }
    };
}

void BackendImpl::fsRename(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = BACK_FS_RENAME;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "" }
    };
}

void BackendImpl::fsPull(co::Json &req, co::Json &res)
{
    BridgeJsonData bridge;
    bridge.type = BACK_FS_PULL;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "" }
    };
}

void BackendImpl::registerDiscovery(co::Json& req, co::Json& res)
{
    BridgeJsonData bridge;
    bridge.type = BACK_DISC_REGISTER;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "" }
    };
}

void BackendImpl::unregisterDiscovery(co::Json& req, co::Json& res)
{
    BridgeJsonData bridge;
    bridge.type = BACK_DISC_UNREGISTER;
    bridge.json = req.str();
    _interface->bridgeChan()->operator<<(bridge);

    // do not need to wait for result
    res = {
        { "result", true },
        { "msg", "" }
    };
}
