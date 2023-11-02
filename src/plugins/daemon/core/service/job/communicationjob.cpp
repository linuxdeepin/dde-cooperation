// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "communicationjob.h"
#include "co/log.h"
#include "co/fs.h"
#include "co/path.h"
#include "co/co.h"
#include "common/constant.h"
#include "service/fsadapter.h"
#include "utils/config.h"

CommunicationJob::CommunicationJob(QObject *parent)
    : QObject(parent)
{
}

CommunicationJob::~CommunicationJob()
{
    if (_rpcBinder) {
        _rpcBinder->deleteLater();
        _rpcBinder = nullptr;
    }
}

void CommunicationJob::initRpc(fastring appname, fastring target, uint16 port)
{
    if (nullptr == _rpcBinder) {
        _rpcBinder = new RemoteServiceBinder(this);
        _rpcBinder->createExecutor(appname.c_str(), target.c_str(), port);
    }
}

void CommunicationJob::initJob(fastring appname, fastring targetappname)
{
    _app_name = appname;
    _tar_app_name = targetappname;
    _inited = true;
}

fastring CommunicationJob::getAppName()
{
    return _app_name;
}

bool CommunicationJob::sendMsg(CommunicationType type, const QString &info)
{
    if (_rpcBinder == nullptr) {
        ELOG << "sendMsg ERROR: no executor, type " << type << info.toStdString();
        return false;
    }

    _rpcBinder->doSendApplyTransFiles(_app_name.c_str(), info);

    return true;
}
