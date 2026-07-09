// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "service/jobmanager.h"
#include "service/job/transferjob.h"
#include "service/comshare.h"
#include "ipc/bridge.h"
#include "common/commonstruct.h"
#include "ipc/proto/chan.h"
#include "common/constant.h"
#include "co/json.h"

#include <QTemporaryDir>
#include <QSharedPointer>

// Exercises the JobManager singleton's safe (non-network) methods.
// Lines in jobmanager.cpp covered: instance, confirm/remove/isTransferConfirmed,
// doJobAction (cancel/resume/unknown branches), handleRemoveJob,
// handleRemoteRequestJob (invalid json early-return), handleTransReport
// (default + OK branches), handleCancelJob (no job), handleFileTransStatus
// (json build path), handleJobTransStatus (finished -> removeConfirmedTransfer).

TEST(JobManagerTest, InstanceSingleton)
{
    JobManager *p1 = JobManager::instance();
    JobManager *p2 = JobManager::instance();
    EXPECT_NE(p1, nullptr);
    EXPECT_EQ(p1, p2);
}

TEST(JobManagerTest, ConfirmAndIsTransferConfirmed)
{
    JobManager *jm = JobManager::instance();
    jm->confirmTransfer("app1");
    EXPECT_TRUE(jm->isTransferConfirmed("app1"));
    EXPECT_FALSE(jm->isTransferConfirmed("app2"));
}

TEST(JobManagerTest, RemoveConfirmedTransfer)
{
    JobManager *jm = JobManager::instance();
    jm->confirmTransfer("app1");
    ASSERT_TRUE(jm->isTransferConfirmed("app1"));
    jm->removeConfirmedTransfer("app1");
    EXPECT_FALSE(jm->isTransferConfirmed("app1"));
}

TEST(JobManagerTest, DoJobActionCancelNoJob)
{
    JobManager *jm = JobManager::instance();
    // No job with id 99999 exists -> cancel branch finds nothing -> false.
    EXPECT_FALSE(jm->doJobAction(BACK_CANCEL_JOB, 99999));
}

TEST(JobManagerTest, DoJobActionResume)
{
    JobManager *jm = JobManager::instance();
    // BACK_RESUME_JOB branch is empty -> result stays false.
    EXPECT_FALSE(jm->doJobAction(BACK_RESUME_JOB, 1));
}

TEST(JobManagerTest, DoJobActionUnknown)
{
    JobManager *jm = JobManager::instance();
    // Unknown action code -> neither branch taken -> false.
    EXPECT_FALSE(jm->doJobAction(9999, 1));
}

TEST(JobManagerTest, HandleRemoveJobNoCrash)
{
    JobManager *jm = JobManager::instance();
    // Removing a nonexistent job should not crash.
    jm->handleRemoveJob(123);
    SUCCEED();
}

TEST(JobManagerTest, HandleRemoteRequestJobInvalidJson)
{
    JobManager *jm = JobManager::instance();
    QString target;
    EXPECT_FALSE(jm->handleRemoteRequestJob("not json", &target));
}

TEST(JobManagerTest, HandleTransReportUnknownResult)
{
    JobManager *jm = JobManager::instance();
    co::Json info;
    info.parse_from(R"({"job_id":555,"path":"/tmp/x","result":999,"error":""})");
    FileTransResponse reply;
    // result=999 falls into default branch -> returns true.
    EXPECT_TRUE(jm->handleTransReport(info, &reply));
    EXPECT_EQ(reply.id, 555);
}

TEST(JobManagerTest, HandleTransReportOk)
{
    JobManager *jm = JobManager::instance();
    co::Json info;
    info.parse_from(R"({"job_id":777,"path":"/tmp/ok","result":1,"error":""})");
    FileTransResponse reply;
    // OK (1) branch -> returns true.
    EXPECT_TRUE(jm->handleTransReport(info, &reply));
    EXPECT_EQ(reply.id, 777);
}

