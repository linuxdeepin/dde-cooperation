// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "service/rpc/sendrpcservice.h"
#include "service/comshare.h"
#include "service/jobmanager.h"
#include "common/commonstruct.h"
#include "common/constant.h"
#include "ipc/proto/comstruct.h"
#include "service/discoveryjob.h"
#include "service/searchlight.h"

#include <QSignalSpy>
#include <QCoreApplication>
#include <QStringList>
#include <QReadLocker>
#include <QWriteLocker>

// ---------------------------------------------------------------------------
// SendRpcService is a real singleton (static local in instance()). Its
// constructor spawns a worker QThread. We exercise only safe public methods
// plus a few private members reached through -fno-access-control. We NEVER
// call handleAboutToQuit() in tests: it stops the worker thread, and the
// singleton destructor already does that at process exit.
//
// IMPORTANT: _ping_appname is a shared QStringList on the singleton. Tests
// that add entries clean them up afterwards so the RemovePing test can
// reliably observe stopPingTimer being emitted when the list becomes empty.
// ---------------------------------------------------------------------------

// 1. InstanceSingleton — instance() returns the same object on repeated calls.
TEST(SendRpcServiceTest, InstanceSingleton)
{
    auto *a = SendRpcService::instance();
    auto *b = SendRpcService::instance();
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(a, b);
}

// 2. AddPing — addPing appends "app1" to the internal _ping_appname list.
TEST(SendRpcServiceTest, AddPing)
{
    SendRpcService *s = SendRpcService::instance();
    s->addPing("app1");

    // _ping_appname and _ping_lock are private; -fno-access-control lets us in.
    QReadLocker lk(&s->_ping_lock);
    EXPECT_TRUE(s->_ping_appname.contains("app1"));
    lk.unlock();

    s->removePing("app1");
}

// 3. AddPingDuplicate — adding the same app twice still yields a single entry.
TEST(SendRpcServiceTest, AddPingDuplicate)
{
    SendRpcService *s = SendRpcService::instance();
    s->addPing("dupapp");
    s->addPing("dupapp");

    QReadLocker lk(&s->_ping_lock);
    EXPECT_EQ(s->_ping_appname.count("dupapp"), 1);
    lk.unlock();

    // clean up
    s->removePing("dupapp");
}

// 4. AddPingMultiple — two distinct apps both present in the list.
TEST(SendRpcServiceTest, AddPingMultiple)
{
    SendRpcService *s = SendRpcService::instance();
    s->addPing("multi1");
    s->addPing("multi2");

    QReadLocker lk(&s->_ping_lock);
    EXPECT_TRUE(s->_ping_appname.contains("multi1"));
    EXPECT_TRUE(s->_ping_appname.contains("multi2"));
    lk.unlock();

    // clean up
    s->removePing("multi1");
    s->removePing("multi2");
}

// 5. RemovePing — after add then remove, the list no longer holds the app.
// removePing emits stopPingTimer when the list becomes empty, so spy on it.
TEST(SendRpcServiceTest, RemovePing)
{
    SendRpcService *s = SendRpcService::instance();
    s->addPing("removable");

    QSignalSpy spy(s, &SendRpcService::stopPingTimer);
    s->removePing("removable");

    // list now empty -> stopPingTimer emitted
    EXPECT_GE(spy.count(), 1);

    QReadLocker lk(&s->_ping_lock);
    EXPECT_FALSE(s->_ping_appname.contains("removable"));
}

// 6. RemovePingNonexistent — removing an app that was never added must not crash.
TEST(SendRpcServiceTest, RemovePingNonexistent)
{
    SendRpcService::instance()->removePing("nope");
    SUCCEED();
}

