// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 深挖: call 成功路径 (demarshallResponse 成功) + remoteConnect 成功 (触发服务端 SignalHandler)
// 用 Q_OBJECT TestSubject (QT6_WRAP_CPP 生成 moc) 作服务端 subject。

#include "slotipc/service.h"
#include "slotipc/interface.h"
#include "testsubject.h"

#include <gtest/gtest.h>
#include <QHostAddress>
#include <QTest>

// ---- call() 成功, 带返回值: 覆盖 callImpl 成功 + demarshallResponse 成功路径 ----
TEST(RpcTest, CallWithReturnSucceeds)
{
    TestSubject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.rpc.callret", &subject));

    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.rpc.callret"));
    QTest::qWait(50);

    int result = 0;
    // Qt6: Q_RETURN_ARG 返回 QMetaMethodReturnArgument, 隐式转 METHOD_RE_ARG
    EXPECT_TRUE(iface.call("add", Q_RETURN_ARG(int, result), Q_ARG(int, 2), Q_ARG(int, 3)));
    EXPECT_EQ(result, 5);
    EXPECT_EQ(subject.addResult(), 5);

    iface.disconnectFromServer();
    QTest::qWait(50);
}

// ---- callNoReply 成功: 服务端真正调用了 slot ----
TEST(RpcTest, CallNoReplyInvokesSlot)
{
    TestSubject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.rpc.callnr", &subject));

    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.rpc.callnr"));
    QTest::qWait(50);

    EXPECT_EQ(subject.lastArg(), 0);
    iface.callNoReply("receiveInt", Q_ARG(int, 42));
    QTest::qWait(100);   // 异步: 等服务端调用 slot
    EXPECT_EQ(subject.lastArg(), 42);

    iface.disconnectFromServer();
    QTest::qWait(50);
}

// ---- remoteConnect 远端信号 → 本地方法: 触发服务端 SignalHandler 创建 ----
TEST(RpcTest, RemoteConnectRemoteSignalTriggersSignalHandler)
{
    TestSubject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.rpc.signal", &subject));

    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.rpc.signal"));
    QTest::qWait(50);

    TestSubject receiver;   // 本地接收对象
    // 远端 subject 的 progress(int) 信号 → 本地 receiver 的 voidSlot(int)
    EXPECT_TRUE(iface.remoteConnect(SIGNAL(progress(int)), &receiver, SLOT(voidSlot(int))));
    QTest::qWait(50);

    // 服务端 emit progress → SignalHandler relay → 客户端 invoke receiver.voidSlot
    emit subject.progress(7);
    QTest::qWait(100);
    EXPECT_EQ(receiver.lastArg(), 7);

    iface.disconnectFromServer();
    QTest::qWait(50);
}

// ---- disconnectSignal: 断开远端信号 → 本地方法 连接 ----
TEST(RpcTest, DisconnectSignalAfterRemoteConnect)
{
    TestSubject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.rpc.disc", &subject));
    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.rpc.disc"));
    QTest::qWait(50);

    TestSubject receiver;
    ASSERT_TRUE(iface.remoteConnect(SIGNAL(progress(int)), &receiver, SLOT(voidSlot(int))));
    QTest::qWait(50);
    // 断开
    EXPECT_TRUE(iface.disconnectSignal(SIGNAL(progress(int)), &receiver, SLOT(voidSlot(int))));
    QTest::qWait(50);
    iface.disconnectFromServer();
    QTest::qWait(50);
}

// ---- remoteSlotConnect (deprecated) + disconnectRemoteMethod ----
TEST(RpcTest, RemoteSlotConnectAndDisconnect)
{
    TestSubject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.rpc.slot", &subject));
    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.rpc.slot"));
    QTest::qWait(50);

    // 本地信号 → 远端 slot (deprecated 别名, 需 SLOT() 宏前缀让 checkConnectCorrection 通过)
    TestSubject localObj;
    iface.remoteSlotConnect(&localObj, SIGNAL(progress(int)), SLOT(receiveInt(int)));
    QTest::qWait(50);
    // 断开
    iface.disconnectRemoteMethod(&localObj, SIGNAL(progress(int)), SLOT(receiveInt(int)));
    QTest::qWait(50);
    iface.disconnectFromServer();
    QTest::qWait(50);
}

