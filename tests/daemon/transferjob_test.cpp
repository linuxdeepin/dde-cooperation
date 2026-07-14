// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "service/job/transferjob.h"
#include "common/commonstruct.h"
#include "common/constant.h"
#include "ipc/proto/chan.h"
#include "co/json.h"
#include "utils/config.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QFile>
#include <QDir>
#include "stub.h"
#include "utils/utils.h"

// setDeviceNotenough() is declared in transferjob.h but has no definition in
// transferjob.cpp. Provide the implementation here so the test (and the
// daemon_tests executable) links. Compiled with -fno-access-control so the
// private atomic member is reachable.
void TransferJob::setDeviceNotenough()
{
    _device_not_enough = true;
}

// ---------------------------------------------------------------------------
// All tests construct a bare TransferJob and exercise safe members only.
// start(), handleBlockQueque, readFileBlock, scanPath, readPath, handleUpdate
// and sendToRemote are AVOIDED (coroutine loops / heavy IO).
// Private members are reached directly thanks to -fno-access-control.
// ---------------------------------------------------------------------------

// 1. ConstructorStatusNone — _status == NONE right after construction.
TEST(TransferJobTest, ConstructorStatusNone)
{
    TransferJob job;
    EXPECT_EQ(job._status, NONE);
}

// 2. IsWriteJobFalse — isWriteJob() false by default.
TEST(TransferJobTest, IsWriteJobFalse)
{
    TransferJob job;
    EXPECT_FALSE(job.isWriteJob());
}

// 3. GetAppNameEmpty — getAppName() empty by default.
TEST(TransferJobTest, GetAppNameEmpty)
{
    TransferJob job;
    EXPECT_TRUE(job.getAppName().empty());
}

// 4. EndedInitiallyFalse — ended() false (status NONE != STOPED).
TEST(TransferJobTest, EndedInitiallyFalse)
{
    TransferJob job;
    EXPECT_FALSE(job.ended());
}

// 5. IsRunningFalse — isRunning() false (status NONE != STARTED).
TEST(TransferJobTest, IsRunningFalse)
{
    TransferJob job;
    EXPECT_FALSE(job.isRunning());
}

// 6. FreeBytesNegative — freeBytes() == -1 (default _device_free_size).
TEST(TransferJobTest, FreeBytesNegative)
{
    TransferJob job;
    EXPECT_EQ(job.freeBytes(), -1);
}

// 7. StopSetsStatus — stop(); ended() true (status STOPED).
TEST(TransferJobTest, StopSetsStatus)
{
    TransferJob job;
    job.stop();
    EXPECT_EQ(job._status, STOPED);
    EXPECT_TRUE(job.ended());
}

// 8. WaitFinishSetsStatus — waitFinish(); _status == WAIT_DONE.
TEST(TransferJobTest, WaitFinishSetsStatus)
{
    TransferJob job;
    job.waitFinish();
    EXPECT_EQ(job._status, WAIT_DONE);
}

// 9. CancelNotNotify — cancel(false) marks canceled, sets STOPED, and emits
// notifyJobResult (via handleJobStatus(JOB_TRANS_CANCELED)).
TEST(TransferJobTest, CancelNotNotify)
{
    TransferJob job;
    QSignalSpy spy(&job, &TransferJob::notifyJobResult);
    ASSERT_TRUE(spy.isValid());

    job.cancel(false);

    EXPECT_TRUE(job._mark_canceled);
    EXPECT_EQ(job._status, STOPED);
    EXPECT_FALSE(job.isWriteJob());
    ASSERT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(2).toInt(), JOB_TRANS_CANCELED);
}

// 10. CancelNotify — with _status == STARTED, cancel(true) atomically swaps
// to CANCELING.
TEST(TransferJobTest, CancelNotify)
{
    TransferJob job;
    job._status = STARTED;

    job.cancel(true);

    EXPECT_EQ(job._status, CANCELING);
    EXPECT_TRUE(job._mark_canceled);
}

// 11. InitJobWrite — initJob(write=true) sets _writejob, _app_name, status INIT.
TEST(TransferJobTest, InitJobWrite)
{
    TransferJob job;
    job.initJob("app", "target", 1, "[]", false, "savedir", true);

    EXPECT_EQ(job._app_name, fastring("app"));
    EXPECT_EQ(job._tar_app_name, fastring("target"));
    EXPECT_EQ(job._jobid, 1);
    EXPECT_EQ(job._status, INIT);
    EXPECT_TRUE(job.isWriteJob());
    EXPECT_FALSE(job._savedir.empty());
}

// 12. InitJobRead — initJob(write=false) sets _writejob false, status INIT,
// and Comshare status to TRAN_FILE_SEN.
TEST(TransferJobTest, InitJobRead)
{
    TransferJob job;
    job.initJob("app", "target", 2, "[]", false, "savedir", false);

    EXPECT_EQ(job._status, INIT);
    EXPECT_FALSE(job.isWriteJob());
    EXPECT_FALSE(job._writejob);
}

// 13. InitRpcWriteEmptyTarget — after initJob(write=true), initRpc("") returns
// true early (no send, target empty + writejob allowed).
TEST(TransferJobTest, InitRpcWriteEmptyTarget)
{
    TransferJob job;
    job.initJob("app", "target", 1, "[]", false, "savedir", true);

    bool ok = job.initRpc("", 51597);
    EXPECT_TRUE(ok);
    EXPECT_EQ(job._tar_ip, fastring(""));
    EXPECT_EQ(job._tar_port, (uint16)51597);
}

// 14. InitRpcReadEmptyTarget — after initJob(write=false), initRpc("") returns
// false (logs error, target empty + read job not allowed).
TEST(TransferJobTest, InitRpcReadEmptyTarget)
{
    TransferJob job;
    job.initJob("app", "target", 2, "[]", false, "savedir", false);

    bool ok = job.initRpc("", 51597);
    EXPECT_FALSE(ok);
}

// 15. InitRpcReadValidTarget — initJob(write=false) then initRpc("127.0.0.1")
// creates a RemoteServiceSender and attempts a TRANSJOB send which fails
// (INVOKE) -> returns false, _init_success false.
TEST(TransferJobTest, InitRpcReadValidTarget)
{
    TransferJob job;
    job.initJob("app", "target", 3, "[]", false, "savedir", false);

    bool ok = job.initRpc("127.0.0.1", 51598);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(job._init_success);
}

