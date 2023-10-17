// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "backendservice.h"
#include "common/constant.h"
#include "utils/config.h"

#include "co/co.h"
#include "co/time.h"

using namespace ipc;

// must change the version if the IPC API changed.
#define BACKEND_PROTO_VERSION UNI_IPC_PROTO

BackendService::BackendService(QObject *parent)
    : QObject(parent)
{
}

BackendService::~BackendService()
{
}

fastring BackendService::handlePing(const char *who, const char *version, int cbport)
{
    fastring s = "";
    fastring my_ver(BACKEND_PROTO_VERSION);
    if (my_ver.compare(version) != 0) {
        DLOG << version << " =version not match= " << my_ver;
        return s;
    }
    QString iam(who);
    s = co::randstr(who, 8); // 长度为8的16进制字符串
    QString session(s.c_str());
    qInfo() << "gen session: " << iam << " >> " << session;

    emit sigSaveSession(iam, session, cbport);

    return s;
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


void BackendService::handleConnect(const char *session, const char *ip, const char *password)
{
    QString ses_id(session);
    QString target_ip(ip);
    QString user_password(password);
    emit sigConnect(ses_id, target_ip, user_password);
}

void BackendService::handleSendFiles(QString session, int jobid, QStringList &paths, bool sub, QString savedir)
{
    emit sigSendFiles(session, jobid, paths, sub, savedir);
}

void BackendImpl::ping(co::Json &req, co::Json &res)
{
    PingBackParam param;
    param.from_json(req);

    fastring session = _interface->handlePing(param.who.c_str(), param.version.c_str(), param.cb_port);
    res = {
        { "result", session.empty() ? false : true },
        { "msg", session }
    };
}

void BackendImpl::getDiscovery(co::Json &req, co::Json &res)
{

}

void BackendImpl::getPeerInfo(co::Json &req, co::Json &res)
{

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
    ConnectParam param;
    param.from_json(req);
    fastring session = param.session;
    fastring ip = param.host;
    fastring pass = param.password;
    _interface->handleConnect(session.c_str(), ip.c_str(), pass.c_str());
    res = {
        { "result", true},
        { "msg", ""}
    };
}

void BackendImpl::tryTargetSpace(co::Json &req, co::Json &res)
{

}

void BackendImpl::tryApplist(co::Json &req, co::Json &res)
{

}

void BackendImpl::miscMessage(co::Json &req, co::Json &res)
{

}

void BackendImpl::tryTransFiles(co::Json &req, co::Json &res)
{
    TransFilesParam param;
    param.from_json(req);

    QString session = QString(param.session.c_str());
    QString savedir = QString(param.savedir.c_str());
    QStringList paths;
    for (uint32 i = 0; i < param.paths.size(); i++) {
        paths << param.paths[i].c_str();
    }

    qInfo() << "paths: " << paths;
    _interface->handleSendFiles(session, param.id, paths, param.sub, savedir);

    res = {
        { "result", true},
        { "msg", ""}
    };
}

void BackendImpl::resumeTransJob(co::Json &req, co::Json &res)
{

}

void BackendImpl::cancelTransJob(co::Json &req, co::Json &res)
{

}

void BackendImpl::fsCreate(co::Json &req, co::Json &res)
{

}

void BackendImpl::fsDelete(co::Json &req, co::Json &res)
{

}

void BackendImpl::fsRename(co::Json &req, co::Json &res)
{

}

void BackendImpl::fsPull(co::Json &req, co::Json &res)
{

}
