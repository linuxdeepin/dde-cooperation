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
}

void CommunicationJob::initRpc(fastring target, uint16 port)
{
    _targetIP = target;
    _port = port;
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

fastring CommunicationJob::getTarAppName() const
{
    return _tar_app_name;
}
