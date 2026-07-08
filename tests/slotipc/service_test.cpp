// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// SlotIPCService 覆盖 (QLocalServer/QTcpServer 本地 socket, 安全)
// listen/listenTcp/close/serverName/tcpPort/tcpAddress + 无效入参 + 端口冲突 + 重复关闭。

#include "slotipc/service.h"

#include <gtest/gtest.h>
#include <QHostAddress>

TEST(ServiceTest, Constructor)
{
    SlotIPCService service;
    EXPECT_TRUE(service.serverName().isEmpty());
    // tcpPort 未 listen 时为未初始化哨兵值 (实测 65535), 不强断言具体值
}

TEST(ServiceTest, LocalServerListenAndClose)
{
    SlotIPCService service;
    EXPECT_TRUE(service.listen("test.service.slotipc"));
    EXPECT_EQ(service.serverName().toStdString(), "test.service.slotipc");
    service.close();
    EXPECT_TRUE(service.serverName().isEmpty());
}

TEST(ServiceTest, InvalidLocalServerName)
{
    SlotIPCService service;
    // 含路径分隔符的 socket 名在 Linux 上非法
    EXPECT_FALSE(service.listen("invalid/service/name"));
}

TEST(ServiceTest, TcpServerAutoPort)
{
    SlotIPCService service;
    EXPECT_TRUE(service.listenTcp(QHostAddress::LocalHost, 0));
    EXPECT_NE(service.tcpPort(), 0);
    EXPECT_EQ(service.tcpAddress(), QHostAddress::LocalHost);
    service.close();
    EXPECT_EQ(service.tcpPort(), 0);
}

TEST(ServiceTest, TcpServerFixedPort)
{
    SlotIPCService service;
    ASSERT_TRUE(service.listenTcp(QHostAddress::LocalHost, 0));
    quint16 port = service.tcpPort();
    service.close();
    EXPECT_TRUE(service.listenTcp(QHostAddress::LocalHost, port));
    EXPECT_EQ(service.tcpPort(), port);
}

TEST(ServiceTest, TcpPortConflict)
{
    SlotIPCService s1;
    ASSERT_TRUE(s1.listenTcp(QHostAddress::LocalHost, 0));
    quint16 port = s1.tcpPort();
    SlotIPCService s2;
    EXPECT_FALSE(s2.listenTcp(QHostAddress::LocalHost, port));
}

TEST(ServiceTest, MultipleCloseSafe)
{
    SlotIPCService service;
    service.close();
    service.close();
    EXPECT_TRUE(service.listen("test.service.mc"));
    service.close();
    service.close();
}

TEST(ServiceTest, ListenWithSubjectOverload)
{
    SlotIPCService service;
    QObject subject;
    EXPECT_TRUE(service.listen(&subject));
    EXPECT_FALSE(service.serverName().isEmpty());
    service.close();
}

TEST(ServiceTest, ListenTcpWithSubject)
{
    SlotIPCService service;
    QObject subject;
    EXPECT_TRUE(service.listenTcp(&subject));
    EXPECT_NE(service.tcpPort(), 0);
    service.close();
}

// tcpPort/tcpAddress 未监听时的警告分支 (service.cpp 275-276, 293-294)
TEST(ServiceTest, TcpPortAddressWithoutListening)
{
    SlotIPCService service;
    // 未 listenTcp → 走警告分支 (返回值不强求, quint16(-1)=65535)
    service.tcpPort();
    EXPECT_EQ(service.tcpAddress(), QHostAddress::Null);
}
