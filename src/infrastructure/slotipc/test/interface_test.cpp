#include <gtest/gtest.h>
#include "slotipc/interface.h"
#include "slotipc/service.h"

class InterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        service = new SlotIPCService;
        interface = new SlotIPCInterface;
    }

    void TearDown() override {
        delete interface;
        delete service;
    }

    SlotIPCService* service;
    SlotIPCInterface* interface;
};

TEST_F(InterfaceTest, Constructor) {
    EXPECT_NE(interface, nullptr);
    EXPECT_FALSE(interface->isConnected());
}

TEST_F(InterfaceTest, InvalidLocalConnection) {
    // 测试无效服务名连接
    EXPECT_FALSE(interface->connectToServer(""));
    EXPECT_FALSE(interface->connectToServer("invalid/service/name"));
    EXPECT_FALSE(interface->isConnected());
}

TEST_F(InterfaceTest, InvalidTcpConnection) {
    // 测试无效TCP连接
    EXPECT_FALSE(interface->connectToServer(QHostAddress::Null, 0));
    EXPECT_FALSE(interface->connectToServer(QHostAddress::LocalHost, 65535));
    EXPECT_FALSE(interface->isConnected());
}

TEST_F(InterfaceTest, LocalConnection) {
    // 测试本地服务连接
    EXPECT_TRUE(service->listen("test.service"));
    EXPECT_TRUE(interface->connectToServer("test.service"));
    EXPECT_TRUE(interface->isConnected());
    
    interface->disconnectFromServer();
    EXPECT_FALSE(interface->isConnected());
}

TEST_F(InterfaceTest, TcpConnection) {
    // 测试TCP服务连接
    EXPECT_TRUE(service->listenTcp(QHostAddress::LocalHost, 0));
    EXPECT_TRUE(interface->connectToServer(QHostAddress::LocalHost, service->tcpPort()));
    EXPECT_TRUE(interface->isConnected());
    
    interface->disconnectFromServer();
    EXPECT_FALSE(interface->isConnected());
}

TEST_F(InterfaceTest, MultipleConnectDisconnect) {
    // 测试重复连接/断开
    EXPECT_TRUE(service->listen("test.service"));
    
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(interface->connectToServer("test.service"));
        EXPECT_TRUE(interface->isConnected());
        interface->disconnectFromServer();
        EXPECT_FALSE(interface->isConnected());
    }
}

/*
TEST_F(InterfaceTest, Signals) {
    // 测试信号发射(暂时禁用)
}
*/

/*
TEST_F(InterfaceTest, ThreadSafety) {
    // 测试多线程连接(暂时禁用)
}
*/