#include <gtest/gtest.h>
#include "slotipc/service.h"

class ServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        service = new SlotIPCService;
    }

    void TearDown() override {
        delete service;
    }

    SlotIPCService* service;
};

TEST_F(ServiceTest, Constructor) {
    EXPECT_NE(service, nullptr);
    EXPECT_TRUE(service->serverName().isEmpty());
    EXPECT_EQ(service->tcpPort(), 0);
}

TEST_F(ServiceTest, LocalServer) {
    // 测试本地socket服务
    EXPECT_TRUE(service->listen("test.service"));
    EXPECT_EQ(service->serverName(), "test.service");
    
    service->close();
    EXPECT_TRUE(service->serverName().isEmpty());
}

TEST_F(ServiceTest, InvalidLocalServerName) {
    // 测试无效服务名
    EXPECT_FALSE(service->listen(""));
    EXPECT_FALSE(service->listen("invalid/service/name"));
}

TEST_F(ServiceTest, TcpServer) {
    // 测试TCP服务
    EXPECT_TRUE(service->listenTcp(QHostAddress::LocalHost, 0));
    EXPECT_NE(service->tcpPort(), 0);
    EXPECT_EQ(service->tcpAddress(), QHostAddress::LocalHost);
    
    quint16 port = service->tcpPort();
    service->close();
    EXPECT_EQ(service->tcpPort(), 0);
    
    // 测试指定端口
    EXPECT_TRUE(service->listenTcp(QHostAddress::LocalHost, port));
    EXPECT_EQ(service->tcpPort(), port);
}

TEST_F(ServiceTest, TcpPortConflict) {
    // 测试端口冲突
    SlotIPCService anotherService;
    EXPECT_TRUE(service->listenTcp(QHostAddress::LocalHost, 0));
    quint16 port = service->tcpPort();
    EXPECT_FALSE(anotherService.listenTcp(QHostAddress::LocalHost, port));
}

TEST_F(ServiceTest, MultipleClose) {
    // 测试重复关闭
    service->close();
    service->close(); // 不应该崩溃
    
    EXPECT_TRUE(service->listen("test.service"));
    service->close();
    service->close(); // 关闭已关闭的服务
}

/*
TEST_F(ServiceTest, Signals) {
    // 测试信号发射(暂时禁用，因为缺少相关信号定义)
    QSignalSpy newConnectionSpy(service, &SlotIPCService::newConnection);
    QSignalSpy disconnectedSpy(service, &SlotIPCService::disconnected);
    
    EXPECT_TRUE(service->listen("test.service"));
    EXPECT_EQ(newConnectionSpy.count(), 0);
    EXPECT_EQ(disconnectedSpy.count(), 0);
    
    // 实际连接测试需要在connection测试中完成
}
*/

/*
TEST_F(ServiceTest, ThreadSafety) {
    // 测试多线程安全性(暂时禁用)
}
*/