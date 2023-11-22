// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "jobmanager.h"
#include "common/constant.h"
#include "ipc/proto/backend.h"
#include "ipc/bridge.h"
#include "service/ipc/sendipcservice.h"
#include "service/rpc/sendrpcservice.h"

#include "utils/config.h"

JobManager *JobManager::instance()
{
    static JobManager manager;
    return &manager;
}

JobManager::~JobManager()
{

}

bool JobManager::handleCreateFile(const int jobId, const QString &fileName, const bool isDir)
{
    auto job = _transjob_recvs.value(jobId);
    if (job.isNull())
        return false;
    return job->createFile(fileName, isDir);
}

bool JobManager::handleRemoteRequestJob(QString json)
{
    co::Json info;
    if (!info.parse_from(json.toStdString())) {
        return false;
    }
    FileTransJob fsjob;
    fsjob.from_json(info);
    int32 jobId = fsjob.job_id;
    fastring savedir = fsjob.save_path;
    fastring appName = fsjob.app_who;
    fastring tarAppname = fsjob.targetAppname;
    SendRpcService::instance()->removePing(appName.c_str());
    tarAppname = tarAppname.empty() ? appName : tarAppname;
    SendRpcService::instance()->removePing(tarAppname.c_str());
    QSharedPointer<TransferJob> job(new TransferJob());
    job->initJob(fsjob.app_who, tarAppname, jobId, fsjob.path, fsjob.sub, savedir, fsjob.write);
    if (!job->initRpc(fsjob.ip.empty() ? _connected_target : fsjob.ip, UNI_RPC_PORT_TRANS) || !job->initSuccess()) {
        ELOG << "init job rpc error !! json = " << fsjob.as_json();
        return false;
    }
    connect(job.data(), &TransferJob::notifyFileTransStatus, this, &JobManager::handleFileTransStatus, Qt::QueuedConnection);
    connect(job.data(), &TransferJob::notifyJobResult, this, &JobManager::handleJobTransStatus, Qt::QueuedConnection);
    connect(job.data(), &TransferJob::notifyJobFinished, this, &JobManager::handleRemoveJob, Qt::QueuedConnection);

    g_m.lock();
    if (fsjob.write) {
        DLOG << "(" << jobId <<")write job save to: " << savedir;
        _transjob_recvs.insert(jobId, job);
    } else {
        DLOG << "(" << jobId <<")read job save to: " << savedir;
        _transjob_sends.insert(jobId, job);
    }
    g_m.unlock();

    UNIGO([this, job]() {
        // start job one by one
        co::mutex_guard g(g_m);
        // DLOG << ".........start job: sched: " << co::sched_id() << " co: " << co::coroutine_id();
        job->start();
    });


    return true;
}

bool JobManager::doJobAction(const uint action, const int jobid)
{
    bool result = false;

    if (BACK_CANCEL_JOB == action) {
        auto rjob = _transjob_recvs.value(jobid);
        if (!rjob.isNull()) {
            rjob->cancel(true);
            result = true;
        }

        auto sjob = _transjob_sends.value(jobid);
        if (!sjob.isNull()) {
            sjob->cancel(true);
            result = true;
        }
    } else if (BACK_RESUME_JOB == action) {

    }
    return result;
}

bool JobManager::handleFSData(const co::Json &info, fastring buf, FileTransResponse *reply)
{ 
    QSharedPointer<FSDataBlock> datablock(new FSDataBlock);
    datablock->from_json(info);
    datablock->data = buf;
    int32 jobId = datablock->job_id;

    if (reply) {
        reply->id = datablock->file_id;
        reply->name = datablock->filename;
    }
    auto job = _transjob_recvs.value(jobId);
    if (!job.isNull()) {
        job->pushQueque(datablock);
    } else {
        return false;
    }

    return true;
}

bool JobManager::handleFSInfo(co::Json &info)
{
    FileInfo finfo;
    finfo.from_json(info);
    int32 jobId = finfo.job_id;

    auto job = _transjob_recvs.value(jobId);
    if (!job.isNull()) {
        //update the file relative to abs path
        fastring savedpath = path::join(DaemonConfig::instance()->getStorageDir(job->getAppName()), finfo.name);
        finfo.name = savedpath;
        job->insertFileInfo(finfo);
    } else {
        return false;
    }

    return true;
}

