// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fsservice.h"
#include "common/constant.h"

#include "co/co.h"
#include "co/time.h"

using namespace ipc;

void FSImpl::compatible(co::Json &req, co::Json &res)
{
}

void FSImpl::readDir(co::Json &req, co::Json &res)
{
}

void FSImpl::removeDir(co::Json &req, co::Json &res)
{
}

void FSImpl::create(co::Json &req, co::Json &res)
{
}

void FSImpl::rename(co::Json &req, co::Json &res)
{
}

void FSImpl::removeFiles(co::Json &req, co::Json &res)
{
}

void FSImpl::sendFiles(co::Json &req, co::Json &res)
{
    qInfo() << "sendFiles";
    co::Json filepaths = req.get("filepaths");

    QStringList paths;
    uint32 size = filepaths.size();
    for (int i = 0; i < size; i++) {
        paths << filepaths[i].as_c_str();
    }

    qInfo() << "paths: " << paths;
    _interface->handleSendFiles(paths);
}

void FSImpl::receiveFiles(co::Json &req, co::Json &res)
{
}

void FSImpl::resumeJob(co::Json &req, co::Json &res)
{
}

void FSImpl::cancelJob(co::Json &req, co::Json &res)
{
}

void FSImpl::fsReport(co::Json &req, co::Json &res)
{
}

FSService::FSService(QObject *parent)
    : QObject(parent)
{
}

FSService::~FSService()
{
}

void FSService::handleSendFiles(QStringList &paths)
{
    QString savedir("/home");
    emit sigSendFiles(_job_id, paths, savedir);
}