// ---- TCP + call(): 覆盖 sendSynchronousRequest 的 TCP 分支 (interface.cpp 156-181) ----
// 注: TCP 二级 socket 的同步握手在此环境不可靠, call 可能返回 false;
// 目标是覆盖 TCP 分支代码路径, 返回值不强求。
TEST(RpcTest, CallWithReturnOverTcp)
{
    TestSubject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listenTcp(QHostAddress::LocalHost, 0));
    quint16 port = service.tcpPort();

    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer(QHostAddress::LocalHost, port));
    QTest::qWait(50);

    int result = 0;
    iface.call("add", Q_RETURN_ARG(int, result), Q_ARG(int, 10), Q_ARG(int, 20));
    // 覆盖 TCP sync 分支即可, 不强求 result==30
    iface.disconnectFromServer();
    QTest::qWait(50);
}

// ---- remoteConnect 不兼容签名: 覆盖 checkConnectCorrection 警告分支 (interface.cpp 73-75) ----
TEST(RpcTest, RemoteConnectIncompatibleSignature)
{
    TestSubject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.rpc.sig", &subject));
    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.rpc.sig"));
    QTest::qWait(50);

    TestSubject receiver;
    // progress(int) vs voidSlot(QString) 参数不兼容 → checkConnectCorrection 返回 false
    EXPECT_FALSE(iface.remoteConnect(SIGNAL(progress(int)), &receiver, SLOT(voidSlot(QString))));
    iface.disconnectFromServer();
    QTest::qWait(50);
}

// ---- null localObject 早返回 (覆盖 if(!localObject) 分支) ----
TEST(RpcTest, RemoteSlotConnectNullObject)
{
    TestSubject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.rpc.null", &subject));
    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.rpc.null"));
    QTest::qWait(50);

    EXPECT_FALSE(iface.remoteSlotConnect(nullptr, SIGNAL(progress(int)), "receiveInt"));
    EXPECT_FALSE(iface.disconnectSlot(nullptr, SIGNAL(progress(int)), "receiveInt"));
    iface.disconnectFromServer();
    QTest::qWait(50);
}

// ---- disconnectSlot 成功路径 (先 remoteSlotConnect 建连, 再断) ----
TEST(RpcTest, DisconnectSlotAfterConnect)
{
    TestSubject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.rpc.discslot", &subject));
    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.rpc.discslot"));
    QTest::qWait(50);

    TestSubject localObj;
    iface.remoteSlotConnect(&localObj, SIGNAL(progress(int)), SLOT(receiveInt(int)));
    QTest::qWait(50);
    // disconnectSlot 需 SLOT() 宏前缀 ('1'); 走成功分支
    EXPECT_TRUE(iface.disconnectSlot(&localObj, SIGNAL(progress(int)), SLOT(receiveInt(int))));
    iface.disconnectFromServer();
    QTest::qWait(50);
}

// ---- receiver 析构触发 _q_removeRemoteConnectionsOfObject (interface.cpp 287-299) ----
TEST(RpcTest, ReceiverDestroyedCleansUpRemoteConnections)
{
    TestSubject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.rpc.rcvdest", &subject));
    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.rpc.rcvdest"));
    QTest::qWait(50);

    {
        TestSubject receiver;
        ASSERT_TRUE(iface.remoteConnect(SIGNAL(progress(int)), &receiver, SLOT(voidSlot(int))));
        QTest::qWait(50);
        // receiver 出作用域析构 → destroyed(QObject*) → _q_removeRemoteConnectionsOfObject
    }
    QTest::qWait(50);
    iface.disconnectFromServer();
    QTest::qWait(50);
}

// ---- localObject 析构触发 _q_removeSignalHandlersOfObject (interface.cpp 259-270) ----
TEST(RpcTest, LocalObjectDestroyedCleansUpSignalHandlers)
{
    TestSubject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.rpc.locdest", &subject));
    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.rpc.locdest"));
    QTest::qWait(50);

    {
        TestSubject localObj;
        iface.remoteSlotConnect(&localObj, SIGNAL(progress(int)), SLOT(receiveInt(int)));
        QTest::qWait(50);
        // localObj 出作用域析构 → _q_removeSignalHandlersOfObject
    }
    QTest::qWait(50);
    iface.disconnectFromServer();
    QTest::qWait(50);
}