bool JobManager::handleCancelJob(co::Json &info, FileTransResponse *reply)
{
    FileTransJobAction obj;
    obj.from_json(info);
    int32 jobId = obj.job_id;
    bool result = false;
    if (reply) {
        reply->id = jobId;
        reply->name = obj.appname;
    }

    auto rjob = _transjob_recvs.value(jobId);
    if (!rjob.isNull()) {
        //disconnect(job, &TransferJob::notifyFileTransStatus, this, &ServiceManager::handleFileTransStatus);
        DLOG << "recv > remote canceled this job: " << jobId;
        rjob->cancel();
        QString name(rjob->getAppName().c_str());
        SendIpcService::instance()->handleRemoveJob(rjob->getAppName().c_str(), jobId);
        result = true;
    }

    auto sjob = _transjob_sends.value(jobId);
    if (!sjob.isNull()) {
        //disconnect(job, &TransferJob::notifyFileTransStatus, this, &ServiceManager::handleFileTransStatus);
        DLOG << "send > remote canceled this job: " << jobId;
        sjob->cancel();
        QString name(sjob->getAppName().c_str());
        SendIpcService::instance()->handleRemoveJob(sjob->getAppName().c_str(), jobId);
        result = true;
    }

    return result;
}

bool JobManager::handleTransReport(co::Json &info, FileTransResponse *reply)
{
    FSReport obj;
    obj.from_json(info);
    int32 jobId = obj.job_id;
    fastring path = obj.path;
    if (reply) {
        reply->id = jobId;
        reply->name = path;
    }

    switch (obj.result) {
    case IO_ERROR:
    {
        auto job = _transjob_sends.value(jobId);
        if (!job.isNull()) {
            // move the job into breaks record map.
            _transjob_break.insert(jobId, _transjob_sends.take(jobId));
        }
        break;
    }
    case OK:
        break;
    case FINIASH:
    {
        auto job = _transjob_recvs.value(jobId);
        if (!job.isNull()) {
            //disconnect(job, &TransferJob::notifyFileTransStatus, this, &ServiceManager::handleFileTransStatus);
            job->waitFinish();
            _transjob_recvs.remove(jobId);
            QString name(job->getAppName().c_str());
            SendIpcService::instance()->handleRemoveJob(job->getAppName().c_str(), jobId);
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

void JobManager::handleFileTransStatus(QString appname, int status, QString fileinfo)
{
    //DLOG << "notify file trans status to:" << appname.toStdString();
    co::Json infojson;
    infojson.parse_from(fileinfo.toStdString());
    FileInfo filejob;
    filejob.from_json(infojson);

    co::Json req, res;
    //notifyFileStatus {FileStatus}
    req = {
        { "job_id", filejob.job_id },
        { "file_id", filejob.file_id },
        { "name", filejob.name },
        { "status", status },
        { "total", filejob.total_size },
        { "current", filejob.current_size },
        { "second", filejob.time_spended },
    };

    req.add_member("api", "Frontend.notifyFileStatus");
    SendIpcService::instance()->handleSendToClient(appname, req.str().c_str());
}

void JobManager::handleJobTransStatus(QString appname, int jobid, int status, QString savedir)
{
    //DLOG << "notify file trans status to:" << appname.toStdString() << " jobid=" << jobid;
    co::Json req;
    //cbTransStatus {GenericResult}
    req = {
        { "id", jobid },
        { "result", status },
        { "msg", savedir.toStdString() },
    };

    req.add_member("api", "Frontend.cbTransStatus");
    SendIpcService::instance()->handleSendToClient(appname, req.str().c_str());
}

void JobManager::handleRemoveJob(const int jobid)
{
    _transjob_recvs.remove(jobid);
    _transjob_sends.remove(jobid);
}


JobManager::JobManager(QObject *parent) : QObject (parent)
{

}
