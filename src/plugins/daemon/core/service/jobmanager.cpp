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
    job->initRpc(_connected_target, UNI_RPC_PORT_BASE);
    connect(job.data(), &TransferJob::notifyFileTransStatus, this, &JobManager::handleFileTransStatus, Qt::QueuedConnection);
    connect(job.data(), &TransferJob::notifyJobResult, this, &JobManager::handleJobTransStatus, Qt::QueuedConnection);

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

bool JobManager::doJobAction(uint32_t action, const co::Json &jsonobj)
{
    ipc::TransJobParam param;
    param.from_json(jsonobj);

    QString session(param.session.c_str());
    int jobid = param.job_id;
    bool remote = param.is_remote;

    if (BACK_CANCEL_JOB == action) {
        if (remote) {
            //receive job cancel and notify remote send cancel
            auto job = _transjob_recvs.value(jobid);
            if (!job.isNull()) {
                job->cancel();
                _transjob_recvs.remove(jobid);
                return true;
            } else {
                return false;
            }
        } else {
            //send job cancel.
            auto job = _transjob_sends.value(jobid);
            if (!job.isNull()) {
                job->cancel();
                _transjob_sends.remove(jobid);
                return true;
            } else {
                return false;
            }
        }
    } else if (BACK_RESUME_JOB == action) {

    }
    return true;
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
        job->insertFileInfo(finfo);
    } else {
        return false;
    }

    return true;
}

bool JobManager::handleCancelJob(co::Json &info, FileTransResponse *reply)
{
    FSJobCancel obj;
    obj.from_json(info);
    int32 jobId = obj.job_id;
    if (reply) {
        reply->id = jobId;
        reply->name = obj.path;
    }


    auto job = _transjob_recvs.value(jobId);
    if (!job.isNull()) {
        //disconnect(job, &TransferJob::notifyFileTransStatus, this, &ServiceManager::handleFileTransStatus);
        job->stop();
        _transjob_recvs.remove(jobId);
        QString name(job->getAppName().c_str());
        SendIpcService::instance()->handleRemoveJob(job->getAppName().c_str(), jobId);
        return true;
    } else {
        return false;
    }
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
    //DLOG << "notify file trans status to:" << s->getName().toStdString();
    UNIGO([appname, status, fileinfo]() {
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
    });
}

void JobManager::handleJobTransStatus(QString appname, int jobid, int status, QString savedir)
{
    //DLOG << "notify file trans status to:" << s->getName().toStdString();
    UNIGO([appname, jobid, status, savedir]() {
        co::Json req;
        //cbTransStatus {GenericResult}
        req = {
            { "id", jobid },
            { "result", status },
            { "msg", savedir.toStdString() },
        };

        req.add_member("api", "Frontend.cbTransStatus");
        SendIpcService::instance()->handleSendToClient(appname, req.str().c_str());
    });
}


JobManager::JobManager(QObject *parent) : QObject (parent)
{

}