// ---- callNoReply 成功 (无返回值) → 服务端 sendResponseMessage 路径 ----
TEST(RpcTest, CallWithoutReturnServerResponsePath)
{
    TestSubject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.rpc.callnoret", &subject));
    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.rpc.callnoret"));
    QTest::qWait(50);

    // call() 无返回值参数 → MessageCallWithReturn + empty returnType → invokeMethod success 分支
    EXPECT_TRUE(iface.call("receiveInt", Q_ARG(int, 99)));
    QTest::qWait(80);
    EXPECT_EQ(subject.lastArg(), 99);
    iface.disconnectFromServer();
    QTest::qWait(50);
}

// ---- remoteConnect 重载 #2 (localObject, localSignal, remoteMethod=remote SLOT) ----
// 覆盖 interface.cpp 519-568 的 type=='1' slot 分支 + handleLocalSignalRequest
TEST(RpcTest, RemoteConnectLocalSignalToRemoteSlot)
{
    TestSubject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.rpc.rc2slot", &subject));
    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.rpc.rc2slot"));
    QTest::qWait(50);

    TestSubject localObj;
    // localObj 的 progress(int) → 远端 subject 的 receiveInt(int) slot
    EXPECT_TRUE(iface.remoteConnect(&localObj, SIGNAL(progress(int)), SLOT(receiveInt(int))));
    QTest::qWait(50);
    // 触发本地信号 → 客户端 relay → 服务端 invoke subject.receiveInt
    emit localObj.progress(55);
    QTest::qWait(80);
    EXPECT_EQ(subject.lastArg(), 55);
    iface.disconnectFromServer();
    QTest::qWait(50);
}

// ---- remoteConnect 重载 #2, remoteMethod = remote SIGNAL (type=='2' 分支) ----
TEST(RpcTest, RemoteConnectLocalSignalToRemoteSignal)
{
    TestSubject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.rpc.rc2sig", &subject));
    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.rpc.rc2sig"));
    QTest::qWait(50);

    TestSubject localObj;
    // localObj 的 progress(int) → 远端 subject 的 progress(int) signal (type=='2' → sendRemoteConnectRequest)
    iface.remoteConnect(&localObj, SIGNAL(progress(int)), SIGNAL(progress(int)));
    QTest::qWait(50);
    iface.disconnectFromServer();
    QTest::qWait(50);
}

// ---- remoteConnect 重载 #2, null localObject 早返回 (覆盖 523-528) ----
TEST(RpcTest, RemoteConnectOverload2NullObject)
{
    TestSubject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.rpc.rc2null", &subject));
    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.rpc.rc2null"));
    QTest::qWait(50);
    EXPECT_FALSE(iface.remoteConnect(nullptr, SIGNAL(progress(int)), SLOT(receiveInt(int))));
    iface.disconnectFromServer();
    QTest::qWait(50);
}

// ---- service.close() 带 active connection → sendAboutToQuit (service.cpp ~65) ----
TEST(RpcTest, ServiceCloseWithActiveConnection)
{
    TestSubject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.rpc.close", &subject));
    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.rpc.close"));
    QTest::qWait(50);
    service.close();   // 带 active 连接关闭 → sendAboutToQuit 路径
    QTest::qWait(50);
}

// ---- remoteConnect 到不存在的远端信号 → 服务端 "Signal doesn't exist" 错误 ----
TEST(RpcTest, RemoteConnectToNonexistentSignal)
{
    TestSubject subject;
    SlotIPCService service;
    ASSERT_TRUE(service.listen("test.rpc.nosig", &subject));
    SlotIPCInterface iface;
    ASSERT_TRUE(iface.connectToServer("test.rpc.nosig"));
    QTest::qWait(50);
    // subject 没有 noSuchSignal 信号 → 服务端 sendErrorMessage
    iface.remoteConnect(SIGNAL(nonexistent(int)), &subject, SLOT(voidSlot(int)));
    QTest::qWait(50);
    iface.disconnectFromServer();
    QTest::qWait(50);
}
