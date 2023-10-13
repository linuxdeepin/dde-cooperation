// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "frontendservice.h"
#include "common/constant.h"

#include "co/co.h"
#include "co/time.h"

using namespace ipc;

// must change the version if the IPC API changed.
#define FRONTEND_PROTO_VERSION UNI_IPC_PROTO

FrontendService::FrontendService(QObject *parent)
    : QObject(parent)
{
}

FrontendService::~FrontendService()
{
}

void FrontendService::handlePing(QString sessionid)
{
    emit sigSession(sessionid);
}

void FrontendService::handleConnectstatus(int result, QString msg)
{
    emit sigConnectStatus(result, msg);
}

void FrontendService::handleTransJobstatus(int id, int result, QString msg)
{
    emit sigTransJobtatus(id, result, msg);
}

void FrontendService::handleFileTransstatus(QString statusstr)
{
    emit sigFileTransStatus(statusstr);
}


void FrontendImpl::ping(co::Json &req, co::Json &res)
{
    PingFrontParam param;
    param.from_json(req);

    bool result = false;
    fastring s = "";
    fastring my_ver(FRONTEND_PROTO_VERSION);
    if (my_ver.compare(param.version) == 0) {
        result = true;
    } else {
        DLOG << param.version << " =version not match= " << my_ver;
    }

    fastring session = param.session;
    res = {
        { "result", result },
        { "msg", session }
    };

    _interface->handlePing(QString(session.c_str()));
}

void FrontendImpl::cbPeerInfo(co::Json &req, co::Json &res)
{

}

void FrontendImpl::cbConnect(co::Json &req, co::Json &res)
{
    GenericResult param;
    param.from_json(req);
    QString mesg(param.msg.c_str());
    _interface->handleConnectstatus(param.result, mesg);
    res = {
        { "result", true},
        { "msg", ""}
    };
}

void FrontendImpl::cbTargetSpace(co::Json &req, co::Json &res)
{

}

void FrontendImpl::cbApplist(co::Json &req, co::Json &res)
{

}

void FrontendImpl::cbMiscMessage(co::Json &req, co::Json &res)
{

}

void FrontendImpl::cbTransStatus(co::Json &req, co::Json &res)
{
    GenericResult param;
    param.from_json(req);
    QString mesg(param.msg.c_str());
    _interface->handleTransJobstatus(param.id, param.result, mesg);
    res = {
        { "result", true},
        { "msg", ""}
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
    QString objstr(req.str().c_str());
    qInfo() << "notifyFileStatus: " << objstr;
    _interface->handleFileTransstatus(objstr);
    res = {
        { "result", true},
        { "msg", ""}
    };
}