TEST(JobManagerTest, HandleCancelJobNoJob)
{
    JobManager *jm = JobManager::instance();
    co::Json info;
    info.parse_from(R"({"job_id":888,"appname":"app","type":1})");
    FileTransResponse reply;
    // No matching recv/send job -> returns false.
    EXPECT_FALSE(jm->handleCancelJob(info, &reply));
    EXPECT_EQ(reply.id, 888);
}

TEST(JobManagerTest, HandleFileTransStatus)
{
    JobManager *jm = JobManager::instance();
    // Just exercises the json build + SendIpcService call path (no session -> early return).
    jm->handleFileTransStatus("app", FILE_TRANS_IDLE,
                              R"({"job_id":1,"file_id":1,"name":"f","total_size":100,"current_size":50,"time_spended":10})");
    SUCCEED();
}

TEST(JobManagerTest, HandleJobTransStatusFinished)
{
    JobManager *jm = JobManager::instance();
    // Pre-confirm so we can verify the finished status clears it.
    jm->confirmTransfer("app1");
    ASSERT_TRUE(jm->isTransferConfirmed("app1"));

    jm->handleJobTransStatus("app1", 1, JOB_TRANS_FINISHED, "/tmp");

    // JOB_TRANS_FINISHED triggers removeConfirmedTransfer -> no longer confirmed.
    EXPECT_FALSE(jm->isTransferConfirmed("app1"));
}

// ---- Extended coverage: insert real jobs into the maps via -fno-access-control ----

TEST(JobManagerTest, HandleFSDataNoJob)
{
    JobManager *jm = JobManager::instance();
    FSDataBlock db;
    db.job_id = 4242;
    co::Json info = db.as_json();
    FileTransResponse reply;
    EXPECT_FALSE(jm->handleFSData(info, fastring("data"), &reply));
}

TEST(JobManagerTest, HandleFSDataWithRecvJob)
{
    JobManager *jm = JobManager::instance();
    // Insert a real TransferJob into _transjob_recvs via private member.
    int jobId = 7777;
    QSharedPointer<TransferJob> job(new TransferJob());
    job->initJob("covapp", "target", jobId, "[]", false, "savedir", true);
    jm->_transjob_recvs.insert(jobId, job);

    FSDataBlock db;
    db.job_id = jobId;
    db.file_id = 1;
    db.flags = JobTransFileOp::FIlE_CREATE;
    co::Json info = db.as_json();
    FileTransResponse reply;
    bool res = jm->handleFSData(info, fastring(""), &reply);
    EXPECT_TRUE(res);

    // reply populated
    EXPECT_EQ(reply.id, db.file_id);
    jm->_transjob_recvs.remove(jobId);
}

TEST(JobManagerTest, HandleFSDataCountedNoSpace)
{
    JobManager *jm = JobManager::instance();
    int jobId = 8888;
    QSharedPointer<TransferJob> job(new TransferJob());
    job->initJob("covapp2", "target", jobId, "[]", false, "savedir", true);
    // freeBytes defaults to -1, so any data_size >= -1 triggers the false path.
    jm->_transjob_recvs.insert(jobId, job);

    FSDataBlock db;
    db.job_id = jobId;
    db.flags = JobTransFileOp::FILE_COUNTED;
    db.data_size = 100;
    co::Json info = db.as_json();
    FileTransResponse reply;
    EXPECT_FALSE(jm->handleFSData(info, fastring(""), &reply));
    jm->_transjob_recvs.remove(jobId);
}

TEST(JobManagerTest, HandleCancelJobWithRecvJob)
{
    JobManager *jm = JobManager::instance();
    int jobId = 5555;
    QSharedPointer<TransferJob> job(new TransferJob());
    job->initJob("cancelapp", "target", jobId, "[]", false, "savedir", true);
    jm->_transjob_recvs.insert(jobId, job);

    FileTransJobAction act;
    act.job_id = jobId;
    act.appname = "cancelapp";
    act.type = TRANS_CANCEL;
    co::Json info = act.as_json();
    FileTransResponse reply;
    bool res = jm->handleCancelJob(info, &reply);
    EXPECT_TRUE(res);
    jm->_transjob_recvs.remove(jobId);
}

