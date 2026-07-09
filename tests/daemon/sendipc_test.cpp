// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QSignalSpy>

#include "service/ipc/sendipcservice.h"
#include "ipc/session.h"
#include "common/constant.h"

// SendIpcService::instance() is a real singleton. The constructor starts an
// internal _ping QTimer (1000 ms); that is harmless in the QCoreApplication
// test main. Private members are accessed directly thanks to
// -fno-access-control.

// Reset shared singleton state before every case so the tests are independent.
class SendIpcTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        svc = SendIpcService::instance();
        // handleAboutToQuit clears _sessions, _offline_status and stops timers.
        svc->handleAboutToQuit();
    }

    SendIpcService *svc = nullptr;
};

// 1. InstanceSingleton
TEST_F(SendIpcTest, InstanceSingleton)
{
    auto *a = SendIpcService::instance();
    auto *b = SendIpcService::instance();
    EXPECT_EQ(a, b);
    EXPECT_NE(a, nullptr);
}

// 2. HandleSaveSessionAndQuery
TEST_F(SendIpcTest, HandleSaveSessionAndQuery)
{
    svc->handleSaveSession("app1", "ses1", "sig1");
    auto s = svc->_sessions.value("app1");
    EXPECT_FALSE(s.isNull());
    EXPECT_EQ(s->getName(), QString("app1"));
    EXPECT_EQ(s->getSession(), QString("ses1"));
    EXPECT_EQ(s->signal(), QString("sig1"));
}

// 3. HandleRemoveSessionByAppName
TEST_F(SendIpcTest, HandleRemoveSessionByAppName)
{
    svc->handleSaveSession("app1", "ses1", "sig1");
    ASSERT_FALSE(svc->_sessions.value("app1").isNull());

    svc->handleRemoveSessionByAppName("app1");
    EXPECT_TRUE(svc->_sessions.value("app1").isNull());
    EXPECT_TRUE(svc->_sessions.isEmpty());
}

// 4. HandleRemoveSessionBySessionID
TEST_F(SendIpcTest, HandleRemoveSessionBySessionID)
{
    svc->handleSaveSession("app1", "ses1", "sig1");
    ASSERT_FALSE(svc->_sessions.value("app1").isNull());

    svc->handleRemoveSessionBySessionID("ses1");
    EXPECT_TRUE(svc->_sessions.value("app1").isNull());
    EXPECT_TRUE(svc->_sessions.isEmpty());
}

// 4b. Removing a non-matching session id leaves the session intact.
TEST_F(SendIpcTest, HandleRemoveSessionBySessionIDNoMatch)
{
    svc->handleSaveSession("app1", "ses1", "sig1");
    svc->handleRemoveSessionBySessionID("other");
    EXPECT_FALSE(svc->_sessions.value("app1").isNull());
}

// 5. HandleSendToClientNoSession — no crash, logs error.
TEST_F(SendIpcTest, HandleSendToClientNoSession)
{
    QSignalSpy spy(svc, &SendIpcService::sessionSignal);
    svc->handleSendToClient("nope", 1, "msg");
    EXPECT_EQ(spy.count(), 0);
}

// 6. HandleSendToClientWithSession
TEST_F(SendIpcTest, HandleSendToClientWithSession)
{
    svc->handleSaveSession("app1", "ses1", "sig1");
    QSignalSpy spy(svc, &SendIpcService::sessionSignal);
    svc->handleSendToClient("app1", 100, "hello");
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString(), QString("sig1"));
}

// 6b. The "dde-cooperation" fallback path in handleSendToClient.
TEST_F(SendIpcTest, HandleSendToClientDaemonFallback)
{
    svc->handleSaveSession("dde-cooperation", "ses1", "sigD");
    QSignalSpy spy(svc, &SendIpcService::sessionSignal);
    // appName == "daemon-cooperation" triggers the fallback lookup.
    svc->handleSendToClient("daemon-cooperation", 1, "msg");
    EXPECT_EQ(spy.count(), 1);
}

// 6c. An empty signal name means no emission.
TEST_F(SendIpcTest, HandleSendToClientEmptySignal)
{
    svc->handleSaveSession("app1", "ses1", "");
    QSignalSpy spy(svc, &SendIpcService::sessionSignal);
    svc->handleSendToClient("app1", 1, "msg");
    EXPECT_EQ(spy.count(), 0);
}

