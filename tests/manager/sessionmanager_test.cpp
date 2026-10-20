// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "sessionmanager.h"
#include "sessionworker.h"
#include "transferworker.h"
#include "sessionproto.h"

#include <QSignalSpy>
#include <QStandardPaths>
#include <QDir>
#include "stub.h"

class SessionManagerTest : public ::testing::Test {
protected:
    SessionManager *mgr = nullptr;

    void SetUp() override { mgr = new SessionManager(); }
    void TearDown() override
    {
        if (mgr)
            delete mgr;
    }
};

TEST_F(SessionManagerTest, ConstructDestruct)
{
    EXPECT_NE(mgr, nullptr);
}

TEST_F(SessionManagerTest, SetStorageRoot)
{
    mgr->setStorageRoot("/tmp/test_storage");
    SUCCEED();
}

TEST_F(SessionManagerTest, UpdateSaveFolder)
{
    mgr->setStorageRoot("/tmp/test_save");
    mgr->updateSaveFolder("subfolder");
    SUCCEED();
}

TEST_F(SessionManagerTest, UpdateSaveFolderEmpty)
{
    mgr->setStorageRoot("");
    mgr->updateSaveFolder("downloads");
    SUCCEED();
}

TEST_F(SessionManagerTest, UpdateSaveFolderEmptyBoth)
{
    mgr->setStorageRoot("");
    mgr->updateSaveFolder("");
    SUCCEED();
}

TEST_F(SessionManagerTest, UpdatePin)
{
    mgr->updatePin("1234");
    SUCCEED();
}

TEST_F(SessionManagerTest, UpdateLoginStatus)
{
    QString ip = "192.168.1.100";
    mgr->updateLoginStatus(ip, true);
    SUCCEED();
}

TEST_F(SessionManagerTest, HandleTransCount)
{
    QSignalSpy spy(mgr, &SessionManager::notifyTransChanged);
    mgr->handleTransCount("file1.txt;file2.txt", 1024);
    EXPECT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toInt(), 50); // TRANS_COUNT_SIZE
    EXPECT_EQ(args.at(1).toString(), "file1.txt;file2.txt");
    EXPECT_EQ(args.at(2).toULongLong(), 1024ull);
}

TEST_F(SessionManagerTest, HandleCancelTransWithReason)
{
    QSignalSpy spy(mgr, &SessionManager::notifyTransChanged);
    mgr->handleCancelTrans("192.168.1.100", "net_error");
    // TRANS_EXCEPTION = 49
    EXPECT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toInt(), 49);
}

TEST_F(SessionManagerTest, HandleCancelTransNoReason)
{
    QSignalSpy spy(mgr, &SessionManager::notifyTransChanged);
    mgr->handleCancelTrans("192.168.1.100", "");
    // TRANS_CANCELED = 48
    EXPECT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toInt(), 48);
}

TEST_F(SessionManagerTest, HandleTransDataValidEndpoint)
{
    // handleTransData with valid endpoint tries to create TransferWorker + recvFiles
    // 源码已修复：在 ENABLE_AUTO_UNIT_TEST 模式下 tryStartReceive 捕获 JWT 解析异常
    mgr->handleTransData("127.0.0.1:8080:token123", QStringList{"file.txt"});
    SUCCEED();
}

TEST_F(SessionManagerTest, HandleTransDataInvalidEndpoint)
{
    mgr->handleTransData("invalid:endpoint", QStringList{"file.txt"});
    SUCCEED();
}