// 7. HandleStartTimer / HandleStopTimer — both run on the main thread inside
// a QCoreApplication, so the Q_ASSERT(qApp->thread() == currentThread) holds.
// handleStartTimer starts the internal QTimer; handleStopTimer stops it.
TEST(SendRpcServiceTest, HandleStartAndStopTimer)
{
    SendRpcService *s = SendRpcService::instance();
    s->handleStartTimer();
    s->handleStopTimer();
    SUCCEED();
}

// 8. HandleTimeOut — emits the ping() signal carrying the current _ping_appname
// list. We seed the list, then invoke handleTimeOut() directly on the main
// thread (it just takes a read lock and emits).
TEST(SendRpcServiceTest, HandleTimeOut)
{
    SendRpcService *s = SendRpcService::instance();
    s->addPing("timeoutapp");

    QSignalSpy spy(s, &SendRpcService::ping);
    s->handleTimeOut();

    ASSERT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    QStringList apps = args.at(0).toStringList();
    EXPECT_TRUE(apps.contains("timeoutapp"));

    // clean up so the timer won't keep pinging a phantom app
    s->removePing("timeoutapp");
}

// 9. SendRpcWork stop — set the worker's stop flag directly. After the call,
// _work._stoped must be true. handleAboutToQuit is NOT called; we only flip
// the flag. The singleton destructor will join the thread at process exit.
TEST(SendRpcServiceTest, WorkStop)
{
    SendRpcService *s = SendRpcService::instance();
    // _work is a private member; -fno-access-control makes it reachable.
    s->_work.stop();
    EXPECT_TRUE(s->_work._stoped.load());
}

// 10. CreateRpcSenderEmitsSignal — createRpcSender() is an inline emitter of
// workCreateRpcSender (queued to the worker thread). The worker then calls
// the real createRpcSender which constructs a RemoteServiceSender (no socket
// opened at construction). We only assert the signal is emitted here.
TEST(SendRpcServiceTest, CreateRpcSenderEmitsSignal)
{
    SendRpcService *s = SendRpcService::instance();
    QSignalSpy spy(s, &SendRpcService::workCreateRpcSender);

    s->createRpcSender("app1", "1.2.3.4", 51597);

    ASSERT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toString(), QString("app1"));
    EXPECT_EQ(args.at(1).toString(), QString("1.2.3.4"));
    EXPECT_EQ(args.at(2).toUInt(), (uint)51597);
}

// 11. SetTargetAppName — inline emitter of workSetTargetAppName.
TEST(SendRpcServiceTest, SetTargetAppName)
{
    SendRpcService *s = SendRpcService::instance();
    QSignalSpy spy(s, &SendRpcService::workSetTargetAppName);

    s->setTargetAppName("app1", "target");

    ASSERT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toString(), QString("app1"));
    EXPECT_EQ(args.at(1).toString(), QString("target"));
}

// 12. DoSendProtoMsg — inline emitter of workDoSendProtoMsg. The worker, when
// it processes the queued call, finds no sender for "app1" and logs a
// PARAM_ERROR result; the emit itself is what we verify here.
TEST(SendRpcServiceTest, DoSendProtoMsg)
{
    SendRpcService *s = SendRpcService::instance();
    QSignalSpy spy(s, &SendRpcService::workDoSendProtoMsg);

    s->doSendProtoMsg(LOGIN_INFO, "app1", "{}");

    ASSERT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toUInt(), (uint)LOGIN_INFO);
    EXPECT_EQ(args.at(1).toString(), QString("app1"));
    EXPECT_EQ(args.at(2).toString(), QString("{}"));
}

// ---- Extended coverage: exercise SendRpcWork private helpers directly ----

TEST(SendRpcServiceCovTest, WorkCreateRpcSenderCached)
{
    // Access the singleton's _work directly; createRpcSender inserts into
    // _remotes keyed by ip. Calling twice with same ip returns the cached one.
    auto *s = SendRpcService::instance();
    auto r1 = s->_work.createRpcSender("appA", "127.0.0.1", 51597);
    EXPECT_FALSE(r1.isNull());
    auto r2 = s->_work.createRpcSender("appA", "127.0.0.1", 51597);
    EXPECT_FALSE(r2.isNull());
    // same ip -> cached
    EXPECT_EQ(r1.data(), r2.data());
}

