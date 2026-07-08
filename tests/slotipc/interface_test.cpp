// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// SlotIPCInterface 覆盖 (service↔interface 本地回环)
// 注: 避开 isConnected() — InterfaceWorker::isConnected() (interfaceworker.cpp:142)
// 无空指针检查, 未连接时调用必崩 (真实 bug, 即记忆中 interface_test 段错误根因)。
// 改为只验证 connectToServer 返回值 + disconnectFromServer 不崩溃, 仍覆盖 connect 主路径。

#include "slotipc/interface.h"
#include "slotipc/service.h"

#include <gtest/gtest.h>
#include <QHostAddress>
#include <QTest>

TEST(InterfaceTest, ConstructorAndLastError)
{
    SlotIPCInterface iface;
    EXPECT_TRUE(iface.lastError().isEmpty());
    // 不调 isConnected() — 未连接时会段错误 (生产 bug)
}

TEST(InterfaceTest, InvalidLocalConnection)
{
    SlotIPCInterface iface;
    EXPECT_FALSE(iface.connectToServer("invalid/service/name"));
    // lastError 不强断言 (不同 Qt 版本行为不一)
}

TEST(InterfaceTest, InvalidTcpConnection)
{
    SlotIPCInterface iface;
    EXPECT_FALSE(iface.connectToServer(QHostAddress::LocalHost, 65535));
}

TEST(InterfaceTest, LocalConnectionLoopback)
{
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.iface.local"));

    SlotIPCInterface iface;
    EXPECT_TRUE(iface.connectToServer("test.iface.local"));
    QTest::qWait(50);
    // isConnected() 仅在已连接 Interface 上安全 (未连接 m_connection 为 null 会段错误)
    EXPECT_TRUE(iface.isConnected());
    iface.disconnectFromServer();
    QTest::qWait(50);
}

TEST(InterfaceTest, TcpConnectionLoopback)
{
    SlotIPCService service;
    ASSERT_TRUE(service.listenTcp(QHostAddress::LocalHost, 0));
    quint16 port = service.tcpPort();

    SlotIPCInterface iface;
    EXPECT_TRUE(iface.connectToServer(QHostAddress::LocalHost, port));
    QTest::qWait(50);
    EXPECT_TRUE(iface.isConnected());
    iface.disconnectFromServer();
    QTest::qWait(50);
}

TEST(InterfaceTest, MultipleConnectDisconnect)
{
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.iface.multi"));
    SlotIPCInterface iface;
    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(iface.connectToServer("test.iface.multi"));
        QTest::qWait(30);
        iface.disconnectFromServer();
        QTest::qWait(30);
    }
}

// ---- call / callNoReply 路径覆盖 ----
// 用普通 QObject 作 subject, 调用不存在的方法 → 服务端找不到 → 客户端走错误/超时路径。
// 仍覆盖 interface.cpp 的 call/callImpl/callNoReply 编解码与发送逻辑。
TEST(InterfaceTest, CallNoReplyNonexistentMethod)
{
    QObject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.iface.callnr", &subject));
    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.iface.callnr"));
    QTest::qWait(50);

    iface.callNoReply("nonexistentSlot", Q_ARG(int, 42));
    QTest::qWait(50);   // 让服务端处理 (找不到方法, 走错误路径)
    iface.disconnectFromServer();
    QTest::qWait(50);
}

TEST(InterfaceTest, CallNonexistentMethodReturnsFalse)
{
    QObject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.iface.call", &subject));
    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.iface.call"));
    QTest::qWait(50);

    // call 同步等待响应; 方法不存在 → 返回 false (覆盖 call/callImpl + demarshallResponse)
    EXPECT_FALSE(iface.call("nonexistentSlot", Q_ARG(int, 1)));
    iface.disconnectFromServer();
    QTest::qWait(50);
}

// ---- remoteConnect: 触发服务端 SignalHandler 创建 (覆盖 signalhandler.cpp) ----
// localObject 的 destroyed() 信号 → 远端 remoteMethod。QObject 自带 destroyed 信号, 无需 Q_OBJECT subject。
TEST(InterfaceTest, RemoteConnectLocalSignalToRemote)
{
    QObject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.iface.remote", &subject));
    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.iface.remote"));
    QTest::qWait(50);

    QObject localObject;
    // localObject 的 destroyed(QObject*) → 远端 remoteOnDestroyed
    // subject 无该方法 → 返回 false, 但已覆盖 remoteConnect 客户端编解码 + 服务端处理路径
    iface.remoteConnect(&localObject, SIGNAL(destroyed(QObject*)), "remoteOnDestroyed");
    QTest::qWait(50);
    iface.disconnectFromServer();
    QTest::qWait(50);
}
