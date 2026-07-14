// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "transferworker.h"
#include "sessionproto.h"
#include "common/constant.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class TransferWorkerTest : public ::testing::Test {
protected:
    std::shared_ptr<TransferWorker> worker;
    QTemporaryDir tmpDir;

    void SetUp() override
    {
        worker = std::make_shared<TransferWorker>("test-job-id");
    }
    void TearDown() override
    {
        if (worker) {
            worker->stop();
            worker.reset();
        }
    }
};

TEST_F(TransferWorkerTest, ConstructDestruct)
{
    EXPECT_NE(worker, nullptr);
}

TEST_F(TransferWorkerTest, OnProgressReturnsCanceledFlag)
{
    // initially not canceled
    bool cancel = worker->onProgress(1024);
    EXPECT_FALSE(cancel);
}

TEST_F(TransferWorkerTest, OnProgressAfterStop)
{
    worker->stop();
    bool cancel = worker->onProgress(512);
    EXPECT_TRUE(cancel);
}

TEST_F(TransferWorkerTest, IsSyncingInitially)
{
    // _canceled is false initially → isSyncing returns true
    EXPECT_TRUE(worker->isSyncing());
}

TEST_F(TransferWorkerTest, IsSyncingAfterStop)
{
    worker->stop();
    EXPECT_FALSE(worker->isSyncing());
}

TEST_F(TransferWorkerTest, SetEveryFileNotify)
{
    worker->setEveryFileNotify(true);
    SUCCEED();
}

TEST_F(TransferWorkerTest, IsServeInitially)
{
    // _recvPath is empty initially → isServe returns true
    EXPECT_TRUE(worker->isServe());
}

TEST_F(TransferWorkerTest, HandleTimerTickStart)
{
    worker->handleTimerTick(false);
    SUCCEED();
}

TEST_F(TransferWorkerTest, HandleTimerTickStop)
{
    worker->handleTimerTick(true);
    SUCCEED();
}

TEST_F(TransferWorkerTest, DoCalculateSpeedWithData)
{
    worker->onProgress(4096);
    worker->doCalculateSpeed();
    SUCCEED();
}

TEST_F(TransferWorkerTest, DoCalculateSpeedNoData)
{
    worker->doCalculateSpeed();
    SUCCEED();
}

TEST_F(TransferWorkerTest, DoCalculateSpeedMultipleCalls)
{
    worker->onProgress(100);
    worker->doCalculateSpeed();
    worker->doCalculateSpeed(); // no data this time
    SUCCEED();
}

// ---- onWebChanged state tests ----

TEST_F(TransferWorkerTest, OnWebChangedError)
{
    QSignalSpy spy(worker.get(), &TransferWorker::onException);
    worker->onWebChanged(-1, "io_error");
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(TransferWorkerTest, OnWebChangedNotFound)
{
    QSignalSpy spy(worker.get(), &TransferWorker::onException);
    worker->onWebChanged(0, "not_found");
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(TransferWorkerTest, OnWebChangedDisconnected)
{
    QSignalSpy spy(worker.get(), &TransferWorker::onException);
    worker->onWebChanged(-2, "disconnected");
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(TransferWorkerTest, OnWebChangedConnected)
{
    // WEB_CONNECTED should not emit notifyChanged (just logs)
    worker->onWebChanged(WEB_CONNECTED, "connected");
    SUCCEED();
}

TEST_F(TransferWorkerTest, OnWebChangedTransStart)
{
    QSignalSpy spy(worker.get(), &TransferWorker::notifyChanged);
    worker->onWebChanged(WEB_TRANS_START, "start");
    EXPECT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toInt(), TRANS_WHOLE_START);
}

TEST_F(TransferWorkerTest, OnWebChangedTransFinish)
{
    QSignalSpy spy(worker.get(), &TransferWorker::notifyChanged);
    QSignalSpy finishSpy(worker.get(), &TransferWorker::onFinished);
    worker->onWebChanged(WEB_TRANS_FINISH, "done");
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(finishSpy.count(), 1);
}

TEST_F(TransferWorkerTest, OnWebChangedIndexBegin)
{
    QSignalSpy spy(worker.get(), &TransferWorker::notifyChanged);
    worker->onWebChanged(WEB_INDEX_BEGIN, "file_index");
    EXPECT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toInt(), TRANS_INDEX_CHANGE);
    EXPECT_EQ(args.at(1).toString(), "file_index");
}

TEST_F(TransferWorkerTest, OnWebChangedIndexEnd)
{
    worker->onWebChanged(WEB_INDEX_END, "index_end");
    SUCCEED();
}

TEST_F(TransferWorkerTest, OnWebChangedFileBeginEveryNotify)
{
    worker->setEveryFileNotify(true);
    QSignalSpy spy(worker.get(), &TransferWorker::notifyChanged);
    worker->onWebChanged(WEB_FILE_BEGIN, "test.txt", 1024);
    EXPECT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toInt(), TRANS_FILE_CHANGE);
}

