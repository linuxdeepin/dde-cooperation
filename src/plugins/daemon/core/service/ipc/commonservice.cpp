// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "commonservice.h"
#include "fsservice.h"
#include "common/constant.h"
#include "utils/config.h"

#include "co/co.h"
#include "co/time.h"

using namespace ipc;

void CommonImpl::compatible(co::Json &req, co::Json &res)
{
}

void CommonImpl::syncConfig(co::Json &req, co::Json &res)
{
    int status = DaemonConfig::instance()->getStatus();

    bool connnect = false;
    bool transfer = false;
    bool result = false;
    switch (status) {
    case status::connected:
        connnect = true;
        break;
    case status::transferring:
        connnect = true;
        transfer = true;
        break;
    case status::result:
        connnect = true;
        transfer = true;
        result = true;
        break;
    default:
        break;
    }

    res = {
        { "connected", connnect },
        { "tranfer", transfer },
        { "result", result }
    };
}

void CommonImpl::syncPeers(co::Json &req, co::Json &res)
{
}

void CommonImpl::tryConnect(co::Json &req, co::Json &res)
{
    fastring ip = req.get("ip").as_string();
    fastring pass = req.get("password").as_string();
    _interface->handleConnect(ip.c_str(), pass.c_str());
    res = {
        { "result", "sended" }
    };
}

void CommonImpl::getSettingPassword(co::Json &req, co::Json &res)
{
    qInfo() << "getConnectPassword";
    fastring pin = _interface->getSettingPin();
    res = {
        { "password", pin }
    };
}

void CommonImpl::tryTargetSpace(co::Json &req, co::Json &res)
{
}

void CommonImpl::tryApplist(co::Json &req, co::Json &res)
{
}

void CommonImpl::chatMessage(co::Json &req, co::Json &res)
{
}

void CommonImpl::miscMessage(co::Json &req, co::Json &res)
{
}

void CommonImpl::commNotify(co::Json &req, co::Json &res)
{
}

CommonService::CommonService(QObject *parent)
    : QObject(parent)
{
}

CommonService::~CommonService()
{
}

fastring CommonService::getSettingPin() const
{
    return DaemonConfig::instance()->refreshPin();
}

void CommonService::handleConnect(const char *ip, const char *password)
{
    QString username("data-transfer");
    QString target_ip(ip);
    QString user_password(password);
    emit sigConnect(target_ip, username, user_password);
}
