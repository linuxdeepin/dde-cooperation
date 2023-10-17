// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "frontendservice.h"
#include "common/constant.h"
#include "ipc/proto/comstruct.h"

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

void FrontendService::handleTransJobstatus(int id, int result, QString path)
{
    emit sigTransJobtatus(id, result, path);
}

void FrontendService::handleFileTransstatus(QString statusstr)
{
    emit sigFileTransStatus(statusstr);
}

void FrontendService::handlePeerChanges(bool find, fastring peerinfo)
{
    QString info(peerinfo.c_str());
    qInfo() << "handlePeerChanges: " << info << " find=" << find;
    emit sigPeerChanged(find, info);

    // example to parse string to PeerInfo object
//    PeerInfo peerobj;
//    co::Json peerJson;
//    peerJson.parse_from(info.toStdString());
//    peerobj.from_json(peerJson);

//    qInfo() << " peer : " << peerobj.as_json().str().c_str();
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
    GenericResult param;
    param.from_json(req);

    res = {
        { "result", true },
        { "msg", "ok" }
    };

    _interface->handlePeerChanges(param.result > 0, param.msg);
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
    QString mesg(param.msg.c_str()); // job path
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