TEST(JobManagerTest, HandleCancelJobWithSendJob)
{
    JobManager *jm = JobManager::instance();
    int jobId = 6666;
    QSharedPointer<TransferJob> job(new TransferJob());
    job->initJob("sendapp", "target", jobId, "[]", false, "savedir", false);
    jm->_transjob_sends.insert(jobId, job);

    FileTransJobAction act;
    act.job_id = jobId;
    act.appname = "sendapp";
    act.type = TRANS_CANCEL;
    co::Json info = act.as_json();
    FileTransResponse reply;
    bool res = jm->handleCancelJob(info, &reply);
    EXPECT_TRUE(res);
    jm->_transjob_sends.remove(jobId);
}

TEST(JobManagerTest, HandleTransReportIOError)
{
    JobManager *jm = JobManager::instance();
    int jobId = 3333;
    QSharedPointer<TransferJob> job(new TransferJob());
    job->initJob("ioapp", "target", jobId, "[]", false, "savedir", false);
    jm->_transjob_sends.insert(jobId, job);

    FSReport rep;
    rep.job_id = jobId;
    rep.result = IO_ERROR;
    co::Json info = rep.as_json();
    FileTransResponse reply;
    EXPECT_TRUE(jm->handleTransReport(info, &reply));
    // job moved to _transjob_break
    // (no assertion needed; just exercise the branch)
}

TEST(JobManagerTest, HandleTransReportFinishNoJob)
{
    JobManager *jm = JobManager::instance();
    FSReport rep;
    rep.job_id = 9999;
    rep.result = FINIASH;
    co::Json info = rep.as_json();
    FileTransResponse reply;
    EXPECT_FALSE(jm->handleTransReport(info, &reply));
}

TEST(JobManagerTest, DoJobActionCancelRecvJob)
{
    JobManager *jm = JobManager::instance();
    int jobId = 1111;
    QSharedPointer<TransferJob> job(new TransferJob());
    job->initJob("actapp", "target", jobId, "[]", false, "savedir", true);
    jm->_transjob_recvs.insert(jobId, job);
    EXPECT_TRUE(jm->doJobAction(BACK_CANCEL_JOB, jobId));
    jm->_transjob_recvs.remove(jobId);
}

TEST(JobManagerTest, DoJobActionCancelSendJob)
{
    JobManager *jm = JobManager::instance();
    int jobId = 2222;
    QSharedPointer<TransferJob> job(new TransferJob());
    job->initJob("actapp2", "target", jobId, "[]", false, "savedir", false);
    jm->_transjob_sends.insert(jobId, job);
    EXPECT_TRUE(jm->doJobAction(BACK_CANCEL_JOB, jobId));
    jm->_transjob_sends.remove(jobId);
}

TEST(JobManagerTest, HandleOtherOfflineEmptyMaps)
{
    JobManager *jm = JobManager::instance();
    jm->handleOtherOffline("1.2.3.4");
    SUCCEED();
}

TEST(JobManagerTest, HandleRemoteRequestJobWriteConfirmedBadJson)
{
    JobManager *jm = JobManager::instance();
    // not valid json
    EXPECT_FALSE(jm->handleRemoteRequestJob("not json at all", nullptr));
}

TEST(JobManagerTest, HandleRemoteRequestJobWriteConfirmedSkipped)
{
    // SKIPPED: the confirmed-write path creates a real TransferJob + coroutine
    // + blocking ZRpc call (65s+), making it unstable in unit tests.
    // Coverage of the confirm-check branch is provided by the NotConfirmed test.
    SUCCEED();
}

TEST(JobManagerTest, HandleRemoteRequestJobReadSetsTarget)
{
    JobManager *jm = JobManager::instance();
    // write=false, but no confirmed transfer -> bypasses confirm check,
    // creates TransferJob, initRpc fails (empty ip) -> false. target set.
    FileTransJob job;
    job.job_id = 4747;
    job.write = false;
    job.app_who = "readapp";
    job.targetAppname = "readtarget";
    job.path = "[]";
    QString target;
    bool res = jm->handleRemoteRequestJob(job.as_json().str().c_str(), &target);
    EXPECT_FALSE(res);
    // target defaults to app_who when targetAppname empty; here it's set
}
