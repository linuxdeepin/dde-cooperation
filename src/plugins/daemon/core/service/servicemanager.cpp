// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "servicemanager.h"
#include "rpc/remoteservice.h"
#include "ipc/common.h"
#include "ipc/fs.h"
#include "ipc/chan.h"
#include "ipc/commonservice.h"
#include "ipc/fsservice.h"
#include "common/constant.h"
#include "comshare.h"
#include "co/cout.h"

#include "utils/config.h"

co::chan<IncomeData> _income_chan(10);
co::chan<OutData> _outgo_chan(10, 10);

ServiceManager::ServiceManager(QObject *parent) : QObject(parent)
{
    flag::set_value("rpc_log", "false");
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

    _transjob_sends.clear();
    _transjob_recvs.clear();
    _transjob_break.clear();
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
    go([this]() {
        while(true) {
            IncomeData indata;
            _income_chan >> indata;
//            LOG << "ServiceManager get chan value: " << indata.type << " json:" << indata.json;
            co::Json json_obj = json::parse(indata.json);
            if (json_obj.is_null()) {
                ELOG << "parse error from: " << indata.json;
                continue;
            }
            switch (indata.type) {
            case IN_LOGIN:
            {
                break;
            }
            case IN_TRANSJOB:
            {
                handleRemoteRequestJob(json_obj);
                break;
            }
            case FS_DATA:
            {
                // must update the binrary data into struct object.
                handleFSData(json_obj, indata.buf);
                break;
            }
            case FS_INFO:
            {
                handleFSInfo(json_obj);
                break;
            }
            case TRANS_CANCEL:
            {
                handleCancelJob(json_obj);
                break;
            }
            case FS_REPORT:
            {
                handleTransReport(json_obj);
                break;
            }
            default:
                break;
            }
        }
    });
}

bool ServiceManager::handleRemoteRequestJob(co::Json &info)
{
    FSJob fsjob;
    fsjob.from_json(info);
    int32 jobId = fsjob.job_id;

    TransferJob *job = new TransferJob();
    job->initRpc(_connected_target, UNI_RPC_PORT_BASE);

    if (fsjob.write) {
        // 写文件任务，path也是保存路径
        fastring savedir = fsjob.path;
        if (savedir.empty()) {
            savedir = DaemonConfig::instance()->getStorageDir();
        }
        DLOG << "write job: " << savedir;
        job->initJob(jobId, fsjob.path, fsjob.sub, savedir);
        _transjob_recvs.insert(jobId, job);
    } else {
        job->initJob(jobId, fsjob.path, fsjob.sub, "");
        _transjob_sends.insert(jobId, job);
    }
    go([this, job]() {
        // start job one by one
        co::mutex_guard g(g_m);
        DLOG << ".........start job: sched: " << co::sched_id() << " co: " << co::coroutine_id();
        job->start();
    });

    return true;
}

bool ServiceManager::handleFSData(co::Json &info, fastring buf)
{
    FSDataBlock datablock;
    datablock.from_json(info);
    datablock.data = buf;
    int32 jobId = datablock.job_id;

    TransferJob *job = _transjob_recvs.value(jobId);
    if (nullptr != job) {
        job->pushQueque(datablock);
    } else {
        return false;
    }

    return true;
}

bool ServiceManager::handleFSInfo(co::Json &info)
{
    FileInfo finfo;
    finfo.from_json(info);
    int32 jobId = finfo.job_id;

    TransferJob *job = _transjob_recvs.value(jobId);
    if (nullptr != job) {
        job->insertFileInfo(finfo);
    } else {
        return false;
    }

    return true;
}

bool ServiceManager::handleCancelJob(co::Json &info)
{
    FSJobCancel obj;
    obj.from_json(info);
    int32 jobId = obj.job_id;

    TransferJob *job = _transjob_recvs.value(jobId);
    if (nullptr != job) {
        job->stop();
    } else {
        return false;
    }

    return true;
}

bool ServiceManager::handleTransReport(co::Json &info)
{
    FSReport obj;
    obj.from_json(info);
    int32 jobId = obj.job_id;
    fastring path = obj.path;

    switch (obj.result) {
    case IO_ERROR:
    {
        TransferJob *job = _transjob_sends.value(jobId);
        if (nullptr != job) {
            // move the job into breaks record map.
            _transjob_break.insert(jobId, _transjob_sends.take(jobId));
        }
        break;
    }
    case OK:
        break;
    case FINIASH:
    {
        TransferJob *job = _transjob_recvs.value(jobId);
        if (nullptr != job) {
            job->waitFinish();
            _transjob_recvs.remove(jobId);
        } else {
            return false;
        }
        break;
    }
    default:
        DLOG << "unkown report: " << obj.result;
        break;
    }

    return true;
}

void ServiceManager::newTransJob(int32 jobId, QStringList paths, QString savedir)
{
    int32 id = jobId;
    for (QString path : paths) {
        QByteArray byteArray = path.toUtf8();
        const char *filepath = byteArray.constData();
        co::Json jobjson = {
            { "job_id", id },
            { "path", filepath },
            { "hidden", false },
            { "sub", true },
            { "write", false }
        };
        handleRemoteRequestJob(jobjson);
        id++;
    }

//    DaemonConfig::instance()->setStatus(transferring);
}

void ServiceManager::notifyConnect(QString ip, QString name, QString password)
{
    if (ip == nullptr) {
        return;
    }

    if (_rpcServiceBinder) {
        fastring target_ip = ip.toStdString();
        fastring user = name.toStdString();
        fastring pincode = password.toStdString();
        _connected_target = target_ip;

        go([this, target_ip, user, pincode]() {
            _rpcServiceBinder->createExecutor(target_ip.c_str(), UNI_RPC_PORT_BASE);
            _rpcServiceBinder->doLogin(user.c_str(), pincode.c_str());
        });
    }
}