TEST_F(SessionManagerTest, HandleRpcResult)
{
    QSignalSpy spy(mgr, &SessionManager::notifyAsyncRpcResult);
    mgr->handleRpcResult(REQ_LOGIN, R"({"status":"ok"})");
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(SessionManagerTest, HandleFileCountedEmptyIp)
{
    mgr->handleFileCounted("", QStringList{"file.txt"}, 100);
    // should return early without emitting
    SUCCEED();
}

TEST_F(SessionManagerTest, HandleFileCountedValidIp)
{
    mgr->handleFileCounted("192.168.1.50", QStringList{"a.txt", "b.txt"}, 500);
    SUCCEED();
}

TEST_F(SessionManagerTest, HandleTransFinish)
{
    // handleTransFinish releases worker by jobid - with unknown jobid it just warns
    mgr->handleTransFinish("unknown_jobid");
    SUCCEED();
}

TEST_F(SessionManagerTest, HandleTransException)
{
    mgr->handleTransException("10.0.0.1", "io_error");
    SUCCEED();
}

TEST_F(SessionManagerTest, CancelSyncFileWithNetError)
{
    mgr->cancelSyncFile("192.168.1.1", "net_error timeout");
    SUCCEED();
}

TEST_F(SessionManagerTest, CancelSyncFileNormal)
{
    mgr->cancelSyncFile("192.168.1.1", "user_canceled");
    SUCCEED();
}

TEST_F(SessionManagerTest, CancelSyncFileEmptyReason)
{
    mgr->cancelSyncFile("192.168.1.1", "");
    SUCCEED();
}

TEST_F(SessionManagerTest, SetSessionExtCallback)
{
    auto cb = [](int32_t mask, const picojson::value &, std::string *) -> bool {
        return false;
    };
    mgr->setSessionExtCallback(cb);
    SUCCEED();
}

TEST_F(SessionManagerTest, SendRpcRequest)
{
    // sendRpcRequest with empty target won't actually send (no connected session)
    mgr->sendRpcRequest("", REQ_LOGIN, "{}");
    SUCCEED();
}

TEST_F(SessionManagerTest, SessionDisconnect)
{
    mgr->sessionDisconnect("127.0.0.1");
    SUCCEED();
}

// ---- Tests with stubbed SessionWorker methods to cover network paths ----

static bool stub_startListen_true(SessionWorker *, int) { return true; }
static bool stub_startListen_false(SessionWorker *, int) { return false; }
static bool stub_netTouch_true(SessionWorker *, QString &, int) { return true; }
static bool stub_netTouch_false(SessionWorker *, QString &, int) { return false; }
static bool stub_isClientLogin_true(SessionWorker *, QString &) { return true; }
static bool stub_isClientLogin_false(SessionWorker *, QString &) { return false; }
static bool stub_isServerLogined_true(SessionWorker *, const QString &) { return true; }

TEST_F(SessionManagerTest, SessionListenSuccess)
{
    Stub stub;
    stub.set(ADDR(SessionWorker, startListen), stub_startListen_true);
    mgr->sessionListen(8080);
    SUCCEED();
}

TEST_F(SessionManagerTest, SessionListenFail)
{
    Stub stub;
    stub.set(ADDR(SessionWorker, startListen), stub_startListen_false);
    mgr->sessionListen(9999);
    SUCCEED();
}

TEST_F(SessionManagerTest, SessionPingSuccess)
{
    Stub stub;
    stub.set(ADDR(SessionWorker, netTouch), stub_netTouch_true);
    EXPECT_TRUE(mgr->sessionPing("127.0.0.1", 8080));
}

TEST_F(SessionManagerTest, SessionPingFail)
{
    Stub stub;
    stub.set(ADDR(SessionWorker, netTouch), stub_netTouch_false);
    EXPECT_FALSE(mgr->sessionPing("127.0.0.1", 9999));
}

TEST_F(SessionManagerTest, SessionConnectAlreadyLoggedIn)
{
    Stub stub;
    stub.set(ADDR(SessionWorker, isClientLogin), stub_isClientLogin_true);
    int result = mgr->sessionConnect("192.168.1.1", 8080, "1234");
    EXPECT_EQ(result, 1);
}

TEST_F(SessionManagerTest, SessionConnectReusesServerSideSession)
{
    Stub stub;
    stub.set(ADDR(SessionWorker, isClientLogin), stub_isClientLogin_false);
    stub.set(ADDR(SessionWorker, isServerLogined), stub_isServerLogined_true);
    // A new outgoing connection would fail; the server-side session must be
    // reused without even trying it.
    stub.set(ADDR(SessionWorker, netTouch), stub_netTouch_false);
    int result = mgr->sessionConnect("10.0.0.9", 8080, "1234");
    EXPECT_EQ(result, 1);
}

TEST_F(SessionManagerTest, SessionConnectNetTouchFail)
{
    Stub stub;
    stub.set(ADDR(SessionWorker, isClientLogin), stub_isClientLogin_false);
    stub.set(ADDR(SessionWorker, netTouch), stub_netTouch_false);
    int result = mgr->sessionConnect("192.168.1.1", 8080, "1234");
    EXPECT_EQ(result, -1);
}

TEST_F(SessionManagerTest, SessionConnectSuccess)
{
    Stub stub;
    stub.set(ADDR(SessionWorker, isClientLogin), stub_isClientLogin_false);
    stub.set(ADDR(SessionWorker, netTouch), stub_netTouch_true);
    int result = mgr->sessionConnect("192.168.1.1", 8080, "1234");
    EXPECT_EQ(result, 0);
}

// ---- TransferWorker stubs for sendFiles/recvFiles ----
static bool stub_tryStartSend_true(TransferWorker *, QStringList, int, std::vector<std::string> *, std::string *) { return true; }
static bool stub_tryStartSend_false(TransferWorker *, QStringList, int, std::vector<std::string> *, std::string *) { return false; }
static bool stub_tryStartReceive_true(TransferWorker *, QStringList, QString &, int, QString &, QString &) { return true; }
static bool stub_tryStartReceive_false(TransferWorker *, QStringList, QString &, int, QString &, QString &) { return false; }

TEST_F(SessionManagerTest, SendFilesFail)
{
    Stub stub;
    stub.set(ADDR(TransferWorker, tryStartSend), stub_tryStartSend_false);
    QString ip = "192.168.1.1";
    mgr->sendFiles(ip, 8080, QStringList{"/tmp/test.txt"});
    SUCCEED();
}

TEST_F(SessionManagerTest, SendFilesTunnelRegistersAndReleasesService)
{
    Stub stub;
    stub.set(ADDR(TransferWorker, tryStartSend), stub_tryStartSend_true);

    QString registeredId;
    quint16 registeredPort = 0;
    QString releasedId;
    mgr->setTunnelHandlers(
            [&](const QString &id, quint16 port) {
                registeredId = id;
                registeredPort = port;
                return true;
            },
            [&](const QString &id) { releasedId = id; },
            [](const QString &, const QString &) -> quint16 { return 0; },
            [](const QString &) {});

    QString ip = "10.0.0.7";
    mgr->sendFiles(ip, 23456, QStringList{"/tmp/test.txt"}, true);

    EXPECT_FALSE(registeredId.isEmpty());
    EXPECT_EQ(registeredPort, 23456);

    mgr->handleTransFinish(ip);
    EXPECT_EQ(releasedId, registeredId);
}

TEST_F(SessionManagerTest, HandleTransDataTunnelUsesForwardAndStopsIt)
{
    Stub stub;
    stub.set(ADDR(TransferWorker, tryStartReceive), stub_tryStartReceive_true);

    QString startedId;
    QString stoppedId;
    mgr->setTunnelHandlers(
            [](const QString &, quint16) { return true; },
            [](const QString &) {},
            [&](const QString &id, const QString &) -> quint16 {
                startedId = id;
                return 4567;
            },
            [&](const QString &id) { stoppedId = id; });

    const QString serviceId = QStringLiteral("2f8a6a44-7a2e-4b38-a0e5-2a7d1b8a7c11");
    mgr->handleTransData("10.0.0.9:8080:tok", QStringList{"file.txt"}, true, serviceId);
    EXPECT_EQ(startedId, serviceId);
    EXPECT_TRUE(stoppedId.isEmpty());

    // Completion is reported with the peer address, not the loopback address.
    mgr->handleTransFinish("10.0.0.9");
    EXPECT_EQ(stoppedId, serviceId);
}

TEST_F(SessionManagerTest, HandleTransDataRejectsReservedServiceName)
{
    Stub stub;
    stub.set(ADDR(TransferWorker, tryStartReceive), stub_tryStartReceive_false);

    int startCalls = 0;
    mgr->setTunnelHandlers(
            [](const QString &, quint16) { return true; },
            [](const QString &) {},
            [&](const QString &, const QString &) -> quint16 {
                ++startCalls;
                return 4567;
            },
            [](const QString &) {});

    mgr->handleTransData("10.0.0.9:8080:tok", QStringList{"file.txt"}, true,
                         QStringLiteral("barrier"));
    // Not a generated transfer service: fall back to the direct endpoint and
    // never touch the barrier forward.
    EXPECT_EQ(startCalls, 0);
}

TEST_F(SessionManagerTest, HandleTransDataTunnelForwardFailureCleansUp)
{
    Stub stub;
    stub.set(ADDR(TransferWorker, tryStartReceive), stub_tryStartReceive_false);

    QString stoppedId;
    mgr->setTunnelHandlers(
            [](const QString &, quint16) { return true; },
            [](const QString &) {},
            [](const QString &, const QString &) -> quint16 { return 4567; },
            [&](const QString &id) { stoppedId = id; });

    const QString serviceId = QStringLiteral("3a9b7b55-8b3f-4c49-b1f6-3b8e2c9b8d22");
    mgr->handleTransData("10.0.0.9:8080:tok", QStringList{"file.txt"}, true, serviceId);
    EXPECT_EQ(stoppedId, serviceId);
}