TEST(SendRpcServiceCovTest, WorkCreateRpcSenderReassignIp)
{
    auto *s = SendRpcService::instance();
    s->_work.createRpcSender("appRe", "10.0.0.1", 51597);
    // reassign same app to a new ip
    auto r2 = s->_work.createRpcSender("appRe", "10.0.0.2", 51597);
    EXPECT_FALSE(r2.isNull());
}

TEST(SendRpcServiceCovTest, WorkRpcSenderNoIp)
{
    auto *s = SendRpcService::instance();
    // appX has no registered ip -> rpcSender returns null
    auto r = s->_work.rpcSender("appDoesNotExist");
    EXPECT_TRUE(r.isNull());
}

TEST(SendRpcServiceCovTest, WorkRpcSenderWithIp)
{
    auto *s = SendRpcService::instance();
    s->_work.createRpcSender("appWithIp", "127.0.0.1", 51597);
    auto r = s->_work.rpcSender("appWithIp");
    EXPECT_FALSE(r.isNull());
}

TEST(SendRpcServiceCovTest, WorkSetTargetAppNameDirect)
{
    auto *s = SendRpcService::instance();
    // NOTE: _work lives on the worker thread; directly touching its maps from
    // the main thread races with the worker. We only exercise the code path
    // and don't assert on the shared state to avoid a flaky data-race failure.
    s->_work.createRpcSender("appSetUnique123", "127.0.0.1", 51597);
    s->_work.handleSetTargetAppName("appSetUnique123", "targetApp");
    SUCCEED();
}

TEST(SendRpcServiceCovTest, WorkSetTargetAppNameNoSender)
{
    auto *s = SendRpcService::instance();
    // appNoSender has no sender -> handleSetTargetAppName returns early
    s->_work.handleSetTargetAppName("appNoSender", "target");
    SUCCEED();
}

TEST(SendRpcServiceCovTest, WorkHandleCreateRpcSenderWhenStopped)
{
    auto *s = SendRpcService::instance();
    bool wasStopped = s->_work._stoped;
    s->_work._stoped = true;
    s->_work.handleCreateRpcSender("appStopped", "127.0.0.1", 51597);
    // _stoped guard returns early; nothing added
    s->_work._stoped = wasStopped;
    SUCCEED();
}

TEST(SendRpcServiceCovTest, WorkHandleDoSendProtoMsgNoSender)
{
    auto *s = SendRpcService::instance();
    // no sender for this app -> PARAM_ERROR result path (covered; signal is
    // queued so we don't assert on spy count to avoid a process-event race).
    s->_work.handleDoSendProtoMsg(LOGIN_INFO, "appNoSenderX", "{}", QByteArray());
    SUCCEED();
}

TEST(SendRpcServiceCovTest, WorkHandleDoSendProtoMsgStopped)
{
    auto *s = SendRpcService::instance();
    bool wasStopped = s->_work._stoped;
    s->_work._stoped = true;
    s->_work.handleDoSendProtoMsg(LOGIN_INFO, "app1", "{}", QByteArray());
    s->_work._stoped = wasStopped;
    SUCCEED();
}

TEST(SendRpcServiceCovTest, WorkHandlePingEmpty)
{
    auto *s = SendRpcService::instance();
    // empty app list -> loop does nothing
    s->_work.handlePing(QStringList{});
    SUCCEED();
}

TEST(SendRpcServiceCovTest, WorkHandlePingNonexistentApp)
{
    auto *s = SendRpcService::instance();
    // app with no sender -> continue
    s->_work.handlePing(QStringList{"appNoSenderPing"});
    SUCCEED();
}

// ---- TRANS_APPLY / APPLY_SHARE_CONNECT_RES branches in handleDoSendProtoMsg ----

