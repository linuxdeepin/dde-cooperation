// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "servicemanager.h"
#include "rpc/remoteservice.h"
#include "ipc/common.h"
#include "ipc/fs.h"
#include "ipc/commonservice.h"
#include "ipc/fsservice.h"
#include "common/constant.h"
#include "co/co.h"

#include "utils/config.h"

ServiceManager::ServiceManager(QObject *parent) : QObject(parent)
{
    _rpcServiceBinder = new RemoteServiceBinder(this);
    _ipcCommonService = new CommonService(this);
    _ipcFsService = new FSService(this);

    connect(_ipcCommonService, &CommonService::sigConnect, this, &ServiceManager::notifyConnect, Qt::QueuedConnection);
    connect(_ipcFsService, &FSService::sigSendFiles, this, &ServiceManager::newTransJob, Qt::QueuedConnection);

    // start ipc services
    ipc::CommonImpl *commimp = new ipc::CommonImpl();
    commimp->setInterface(_ipcCommonService);
    ipc::FSImpl *fsimp = new ipc::FSImpl();
    fsimp->setInterface(_ipcFsService);
    rpc::Server()
            .add_service(commimp)
            .add_service(fsimp)
            .start("0.0.0.0", 7788, "/common", "", "");
}

ServiceManager::~ServiceManager()
{
    if (_rpcServiceBinder) {
        _rpcServiceBinder->deleteLater();
    }
    if (_ipcCommonService) {
        _ipcCommonService->deleteLater();
    }
    if (_ipcFsService) {
        _ipcFsService->deleteLater();
    }
}

void ServiceManager::startRemoteServer()
{
    if (_rpcServiceBinder) {
        _rpcServiceBinder->startRpcListen();
    }
}

void ServiceManager::newTransJob(int32 jobId, QStringList paths, QString savedir)
{
//    if (!_hasConnected) {
//        go([this]() {
//            notifyConnect("10.8.11.52", "zero1", "256412");
//        });
//        _hasConnected = true;
//    }
//    co::sleep(1);

    bool push = true;
    bool sub = true;
    TransferJob *job = new TransferJob();
    job->initJob(jobId, paths, sub, savedir);
    
    go([this, job, push]() {
        job->startJob(push, _rpcServiceBinder);
    });
    if (push) {
        _transjob_sends.insert(jobId, job);
    } else {
        _transjob_sends.insert(jobId, job);
    }
}

void ServiceManager::notifyConnect(QString ip, QString name, QString password)
{
    if (ip == nullptr) {
        return;
    }

    if (_rpcServiceBinder) {
        QByteArray byteArray = ip.toUtf8();
        char* target_ip = byteArray.data();
        byteArray = name.toUtf8();
        char* user = byteArray.data();

        byteArray = password.toUtf8();
        const char* pincode = byteArray.constData();

        _rpcServiceBinder->createExecutor(target_ip, UNI_RPC_PORT_BASE);
        _rpcServiceBinder->doLogin(user, pincode);
    }
}
