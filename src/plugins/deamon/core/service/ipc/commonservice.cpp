// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "commonservice.h"
#include "fsservice.h"
#include "common/constant.h"

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
    qInfo() << req.get("password").as_string().c_str();
    qInfo() << "tryConnect";
    res = {
        { "error", "not supported" }
    };
}

void CommonImpl::getSettingPassword(co::Json &req, co::Json &res)
{
    qInfo() << "getConnectPassword";
    res = {
        { "password", "666666" }
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
    rpc::Server()
            .add_service(new CommonImpl())
            .add_service(new FSImpl())
            .start("0.0.0.0", 7788, "/common", "", "");
}

CommonService::~CommonService()
{
}