// 7. HandleSendToAllClient
TEST_F(SendIpcTest, HandleSendToAllClient)
{
    svc->handleSaveSession("app1", "s1", "sig1");
    svc->handleSaveSession("app2", "s2", "sig2");
    QSignalSpy spy(svc, &SendIpcService::sessionSignal);
    svc->handleSendToAllClient(1, "msg");
    EXPECT_GE(spy.count(), 2);
}

// 8. HandleAddJobNoSession — no crash.
TEST_F(SendIpcTest, HandleAddJobNoSession)
{
    svc->handleAddJob("nope", 1);
    SUCCEED();
}

// 9. HandleAddJobWithSession
TEST_F(SendIpcTest, HandleAddJobWithSession)
{
    svc->handleSaveSession("app1", "ses1", "sig1");
    svc->handleAddJob("app1", 5);
    auto s = svc->_sessions.value("app1");
    ASSERT_FALSE(s.isNull());
    EXPECT_GE(s->hasJob(5), 0);
}

// 10. HandleRemoveJob
TEST_F(SendIpcTest, HandleRemoveJob)
{
    svc->handleSaveSession("app1", "ses1", "sig1");
    svc->handleAddJob("app1", 5);
    ASSERT_GE(svc->_sessions.value("app1")->hasJob(5), 0);

    svc->handleRemoveJob("app1", 5);
    EXPECT_EQ(svc->_sessions.value("app1")->hasJob(5), -1);
}

// 10b. Removing a job on a non-existent session is a no-op.
TEST_F(SendIpcTest, HandleRemoveJobNoSession)
{
    svc->handleRemoveJob("nope", 5);
    SUCCEED();
}

// 11. PreprocessOfflineStatus
TEST_F(SendIpcTest, PreprocessOfflineStatus)
{
    svc->preprocessOfflineStatus("app1", PING_FAILED, fastring("msg"));
    EXPECT_TRUE(svc->_offline_status.contains("app1"));
    EXPECT_EQ(svc->_offline_status.value("app1").type, PING_FAILED);
    EXPECT_EQ(svc->_offline_status.value("app1").status, REMOTE_CLIENT_OFFLINE);
}

// 12. CancelOfflineStatus
TEST_F(SendIpcTest, CancelOfflineStatus)
{
    svc->preprocessOfflineStatus("app1", PING_FAILED, fastring("msg"));
    ASSERT_TRUE(svc->_offline_status.contains("app1"));

    svc->cancelOfflineStatus("app1");
    EXPECT_FALSE(svc->_offline_status.contains("app1"));
    EXPECT_TRUE(svc->_offline_status.isEmpty());
}

// 13. CancelOfflineStatusAll
TEST_F(SendIpcTest, CancelOfflineStatusAll)
{
    svc->preprocessOfflineStatus("app1", PING_FAILED, fastring("m1"));
    svc->preprocessOfflineStatus("app2", PING_FAILED, fastring("m2"));
    ASSERT_EQ(svc->_offline_status.size(), 2);

    svc->cancelOfflineStatus("all");
    // "all" is not a real key, but the compare("all")==0 branch stops the timer.
    // Remaining entries stay unless individually removed; just verify no crash.
    SUCCEED();
}

// 14. HandleAboutToQuit clears _sessions.
TEST_F(SendIpcTest, HandleAboutToQuit)
{
    svc->handleSaveSession("app1", "ses1", "sig1");
    svc->preprocessOfflineStatus("app1", PING_FAILED, fastring("msg"));
    ASSERT_FALSE(svc->_sessions.isEmpty());
    ASSERT_FALSE(svc->_offline_status.isEmpty());

    svc->handleAboutToQuit();
    EXPECT_TRUE(svc->_sessions.isEmpty());
    EXPECT_TRUE(svc->_offline_status.isEmpty());
}

// 15. HandleStartOfflineTimer / HandleStopOfflineTimer
TEST_F(SendIpcTest, HandleStartAndStopOfflineTimer)
{
    svc->handleStartOfflineTimer();
    svc->handleStopOfflineTimer();
    SUCCEED();
}

// 16. HandleNodeChangedNoSessions — no crash.
TEST_F(SendIpcTest, HandleNodeChangedNoSessions)
{
    svc->handleNodeChanged(true, "{\"name\":\"x\"}");
    SUCCEED();
}

// 16b. HandleNodeChanged with an alive session exercises the send branch.
TEST_F(SendIpcTest, HandleNodeChangedWithSession)
{
    svc->handleSaveSession("dde-cooperation", "ses1", "sig1");
    QSignalSpy spy(svc, &SendIpcService::sessionSignal);
    svc->handleNodeChanged(true, "{\"name\":\"x\"}");
    EXPECT_GE(spy.count(), 1);
}