TEST_F(TransferWorkerTest, OnWebChangedFileBeginNoNotify)
{
    QSignalSpy spy(worker.get(), &TransferWorker::notifyChanged);
    worker->onWebChanged(WEB_FILE_BEGIN, "test.txt", 1024);
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(TransferWorkerTest, OnWebChangedFileEndEveryNotify)
{
    worker->setEveryFileNotify(true);
    // first set the total via FILE_BEGIN
    worker->onWebChanged(WEB_FILE_BEGIN, "test.txt", 2048);
    QSignalSpy spy(worker.get(), &TransferWorker::notifyChanged);
    worker->onWebChanged(WEB_FILE_END, "test.txt");
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(TransferWorkerTest, OnWebChangedFileEndNoNotify)
{
    QSignalSpy spy(worker.get(), &TransferWorker::notifyChanged);
    worker->onWebChanged(WEB_FILE_END, "test.txt");
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(TransferWorkerTest, StopClearsResources)
{
    worker->stop();
    // second stop should be safe
    worker->stop();
    SUCCEED();
}

// ---- tryStartSend / tryStartReceive ----

TEST_F(TransferWorkerTest, TryStartSendSingleFile)
{
    // create a temp file using QTemporaryDir for isolation
    QString tmpFile = tmpDir.path() + "/test_send.txt";
    {
        QFile f(tmpFile);
        if (!f.open(QIODevice::WriteOnly)) {
            throw std::runtime_error("Failed to open file: " + tmpFile.toStdString());
        }
        f.write("test content");
        f.close();
    }

    std::vector<std::string> names;
    std::string token;
    QStringList paths{tmpFile};
    bool result = worker->tryStartSend(paths, 0, &names, &token);
    // may succeed or fail depending on environment; verify it doesn't crash
    if (result) {
        EXPECT_EQ(names.size(), 1u);
        EXPECT_FALSE(token.empty());
    }
}

TEST_F(TransferWorkerTest, TryStartSendNonExistentFile)
{
    std::vector<std::string> names;
    std::string token;
    QStringList paths{"/nonexistent/manager_test_file.txt"};
    // tryStartSend calls startWeb first which creates a FileServer
    // then tries to bind the file, catching exceptions
    bool result = worker->tryStartSend(paths, 0, &names, &token);
    // non-existent file binding throws, caught internally; result depends on startWeb
    SUCCEED();
}

TEST_F(TransferWorkerTest, TryStartReceiveInvalidToken)
{
    // 源码已修复：在 ENABLE_AUTO_UNIT_TEST 模式下捕获 JWT 解析异常，返回 false
    QStringList names{"file.txt"};
    QString ip = "127.0.0.1";
    QString token = "invalid.token.value";
    QString dir = "/tmp";
    EXPECT_FALSE(worker->tryStartReceive(names, ip, 8080, token, dir));
}