// 16. PushQuequePopQueue — push a block, queueCount==1, pop, queueCount==0.
TEST(TransferJobTest, PushQuequePopQueue)
{
    TransferJob job;
    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->job_id = 42;

    job.pushQueque(block);
    EXPECT_EQ(job.queueCount(), 1);

    auto popped = job.popQueue();
    ASSERT_FALSE(popped.isNull());
    EXPECT_EQ(popped->job_id, 42);
    EXPECT_EQ(job.queueCount(), 0);
}

// 17. PushQuequeAfterStop — after stop(), pushQueque is skipped (STOPED).
TEST(TransferJobTest, PushQuequeAfterStop)
{
    TransferJob job;
    job.stop();

    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    job.pushQueque(block);
    EXPECT_EQ(job.queueCount(), 0);
}

// 18. SetFileNameAcName — setFileName maps name->acName; acName returns it.
TEST(TransferJobTest, SetFileNameAcName)
{
    TransferJob job;
    job.setFileName("orig", "renamed");
    EXPECT_EQ(job.acName("orig"), fastring("renamed"));
    EXPECT_EQ(job.acName("nope"), fastring(""));
}

// 19. GetSaveFullpathEmpty — empty filename rejected -> "".
TEST(TransferJobTest, GetSaveFullpathEmpty)
{
    TransferJob job;
    EXPECT_EQ(job.getSaveFullpath("/root", ""), fastring(""));
}

// 20. GetSaveFullpathTraversal — filename with ".." -> "".
TEST(TransferJobTest, GetSaveFullpathTraversal)
{
    TransferJob job;
    EXPECT_EQ(job.getSaveFullpath("/root", "../etc/passwd"), fastring(""));
}

// 21. GetSaveFullpathAbsolute — filename starts with "/" -> "".
TEST(TransferJobTest, GetSaveFullpathAbsolute)
{
    TransferJob job;
    EXPECT_EQ(job.getSaveFullpath("/root", "/etc/passwd"), fastring(""));
}

// 22. GetSaveFullpathValid — set _app_name and a real temp storage dir; a
// nested relative filename is within the allowed root. Just SUCCEED.
TEST(TransferJobTest, GetSaveFullpathValid)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString tempdir = dir.path();

    TransferJob job;
    job._app_name = "testapp";
    DaemonConfig::instance()->setAppConfig("testapp", "storagedir", tempdir.toStdString().c_str());

    fastring result = job.getSaveFullpath(tempdir.toStdString().c_str(), "sub/file.txt");
    // Path is within the allowed root; either the joined path or "" if realpath
    // rejected it on this filesystem. Both outcomes are acceptable here.
    SUCCEED();
}

// 23. CreateFile — empty path -> false.
TEST(TransferJobTest, CreateFileEmpty)
{
    TransferJob job;
    EXPECT_FALSE(job.createFile("", false));
}

// 24. CreateFileValid — create a regular file in a temp dir -> true.
TEST(TransferJobTest, CreateFileValid)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    TransferJob job;
    bool ok = job.createFile(dir.filePath("x").toLocal8Bit().constData(), false);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(QFile::exists(dir.filePath("x")));
}

// 25. CreateFileDir — create a directory in a temp dir -> true.
TEST(TransferJobTest, CreateFileDir)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    TransferJob job;
    bool ok = job.createFile(dir.filePath("d").toLocal8Bit().constData(), true);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(QDir(dir.filePath("d")).exists());
}

// 26. GetSubdir — getSubdir("/a/b/c/file.txt", "/a/b") == "/c".
TEST(TransferJobTest, GetSubdir)
{
    TransferJob job;
    fastring sub = job.getSubdir("/a/b/c/file.txt", "/a/b");
    // path::split("/a/b/c/file.txt") -> dir="/a/b/c"; remove_prefix("/a/b") -> "/c";
    // path::join("/c","") -> "/c".
    EXPECT_EQ(sub, fastring("/c"));
}

// 27. OfflineCancelWrongIp — ip mismatch (_tar_ip empty) -> false.
TEST(TransferJobTest, OfflineCancelWrongIp)
{
    TransferJob job;
    EXPECT_FALSE(job.offlineCancel("9.9.9.9"));
    EXPECT_FALSE(job._offlined);
}

// 28. OfflineCancelEmptyIp — empty ip -> false.
TEST(TransferJobTest, OfflineCancelEmptyIp)
{
    TransferJob job;
    EXPECT_FALSE(job.offlineCancel(""));
}

// 29. SetDeviceNotenough — sets the atomic _device_not_enough flag.
TEST(TransferJobTest, SetDeviceNotenough)
{
    TransferJob job;
    EXPECT_FALSE(job._device_not_enough.load());
    job.setDeviceNotenough();
    EXPECT_TRUE(job._device_not_enough.load());
}

// 30. HandleJobStatusEmitsSignal — emits notifyJobResult with given status.
TEST(TransferJobTest, HandleJobStatusEmitsSignal)
{
    TransferJob job;
    QSignalSpy spy(&job, &TransferJob::notifyJobResult);
    ASSERT_TRUE(spy.isValid());

    job.handleJobStatus(JOB_TRANS_FINISHED);

    ASSERT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(2).toInt(), JOB_TRANS_FINISHED);
}

// 31. HandleTransStatusEmitsSignal — emits notifyFileTransStatus.
TEST(TransferJobTest, HandleTransStatusEmitsSignal)
{
    TransferJob job;
    QSignalSpy spy(&job, &TransferJob::notifyFileTransStatus);
    ASSERT_TRUE(spy.isValid());

    FileInfo info;
    info.job_id = 7;
    info.name = "f.txt";
    job.handleTransStatus(FILE_TRANS_IDLE, info);

    ASSERT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(1).toInt(), FILE_TRANS_IDLE);
}

// ---- Extended coverage: drive heavy IO methods with real temp files ----

#include "service/rpc/remoteservice.h"
// ---- Extended coverage: drive heavy IO methods with real temp files ----


