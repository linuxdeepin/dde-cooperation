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
}

void CommonImpl::syncPeers(co::Json &req, co::Json &res)
{
}

void CommonImpl::tryConnect(co::Json &req, co::Json &res)
{
    const char *ip = req.get("ip").as_string().c_str();
    const char *password = req.get("password").as_string().c_str();
    qInfo() << "tryConnect :" << password;
    _interface->handleConnect(ip, password);
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
    return ::Config::refreshPin();
}

void CommonService::handleConnect(const char *ip, const char *password)
{
    QString username("data-transfer");
    QString target_ip(ip);
    QString user_password(password);
    emit sigConnect(target_ip, username, user_password);
}
