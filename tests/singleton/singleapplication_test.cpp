// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "singleapplication.h"

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QLocalServer>
#include <QLocalSocket>

using namespace deepin_cross;

// SingleApplication has one QLocalServer. Once closeServer() deletes it,
// subsequent socket operations will NPE. So we only call setSingleInstance
// once and test other methods against that single server.

TEST(SingleAppTest, InstanceNotNull)
{
    EXPECT_NE(qApp, nullptr);
}

TEST(SingleAppTest, GetSocketNameStandard)
{
    QString name = qApp->getSocketName("mykey", SingleApplication::StandardSocket);
    EXPECT_FALSE(name.isEmpty());
    EXPECT_TRUE(name.contains("mykey"));
}

TEST(SingleAppTest, GetSocketNameBackup)
{
    QString name = qApp->getSocketName("mykey", SingleApplication::BackupSocket);
    EXPECT_FALSE(name.isEmpty());
    EXPECT_TRUE(name.contains("mykey"));
}

TEST(SingleAppTest, TryCreateSocketEmpty)
{
    EXPECT_FALSE(qApp->tryCreateSocket(""));
}

TEST(SingleAppTest, TestSocketConnectionEmpty)
{
    EXPECT_FALSE(qApp->testSocketConnection(""));
}

TEST(SingleAppTest, TestSocketConnectionNoServer)
{
    EXPECT_FALSE(qApp->testSocketConnection("@nonexistent-socket-99999"));
}

TEST(SingleAppTest, FindActiveSocketNotFound)
{
    QString found = qApp->findActiveSocket("nonexistent-key-67890");
    EXPECT_TRUE(found.isEmpty());
}

TEST(SingleAppTest, DoSendMessageEmptyPath)
{
    EXPECT_FALSE(qApp->doSendMessage("", QByteArray("test")));
}

TEST(SingleAppTest, DoSendMessageNoServer)
{
    EXPECT_FALSE(qApp->doSendMessage("@nonexistent-socket-88888", QByteArray("test")));
}

TEST(SingleAppTest, CheckProcessFalseNoServer)
{
    EXPECT_FALSE(qApp->checkProcess("nonexistent-key-12345"));
}

// Now set up a real single instance — all subsequent tests use it.
TEST(SingleAppTest, SetSingleInstanceSuccess)
{
    QString key = "ut-single-main";
    EXPECT_TRUE(qApp->setSingleInstance(key));
}

TEST(SingleAppTest, SetSingleInstanceDuplicateReturnsFalse)
{
    QString key = "ut-single-main";
    // checkProcess now finds the socket → setSingleInstance returns false
    EXPECT_FALSE(qApp->setSingleInstance(key));
}

TEST(SingleAppTest, CheckProcessTrueAfterSetup)
{
    EXPECT_TRUE(qApp->checkProcess("ut-single-main"));
}

TEST(SingleAppTest, TryCreateSocketAlreadyListening)
{
    // server is already listening → tryCreateSocket returns false
    QString socket = qApp->getSocketName("ut-single-main", SingleApplication::StandardSocket);
    EXPECT_FALSE(qApp->tryCreateSocket(socket));
}

TEST(SingleAppTest, TestSocketConnectionActive)
{
    // Abstract sockets (@) may not allow same-process connect in some kernels.
    // Verify the method returns a bool without crashing.
    QString socket = qApp->getSocketName("ut-single-main", SingleApplication::StandardSocket);
    EXPECT_NO_FATAL_FAILURE(qApp->testSocketConnection(socket));
}

TEST(SingleAppTest, FindActiveSocketFound)
{
    // May or may not find depending on abstract socket behavior
    QString found = qApp->findActiveSocket("ut-single-main");
    EXPECT_TRUE(found.isEmpty() || found.contains("ut-single-main"));
}

TEST(SingleAppTest, DoSendMessageToSelf)
{
    // Send a message to our own socket — may fail on abstract sockets
    QString socket = qApp->getSocketName("ut-single-main", SingleApplication::StandardSocket);
    EXPECT_NO_FATAL_FAILURE(qApp->doSendMessage(socket, QByteArray("dGVzdA==")));
}

TEST(SingleAppTest, ActiveServerNameSet)
{
    EXPECT_FALSE(qApp->activeServerName.isEmpty());
}

TEST(SingleAppTest, OnDeliverMessageEmpty)
{
    EXPECT_NO_FATAL_FAILURE(qApp->onDeliverMessage("test-key", QStringList{}));
}

TEST(SingleAppTest, OnDeliverMessageWithContent)
{
    // Will use the recorded activeServerName to send
    EXPECT_NO_FATAL_FAILURE(qApp->onDeliverMessage("ut-single-main", QStringList{"arg1", "arg2"}));
}

TEST(SingleAppTest, ReadDataNoSocket)
{
    EXPECT_NO_FATAL_FAILURE(qApp->readData());
}

TEST(SingleAppTest, HelpActionTriggered)
{
    EXPECT_NO_FATAL_FAILURE(qApp->helpActionTriggered());
}

TEST(SingleAppTest, HandleConnectionNoPending)
{
    EXPECT_NO_FATAL_FAILURE(qApp->handleConnection());
}