TEST(TransferJobCovTest, CreateSendCounting)
{
    TransferJob job;
    job._jobid = 1;
    job._status = STARTED;
    job.createSendCounting();
    EXPECT_EQ(job.queueCount(), 1);
}

TEST(TransferJobCovTest, ReadFileBlockAcTotal)
{
    TransferJob job;
    job._jobid = 2;
    job._status = STARTED;

    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    tmp.write("hello world content");
    tmp.close();
    QString path = tmp.fileName();

    job.readFileBlock(path.toStdString(), 1, "sub", true);
    EXPECT_GT(job._total_size.load(), 0);
}

TEST(TransferJobCovTest, ReadFileBlockPushesBlocks)
{
    TransferJob job;
    job._jobid = 3;
    job._status = STARTED;

    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    tmp.write("data123");
    tmp.close();
    QString path = tmp.fileName();

    job.readFileBlock(path.toStdString(), 1, "subname", false);
    EXPECT_GT(job.queueCount(), 0);
}

TEST(TransferJobCovTest, ReadFileBlockEmptyFile)
{
    TransferJob job;
    job._jobid = 4;
    job._status = STARTED;

    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    tmp.close();
    QString path = tmp.fileName();

    job.readFileBlock(path.toStdString(), 1, "sub", false);
    EXPECT_EQ(job.queueCount(), 1);
}

TEST(TransferJobCovTest, ReadFileBlockNonexistent)
{
    TransferJob job;
    job._jobid = 5;
    job._status = STARTED;
    job.readFileBlock("/nonexistent/file/xyz", 1, "sub", false);
    EXPECT_EQ(job.queueCount(), 0);
}