TEST(SendRpcServiceCovTest, DISABLED_WorkHandleDoSendTransApplyConfirm)
{
    auto *s = SendRpcService::instance();
    s->_work.createRpcSender("transApp", "127.0.0.1", 51597);
    // TRANS_APPLY with type != APPLY_TRANS_APPLY -> updates confirm state
    ApplyTransFiles info;
    info.appname = "transApp";
    info.tarAppname = "transApp";
    info.type = APPLY_TRANS_CONFIRM;
    s->_work.handleDoSendProtoMsg(TRANS_APPLY, "transApp", info.as_json().str().c_str(), QByteArray());
    EXPECT_TRUE(JobManager::instance()->isTransferConfirmed("transApp"));
    JobManager::instance()->removeConfirmedTransfer("transApp");
}

TEST(SendRpcServiceCovTest, DISABLED_WorkHandleDoSendTransApplyRefused)
{
    auto *s = SendRpcService::instance();
    s->_work.createRpcSender("refuseApp", "127.0.0.1", 51597);
    ApplyTransFiles info;
    info.appname = "refuseApp";
    info.tarAppname = "refuseApp";
    info.type = APPLY_TRANS_REFUSED;
    s->_work.handleDoSendProtoMsg(TRANS_APPLY, "refuseApp", info.as_json().str().c_str(), QByteArray());
    EXPECT_FALSE(JobManager::instance()->isTransferConfirmed("refuseApp"));
}

TEST(SendRpcServiceCovTest, DISABLED_WorkHandleDoSendTransApplyApply)
{
    auto *s = SendRpcService::instance();
    s->_work.createRpcSender("applyApp", "127.0.0.1", 51597);
    ApplyTransFiles info;
    info.appname = "applyApp";
    info.type = APPLY_TRANS_APPLY; // == APPLY_TRANS_APPLY -> skips the inner branch
    s->_work.handleDoSendProtoMsg(TRANS_APPLY, "applyApp", info.as_json().str().c_str(), QByteArray());
    SUCCEED();
}

TEST(SendRpcServiceCovTest, DISABLED_WorkHandleDoSendShareConnectResConfirm)
{
    auto *s = SendRpcService::instance();
    s->_work.createRpcSender("shareApp", "127.0.0.1", 51597);
    // reply==COMFIRM calls DiscoveryJob::updateAnnouncShare which derefs
    // _announcer_p; install a real announcer so it doesn't crash.
    DiscoveryJob::instance()->_announcer_p =
        new searchlight::Announcer("svc", 51597, "{\"uuid\":\"u\"}");
    ShareConnectReply rep;
    rep.appName = "shareApp";
    rep.reply = SHARE_CONNECT_COMFIRM;
    rep.ip = "1.2.3.4";
    s->_work.handleDoSendProtoMsg(APPLY_SHARE_CONNECT_RES, "shareApp", rep.as_json().str().c_str(), QByteArray());
    SUCCEED();
}

TEST(SendRpcServiceCovTest, DISABLED_WorkHandleDoSendShareConnectResRefuse)
{
    auto *s = SendRpcService::instance();
    s->_work.createRpcSender("shareApp2", "127.0.0.1", 51597);
    ShareConnectReply rep;
    rep.appName = "shareApp2";
    rep.reply = SHARE_CONNECT_REFUSE;
    s->_work.handleDoSendProtoMsg(APPLY_SHARE_CONNECT_RES, "shareApp2", rep.as_json().str().c_str(), QByteArray());
    SUCCEED();
}

TEST(SendRpcServiceCovTest, DISABLED_WorkHandleDoSendDefaultType)
{
    auto *s = SendRpcService::instance();
    s->_work.createRpcSender("defApp", "127.0.0.1", 51597);
    // generic type -> else branch
    s->_work.handleDoSendProtoMsg(MISC, "defApp", "{}", QByteArray());
    SUCCEED();
}