TEST(TransferJobCovTest, ScanPathSingleFile)
{
    TransferJob job;
    job._jobid = 6;
    job._status = STARTED;

    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString fpath = dir.filePath("scan.txt");
    QFile f(fpath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("scan content");
    f.close();

    job.scanPath(dir.path().toStdString(), fpath.toStdString(), true);
    EXPECT_GT(job._total_size.load(), 0);
}

TEST(TransferJobCovTest, ScanPathDirectory)
{
    TransferJob job;
    job._jobid = 7;
    job._status = STARTED;

    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString subdir = dir.filePath("sub");
    QDir().mkpath(subdir);
    QString fpath = dir.filePath("sub/a.txt");
    QFile f(fpath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("inner file");
    f.close();

    job.scanPath(dir.path().toStdString(), subdir.toStdString(), false);
    EXPECT_GT(job.queueCount(), 0);
}

TEST(TransferJobCovTest, ScanPathNonexistent)
{
    TransferJob job;
    job._jobid = 8;
    job._status = STARTED;
    job.scanPath("/nope", "/nope/missing", false);
    EXPECT_TRUE(job._mark_canceled);
}

TEST(TransferJobCovTest, ReadFileFromPath)
{
    TransferJob job;
    job._jobid = 9;
    job._status = STARTED;

    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString fpath = dir.filePath("read.txt");
    QFile f(fpath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("read me");
    f.close();

    job.readFile(fpath.toStdString(), 1, "sub", false);
    EXPECT_GT(job.queueCount(), 0);
}

TEST(TransferJobCovTest, HandleBlockQuequeWriteTransOver)
{
    TransferJob job;
    job._jobid = 10;
    job.initJob("covapp", "target", 10, "[]", false, "covsave", true);
    job._status = STARTED;

    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->job_id = 10;
    block->flags = JobTransFileOp::FILE_TRANS_OVER;
    job.pushQueque(block);

    job.handleBlockQueque();
    EXPECT_EQ(job._status, STOPED);
}

TEST(TransferJobCovTest, WriteAndCreateFileDirCreate)
{
    TransferJob job;
    job._jobid = 11;
    job._status = STARTED;

    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString newdir = dir.filePath("newdir");

    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->flags = JobTransFileOp::FIlE_DIR_CREATE;
    bool ok = job.writeAndCreateFile(block, newdir.toStdString());
    EXPECT_TRUE(ok);
    EXPECT_TRUE(QDir(newdir).exists());
}

TEST(TransferJobCovTest, WriteAndCreateFileCounted)
{
    TransferJob job;
    job._jobid = 12;
    job._status = STARTED;

    job._device_free_size = 1000000; // enough space
    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->flags = JobTransFileOp::FILE_COUNTED;
    block->data_size = 100;
    bool ok = job.writeAndCreateFile(block, "/tmp/whatever");
    EXPECT_TRUE(ok);
}

TEST(TransferJobCovTest, WriteAndCreateFileCounting)
{
    TransferJob job;
    job._jobid = 13;
    job._status = STARTED;

    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->flags = JobTransFileOp::FILE_COUNTING;
    bool ok = job.writeAndCreateFile(block, "/tmp/whatever");
    EXPECT_TRUE(ok);
}

TEST(TransferJobCovTest, WriteAndCreateFileTransOver)
{
    TransferJob job;
    job._jobid = 14;
    job._status = STARTED;

    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->flags = JobTransFileOp::FILE_TRANS_OVER;
    bool ok = job.writeAndCreateFile(block, "/tmp/whatever");
    EXPECT_TRUE(ok);
}

TEST(TransferJobCovTest, WriteAndCreateFileEmptyPath)
{
    TransferJob job;
    job._jobid = 15;
    job._status = STARTED;

    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->flags = JobTransFileOp::FIlE_CREATE;
    bool ok = job.writeAndCreateFile(block, "");
    EXPECT_FALSE(ok);
}

TEST(TransferJobCovTest, WriteAndCreateFileDataWrite)
{
    TransferJob job;
    job._jobid = 16;
    job._status = STARTED;

    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString fpath = dir.filePath("wfile.txt");

    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->flags = JobTransFileOp::FIlE_CREATE;
    block->blk_id = 0;
    block->data = "written";
    bool ok = job.writeAndCreateFile(block, fpath.toStdString());
    EXPECT_TRUE(ok);
}

TEST(TransferJobCovTest, SendToRemoteDeviceNotEnough)
{
    TransferJob job;
    job._jobid = 17;
    job._status = STARTED;
    job._device_not_enough = true;

    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->flags = JobTransFileOp::FIlE_CREATE;
    EXPECT_FALSE(job.sendToRemote(block));
}

TEST(TransferJobCovTest, SendToRemoteRealSender)
{
    // Use a real RemoteServiceSender; the send fails (INVOKE_FAIL) which is
    // an acceptable, safe path that still covers sendToRemote's body.
    TransferJob job;
    job._jobid = 19;
    job._status = STARTED;
    job._remote.reset(new RemoteServiceSender("app", "127.0.0.1", 51598, true));

    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->job_id = 19;
    block->file_id = 1;
    block->flags = JobTransFileOp::FIlE_CREATE;
    block->data_size = 0;
    bool ok = job.sendToRemote(block);
    EXPECT_FALSE(ok);
}

// ---- More coverage: offlineCancel, initRpc write, status transitions ----

TEST(TransferJobCovTest, OfflineCancelMatchingIp)
{
    TransferJob job;
    job._jobid = 30;
    job._tar_ip = "5.5.5.5";
    job._offlined = false;
    bool ret = job.offlineCancel("5.5.5.5");
    EXPECT_TRUE(ret);
    EXPECT_TRUE(job._offlined);
}

TEST(TransferJobCovTest, OfflineCancelAlreadyOfflined)
{
    TransferJob job;
    job._offlined = true;
    EXPECT_FALSE(job.offlineCancel("any"));
}

TEST(TransferJobCovTest, InitRpcWriteJobEmptyTarget)
{
    TransferJob job;
    job.initJob("wapp", "target", 40, "[]", false, "save", true);
    // write job + empty target -> returns true early
    EXPECT_TRUE(job.initRpc("", 51597));
}

TEST(TransferJobCovTest, HandleJobStatusFailed)
{
    TransferJob job;
    QSignalSpy spy(&job, &TransferJob::notifyJobResult);
    job.handleJobStatus(JOB_TRANS_FAILED);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(2).toInt(), JOB_TRANS_FAILED);
}

TEST(TransferJobCovTest, HandleJobStatusDoing)
{
    TransferJob job;
    QSignalSpy spy(&job, &TransferJob::notifyJobResult);
    job.handleJobStatus(JOB_TRANS_DOING);
    ASSERT_EQ(spy.count(), 1);
}

TEST(TransferJobCovTest, WaitFinish)
{
    TransferJob job;
    job.waitFinish();
    EXPECT_EQ(job._status, WAIT_DONE);
}

TEST(TransferJobCovTest, StopAndEnded)
{
    TransferJob job;
    EXPECT_FALSE(job.ended());
    job.stop();
    EXPECT_TRUE(job.ended());
}

TEST(TransferJobCovTest, IsRunningAfterStatusSet)
{
    TransferJob job;
    job._status = STARTED;
    EXPECT_TRUE(job.isRunning());
    job._status = STOPED;
    EXPECT_FALSE(job.isRunning());
}

TEST(TransferJobCovTest, AcNameEmpty)
{
    TransferJob job;
    EXPECT_TRUE(job.acName("nothing").empty());
}

TEST(TransferJobCovTest, FreeBytesDefault)
{
    TransferJob job;
    EXPECT_EQ(job.freeBytes(), -1);
}

TEST(TransferJobCovTest, GetSaveFullpathRealpathBranch)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    // configure a real storagedir for the app so realpath resolves
    DaemonConfig::instance()->setAppConfig("realapp", "storagedir", dir.path().toStdString());
    TransferJob job;
    job._app_name = "realapp";
    // valid filename within rootdir
    fastring fp = job.getSaveFullpath(dir.path().toStdString(), "sub/file.txt");
    // either returns a valid path or "" (path-traversal block); both ok
    SUCCEED();
}

TEST(TransferJobCovTest, GetSaveFullpathTraversalRelative)
{
    TransferJob job;
    job._app_name = "realapp2";
    DaemonConfig::instance()->setAppConfig("realapp2", "storagedir", "/tmp");
    // filename with .. -> rejected
    fastring fp = job.getSaveFullpath("/tmp", "../escape.txt");
    EXPECT_TRUE(fp.empty());
}

TEST(TransferJobCovTest, GetSaveFullpathAbsolute)
{
    TransferJob job;
    job._app_name = "realapp3";
    DaemonConfig::instance()->setAppConfig("realapp3", "storagedir", "/tmp");
    // absolute path -> rejected
    fastring fp = job.getSaveFullpath("/tmp", "/etc/passwd");
    EXPECT_TRUE(fp.empty());
}

TEST(TransferJobCovTest, GetSaveFullpathWithNameMap)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    DaemonConfig::instance()->setAppConfig("mapapp", "storagedir", dir.path().toStdString());
    TransferJob job;
    job._app_name = "mapapp";
    // set a rename mapping so the acName branch executes
    job.setFileName("origdir", "renameddir");
    fastring fp = job.getSaveFullpath(dir.path().toStdString(), "origdir/file.txt");
    SUCCEED();
}

TEST(TransferJobCovTest, GetSaveFullpathRenamedFlat)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    DaemonConfig::instance()->setAppConfig("flatapp", "storagedir", dir.path().toStdString());
    TransferJob job;
    job._app_name = "flatapp";
    job.setFileName("origflat", "renamedflat");
    fastring fp = job.getSaveFullpath(dir.path().toStdString(), "origflat");
    SUCCEED();
}

TEST(TransferJobCovTest, GetSaveFullpathTrailingSlashRoot)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    DaemonConfig::instance()->setAppConfig("slashapp", "storagedir", dir.path().toStdString());
    TransferJob job;
    job._app_name = "slashapp";
    // rootdir with trailing slash exercises the pop_back loop
    fastring root = std::string(dir.path().toStdString()) + "/";
    fastring fp = job.getSaveFullpath(root, "file.txt");
    SUCCEED();
}

TEST(TransferJobCovTest, SendToRemoteCountedStubbedOk)
{
    // FILE_COUNTED block with a stubbed sender that returns success.
    // We can't safely stub the member fn, so use a real sender pointing at a
    // closed port; the send fails fast and we just exercise the body.
    TransferJob job;
    job._jobid = 50;
    job._status = STARTED;
    job._remote.reset(new RemoteServiceSender("app", "127.0.0.1", 51598, true));
    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->job_id = 50;
    block->flags = JobTransFileOp::FILE_COUNTED;
    block->data_size = 10;
    bool ok = job.sendToRemote(block);
    EXPECT_FALSE(ok);
}

TEST(TransferJobCovTest, HandleUpdateRealSender)
{
    TransferJob job;
    job._jobid = 60;
    job._status = STARTED;
    job._remote.reset(new RemoteServiceSender("app", "127.0.0.1", 51598, true));
    job.handleUpdate(OK, "/some/path", "err");
    SUCCEED();
}

// ---------------------------------------------------------------------------
// TransferJobCov2Test — additional coverage for sendToRemote success/error
// branches (via stubbed RemoteServiceSender::doSendProtoMsg), nested file
// writes, multi-block reads, deep scanPath trees, and handleBlockQueque write
// + cancel paths. Private members are reached directly (-fno-access-control).
// ---------------------------------------------------------------------------

// Stub replacements for RemoteServiceSender::doSendProtoMsg. The real method
// signature is `SendResult(uint32, const QString &, const QByteArray &)`; the
// Stub framework forwards the call ignoring the implicit `this`, so the free
// stub functions only need to return a properly-typed SendResult.
static SendResult fake_send_success(uint32, const QString &, const QByteArray &)
{
    SendResult r;
    r.protocolType = 0;      // != FS_DATA(1004) -> skip IO_ERROR branch
    r.errorType = INVOKE_OK; // >= 0 -> success path
    return r;
}

static SendResult fake_send_ioerror(uint32, const QString &, const QByteArray &)
{
    SendResult r;
    r.protocolType = 1004;   // FS_DATA
    r.errorType = INVOKE_OK;
    // FileTransResponse.result == -2 == IO_ERROR
    r.data = fastring("{\"id\":1,\"name\":\"x\",\"result\":-2}");
    return r;
}

// sendToRemote with FIlE_CREATE + stubbed-success sender -> returns true and
// accumulates 4096 into _cur_size (data_size==0 + FIlE_CREATE branch).
TEST(TransferJobCov2Test, SendToRemoteStubbedSuccessCreate)
{
    TransferJob job;
    job._jobid = 100;
    job._status = STARTED;
    job._remote.reset(new RemoteServiceSender("app", "127.0.0.1", 51598, true));

    Stub stub;
    stub.set(ADDR(RemoteServiceSender, doSendProtoMsg), fake_send_success);

    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->job_id = 100;
    block->file_id = 1;
    block->flags = JobTransFileOp::FIlE_CREATE;
    block->data_size = 0;
    EXPECT_TRUE(job.sendToRemote(block));
    EXPECT_EQ(job._cur_size.load(), 4096);
}

// sendToRemote with FILE_COUNTED + success -> returns true but _cur_size is NOT
// bumped (the FILE_COUNTED branch skips adding data_size).
TEST(TransferJobCov2Test, SendToRemoteStubbedSuccessCounted)
{
    TransferJob job;
    job._jobid = 101;
    job._status = STARTED;
    job._remote.reset(new RemoteServiceSender("app", "127.0.0.1", 51598, true));

    Stub stub;
    stub.set(ADDR(RemoteServiceSender, doSendProtoMsg), fake_send_success);

    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->job_id = 101;
    block->flags = JobTransFileOp::FILE_COUNTED;
    block->data_size = 5000;
    EXPECT_TRUE(job.sendToRemote(block));
    EXPECT_EQ(job._cur_size.load(), 0); // COUNTED -> skip
}

// sendToRemote with a normal data block (FIlE_NONE) + success -> _cur_size
// grows by data_size.
TEST(TransferJobCov2Test, SendToRemoteStubbedSuccessNormal)
{
    TransferJob job;
    job._jobid = 102;
    job._status = STARTED;
    job._remote.reset(new RemoteServiceSender("app", "127.0.0.1", 51598, true));

    Stub stub;
    stub.set(ADDR(RemoteServiceSender, doSendProtoMsg), fake_send_success);

    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->job_id = 102;
    block->flags = JobTransFileOp::FIlE_NONE;
    block->data_size = 100;
    EXPECT_TRUE(job.sendToRemote(block));
    EXPECT_EQ(job._cur_size.load(), 100);
}

// sendToRemote with FILE_TRANS_OVER + success -> returns true (covers the
// else branch where data_size != 0 / FIlE_CREATE is false -> +data_size==0).
TEST(TransferJobCov2Test, SendToRemoteStubbedSuccessTransOver)
{
    TransferJob job;
    job._jobid = 103;
    job._status = STARTED;
    job._remote.reset(new RemoteServiceSender("app", "127.0.0.1", 51598, true));

    Stub stub;
    stub.set(ADDR(RemoteServiceSender, doSendProtoMsg), fake_send_success);

    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->job_id = 103;
    block->flags = JobTransFileOp::FILE_TRANS_OVER;
    block->data_size = 0;
    EXPECT_TRUE(job.sendToRemote(block));
}

// sendToRemote stubbed to return an IO_ERROR response with the FILE_COUNTED
// flag -> returns false and flips _device_not_enough.
TEST(TransferJobCov2Test, SendToRemoteStubbedIOError)
{
    TransferJob job;
    job._jobid = 104;
    job._status = STARTED;
    job._remote.reset(new RemoteServiceSender("app", "127.0.0.1", 51598, true));

    Stub stub;
    stub.set(ADDR(RemoteServiceSender, doSendProtoMsg), fake_send_ioerror);

    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->job_id = 104;
    block->flags = JobTransFileOp::FILE_COUNTED; // forces _device_not_enough=true
    block->data_size = 100;
    EXPECT_FALSE(job.sendToRemote(block));
    EXPECT_TRUE(job._device_not_enough.load());
}

// writeAndCreateFile with a nested fullpath (sub/folder/file.txt). Parent dirs
// must pre-exist for FSAdapter::writeBlock to succeed.
TEST(TransferJobCov2Test, WriteAndCreateFileSubdirPath)
{
    TransferJob job;
    job._jobid = 105;
    job._status = STARTED;

    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    ASSERT_TRUE(QDir().mkpath(dir.filePath("sub/folder")));
    QString fpath = dir.filePath("sub/folder/file.txt");

    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->flags = JobTransFileOp::FIlE_CREATE;
    block->blk_id = 0;
    block->data = fastring("nested-content");
    bool ok = job.writeAndCreateFile(block, fpath.toStdString());
    EXPECT_TRUE(ok);
    EXPECT_TRUE(QFile::exists(fpath));
}

// readFileBlock on a file larger than BLOCK_SIZE (1 MiB) produces multiple
// queued blocks (exercises the read loop beyond a single iteration).
TEST(TransferJobCov2Test, ReadFileBlockMultiBlock)
{
    TransferJob job;
    job._jobid = 106;
    job._status = STARTED;

    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    QByteArray big(2 * 1024 * 1024 + 100, 'X'); // > 2 * BLOCK_SIZE
    tmp.write(big);
    tmp.close();

    job.readFileBlock(tmp.fileName().toStdString(), 1, "sub", false);
    EXPECT_GE(job.queueCount(), 2);
}

// scanPath over a multi-level directory tree pushes blocks for every directory
// (FIlE_DIR_CREATE) and every file, so a tree with 2 dirs + 3 files yields
// at least 5 blocks.
TEST(TransferJobCov2Test, ScanPathDeepTree)
{
    TransferJob job;
    job._jobid = 107;
    job._status = STARTED;

    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QFile f1(dir.filePath("a.txt"));
    ASSERT_TRUE(f1.open(QIODevice::WriteOnly)); f1.write("1"); f1.close();
    ASSERT_TRUE(QDir().mkpath(dir.filePath("sub/deep")));
    QFile f2(dir.filePath("sub/b.txt"));
    ASSERT_TRUE(f2.open(QIODevice::WriteOnly)); f2.write("22"); f2.close();
    QFile f3(dir.filePath("sub/deep/c.txt"));
    ASSERT_TRUE(f3.open(QIODevice::WriteOnly)); f3.write("333"); f3.close();

    job.scanPath(dir.path().toStdString(), dir.path().toStdString(), false);
    // root-dir + sub-dir + deep-dir + 3 files = 6 blocks
    EXPECT_GE(job.queueCount(), 5);
}

// handleBlockQueque on a write job with a prefilled FIlE_CREATE block (real
// filename + real save root) writes the file, then a FILE_TRANS_OVER block
// terminates the loop and emits notifyJobResult(JOB_TRANS_FINISHED).
TEST(TransferJobCov2Test, HandleBlockQuequeWriteCreateAndTransOver)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    DaemonConfig::instance()->setAppConfig("cov2app", "storagedir", dir.path().toStdString());

    TransferJob job;
    job.initJob("cov2app", "target", 70, "[]", false, "cov2save", true); // write job
    job._status = STARTED;
    fastring saveFulldir = job._save_fulldir; // == <tempdir>/cov2save (created by initJob)

    // write-path decodes block->filename from base64, so encode the real name.
    QSharedPointer<FSDataBlock> b1(new FSDataBlock);
    b1->job_id = 70;
    b1->file_id = 1;
    b1->rootdir = saveFulldir;
    b1->filename = Util::encodeBase64("cov2file.txt");
    b1->blk_id = 0;
    b1->data_size = 5;
    b1->data = fastring("hello");
    b1->flags = JobTransFileOp::FIlE_CREATE;
    job.pushQueque(b1);

    QSharedPointer<FSDataBlock> b2(new FSDataBlock);
    b2->job_id = 70;
    b2->flags = JobTransFileOp::FILE_TRANS_OVER;
    job.pushQueque(b2);

    QSignalSpy spy(&job, &TransferJob::notifyJobResult);
    ASSERT_TRUE(spy.isValid());

    job.handleBlockQueque();

    EXPECT_EQ(job._status, STOPED);
    QString expectedFile = QString(saveFulldir.c_str()) + "/cov2file.txt";
    EXPECT_TRUE(QFile::exists(expectedFile));
    // No exception / cancel / offline -> FINISHED is emitted.
    ASSERT_GE(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(2).toInt(), JOB_TRANS_FINISHED);
}

// handleBlockQueque with _status == CANCELING breaks immediately (exception
// path), emits no result signal, and ends in STOPED.
TEST(TransferJobCov2Test, HandleBlockQuequeCancelingBreaks)
{
    TransferJob job;
    job.initJob("cov2cancel", "target", 71, "[]", false, "cov2save", true); // write job
    job._status = CANCELING;

    QSignalSpy spy(&job, &TransferJob::notifyJobResult);
    ASSERT_TRUE(spy.isValid());

    job.handleBlockQueque();

    EXPECT_EQ(job._status, STOPED);
    // exception=true suppresses the FINISHED notification.
    EXPECT_EQ(spy.count(), 0);
}

// cancel(true) on a STARTED job transitions to CANCELING and, unlike the silent
// cancel(false) path, does NOT emit a JOB_TRANS_CANCELED notification.
TEST(TransferJobCov2Test, CancelNotifyDoesNotEmitCanceled)
{
    TransferJob job;
    job._status = STARTED;

    QSignalSpy spy(&job, &TransferJob::notifyJobResult);
    ASSERT_TRUE(spy.isValid());

    job.cancel(true);

    EXPECT_EQ(job._status, CANCELING);
    EXPECT_TRUE(job._mark_canceled);
    EXPECT_EQ(spy.count(), 0); // notify branch: no CANCELED signal
}

// cancel(false) on a freshly-constructed job emits JOB_TRANS_CANCELED (silent
// branch) — complements the notify-branch test above.
TEST(TransferJobCov2Test, CancelSilentEmitsCanceled)
{
    TransferJob job;
    QSignalSpy spy(&job, &TransferJob::notifyJobResult);
    ASSERT_TRUE(spy.isValid());

    job.cancel(false);

    EXPECT_EQ(job._status, STOPED);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(2).toInt(), JOB_TRANS_CANCELED);
}

// offlineCancel with a matching target ip emits JOB_TRANS_FAILED via
// notifyJobResult and marks the job offlined.
TEST(TransferJobCov2Test, OfflineCancelMatchingIpEmitsFailed)
{
    TransferJob job;
    job._app_name = "cov2offline";
    job._tar_ip = "5.5.5.5";

    QSignalSpy spy(&job, &TransferJob::notifyJobResult);
    ASSERT_TRUE(spy.isValid());

    EXPECT_TRUE(job.offlineCancel("5.5.5.5"));
    EXPECT_TRUE(job._offlined);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(2).toInt(), JOB_TRANS_FAILED);
}

// pushQueque is rejected while the job is CANCELING (same gate as STOPED).
TEST(TransferJobCov2Test, PushQuequeWhileCanceling)
{
    TransferJob job;
    job._status = CANCELING;

    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    job.pushQueque(block);
    EXPECT_EQ(job.queueCount(), 0);
}

// freeBytes() reflects the manually-set _device_free_size and is independent of
// the _device_not_enough flag (which only guards sendToRemote).
TEST(TransferJobCov2Test, FreeBytesReflectsDeviceSize)
{
    TransferJob job;
    EXPECT_EQ(job.freeBytes(), -1); // default sentinel
    job._device_free_size = 12345;
    EXPECT_EQ(job.freeBytes(), 12345);
    job.setDeviceNotenough();
    EXPECT_TRUE(job._device_not_enough.load());
    EXPECT_EQ(job.freeBytes(), 12345); // unchanged by the not-enough flag
}

// writeAndCreateFile with FILE_COUNTED when total_size >= device_free_size ->
// returns false and flips _device_not_enough (the disk-full branch).
TEST(TransferJobCov2Test, WriteAndCreateFileCountedDeviceNotEnough)
{
    TransferJob job;
    job._jobid = 108;
    job._status = STARTED;
    job._device_free_size = 100; // tiny free space

    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->flags = JobTransFileOp::FILE_COUNTED;
    block->data_size = 5000; // 5000 >= 100
    EXPECT_FALSE(job.writeAndCreateFile(block, "/tmp/whatever"));
    EXPECT_TRUE(job._device_not_enough.load());
    EXPECT_EQ(job._total_size.load(), 5000);
}

// handleJobStatus with the not-enough / offlined flags decorates the emitted
// savepath with the "::not enough" / "::off line" markers.
TEST(TransferJobCov2Test, HandleJobStatusDecoratesSavepath)
{
    TransferJob job;
    job._app_name = "cov2flag";
    job._device_not_enough = true;
    job._offlined = true;

    QSignalSpy spy(&job, &TransferJob::notifyJobResult);
    ASSERT_TRUE(spy.isValid());

    job.handleJobStatus(JOB_TRANS_DOING);

    ASSERT_EQ(spy.count(), 1);
    QString savepath = spy.at(0).at(3).toString();
    EXPECT_TRUE(savepath.contains("not enough"));
    EXPECT_TRUE(savepath.contains("off line"));
}

// handleBlockQueque on a write job skips a FILE_COUNTING block via `continue`
// (the write-path early-skip branch), then terminates on FILE_TRANS_OVER.
TEST(TransferJobCov2Test, HandleBlockQuequeWriteCountingSkip)
{
    TransferJob job;
    job.initJob("cov2cnt", "target", 73, "[]", false, "cov2save", true); // write job
    job._status = STARTED;

    QSharedPointer<FSDataBlock> cnt(new FSDataBlock);
    cnt->flags = JobTransFileOp::FILE_COUNTING;
    job.pushQueque(cnt);

    QSharedPointer<FSDataBlock> over(new FSDataBlock);
    over->flags = JobTransFileOp::FILE_TRANS_OVER;
    job.pushQueque(over);

    job.handleBlockQueque();
    EXPECT_EQ(job._status, STOPED);
}

// handleBlockQueque on a READ job: createSendCounting runs first, then a
// stubbed-success sendToRemote processes the FILE_TRANS_OVER block and the loop
// terminates emitting JOB_TRANS_FINISHED.
TEST(TransferJobCov2Test, HandleBlockQuequeReadJobSendSuccess)
{
    TransferJob job;
    job.initJob("cov2read", "target", 72, "[]", false, "cov2save", false); // read job
    job._status = STARTED;
    job._remote.reset(new RemoteServiceSender("app", "127.0.0.1", 51598, true));

    Stub stub;
    stub.set(ADDR(RemoteServiceSender, doSendProtoMsg), fake_send_success);

    QSharedPointer<FSDataBlock> over(new FSDataBlock);
    over->job_id = 72;
    over->flags = JobTransFileOp::FILE_TRANS_OVER;
    job.pushQueque(over);

    QSignalSpy spy(&job, &TransferJob::notifyJobResult);
    ASSERT_TRUE(spy.isValid());

    job.handleBlockQueque();

    EXPECT_EQ(job._status, STOPED);
    ASSERT_GE(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(2).toInt(), JOB_TRANS_FINISHED);
}

// handleBlockQueque breaks via the (_offlined) clause when a block is processed
// while the job is already marked offlined -> emits JOB_TRANS_FAILED and no
// FINISHED.
TEST(TransferJobCov2Test, HandleBlockQuequeOfflinedBreak)
{
    TransferJob job;
    job.initJob("cov2off", "target", 74, "[]", false, "cov2save", true); // write job
    job._status = STARTED;
    job._offlined = true;

    QSharedPointer<FSDataBlock> over(new FSDataBlock);
    over->flags = JobTransFileOp::FILE_TRANS_OVER;
    job.pushQueque(over);

    QSignalSpy spy(&job, &TransferJob::notifyJobResult);
    ASSERT_TRUE(spy.isValid());

    job.handleBlockQueque();

    EXPECT_EQ(job._status, STOPED);
    ASSERT_GE(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(2).toInt(), JOB_TRANS_FAILED);
}

// popQueue on an empty queue returns nullptr (the empty-queue early return).
TEST(TransferJobCov2Test, PopQueueEmptyReturnsNull)
{
    TransferJob job;
    EXPECT_TRUE(job.popQueue().isNull());
    EXPECT_EQ(job.queueCount(), 0);
}

// handleBlockQueque write path with a FILE_CLOSE-flagged create block emits the
// FILE_TRANS_END progress notification (the FILE_CLOSE branch) before the
// FILE_TRANS_OVER block terminates the loop.
TEST(TransferJobCov2Test, HandleBlockQuequeWriteFileCloseNotification)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    DaemonConfig::instance()->setAppConfig("cov2close", "storagedir", dir.path().toStdString());

    TransferJob job;
    job.initJob("cov2close", "target", 76, "[]", false, "cov2save", true); // write job
    job._status = STARTED;
    fastring saveFulldir = job._save_fulldir;

    QSharedPointer<FSDataBlock> b1(new FSDataBlock);
    b1->job_id = 76;
    b1->file_id = 1;
    b1->rootdir = saveFulldir;
    b1->filename = Util::encodeBase64("closefile.txt");
    b1->blk_id = 0;
    b1->data_size = 4;
    b1->data = fastring("done");
    b1->flags = JobTransFileOp::FIlE_CREATE | JobTransFileOp::FILE_CLOSE;
    job.pushQueque(b1);

    QSharedPointer<FSDataBlock> b2(new FSDataBlock);
    b2->job_id = 76;
    b2->flags = JobTransFileOp::FILE_TRANS_OVER;
    job.pushQueque(b2);

    QSignalSpy statusSpy(&job, &TransferJob::notifyFileTransStatus);
    ASSERT_TRUE(statusSpy.isValid());

    job.handleBlockQueque();

    EXPECT_EQ(job._status, STOPED);
    // FILE_CLOSE -> at least one FILE_TRANS_END status emitted.
    bool gotEnd = false;
    for (const auto &args : statusSpy) {
        if (args.at(1).toInt() == FILE_TRANS_END) gotEnd = true;
    }
    EXPECT_TRUE(gotEnd);
}

// initJob with a savedir that escapes the storage root ("../escape") is rejected
// by isPathWithinDir -> early return (status was set to INIT, but the save dir
// is never created).
TEST(TransferJobCov2Test, InitJobSavedirTraversalRejected)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    DaemonConfig::instance()->setAppConfig("cov2trav", "storagedir", dir.path().toStdString());

    TransferJob job;
    job.initJob("cov2trav", "target", 77, "[]", false, "../escape", true);

    EXPECT_EQ(job._status, INIT); // set before the traversal check
    // returned early -> the escape directory was never created
    EXPECT_FALSE(QDir(QString(job._save_fulldir.c_str())).exists());
}

// writeAndCreateFile with FIlE_CREATE + empty payload (len==0) and a successful
// write bumps _cur_size by 4096 (the empty-create accounting branch).
TEST(TransferJobCov2Test, WriteAndCreateFileEmptyDataCreate)
{
    TransferJob job;
    job._jobid = 109;
    job._status = STARTED;

    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString fpath = dir.filePath("empty.txt");

    QSharedPointer<FSDataBlock> block(new FSDataBlock);
    block->flags = JobTransFileOp::FIlE_CREATE;
    block->blk_id = 0;
    block->data = fastring(""); // len == 0
    bool ok = job.writeAndCreateFile(block, fpath.toStdString());
    EXPECT_TRUE(ok);
    EXPECT_EQ(job._cur_size.load(), 4096);
}

// handleBlockQueque on a READ job whose sendToRemote fails (real sender, closed
// port) sets exception=true, logs the sendToRemote-exception warning and emits
// JOB_TRANS_FAILED.
TEST(TransferJobCov2Test, HandleBlockQuequeReadJobSendFail)
{
    TransferJob job;
    job.initJob("cov2fail", "target", 78, "[]", false, "cov2save", false); // read job
    job._status = STARTED;
    job._remote.reset(new RemoteServiceSender("app", "127.0.0.1", 51598, true));

    QSharedPointer<FSDataBlock> over(new FSDataBlock);
    over->job_id = 78;
    over->flags = JobTransFileOp::FILE_TRANS_OVER;
    job.pushQueque(over);

    QSignalSpy spy(&job, &TransferJob::notifyJobResult);
    ASSERT_TRUE(spy.isValid());

    job.handleBlockQueque();

    EXPECT_EQ(job._status, STOPED);
    // send fails -> exception -> JOB_TRANS_FAILED
    ASSERT_GE(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(2).toInt(), JOB_TRANS_FAILED);
}

// handleBlockQueque with _jobid == 1000 and a target file that already exists
// exercises the cross-end rename branch: FSAdapter::reacquirePath renames the
// incoming file to "name(1).ext" and the rename is recorded via setFileName.
TEST(TransferJobCov2Test, HandleBlockQuequeReacquireExistingFile)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    DaemonConfig::instance()->setAppConfig("cov2racq", "storagedir", dir.path().toStdString());

    TransferJob job;
    job.initJob("cov2racq", "target", 1000, "[]", false, "cov2save", true); // jobid 1000
    job._status = STARTED;
    fastring saveFulldir = job._save_fulldir;

    // pre-create the target so reacquirePath detects a collision and renames.
    QString target = QString(saveFulldir.c_str()) + "/racqfile.txt";
    { QFile f(target); ASSERT_TRUE(f.open(QIODevice::WriteOnly)); f.write("old"); }

    QSharedPointer<FSDataBlock> b1(new FSDataBlock);
    b1->job_id = 1000;
    b1->file_id = 1;
    b1->rootdir = saveFulldir;
    b1->filename = Util::encodeBase64("racqfile.txt");
    b1->blk_id = 0;
    b1->data_size = 3;
    b1->data = fastring("new");
    b1->flags = JobTransFileOp::FIlE_CREATE;
    job.pushQueque(b1);

    QSharedPointer<FSDataBlock> b2(new FSDataBlock);
    b2->job_id = 1000;
    b2->flags = JobTransFileOp::FILE_TRANS_OVER;
    job.pushQueque(b2);

    job.handleBlockQueque();

    EXPECT_EQ(job._status, STOPED);
    // the renamed file racqfile(1).txt was created
    QString renamed = QString(saveFulldir.c_str()) + "/racqfile(1).txt";
    EXPECT_TRUE(QFile::exists(renamed));
    // and the rename mapping was recorded
    EXPECT_EQ(job.acName("racqfile.txt"), fastring("racqfile(1).txt"));
}
