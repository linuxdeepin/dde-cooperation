#include <gtest/gtest.h>
#include <QThread>
#include "interfaceworker.h"
#include "slotipc/interface.h"

class InterfaceWorkerTest : public ::testing::Test {
protected:
    void SetUp() override {
        interface = new SlotIPCInterface();
        worker = new SlotIPCInterfaceWorker(interface);
    }

    void TearDown() override {
        // worker->stop(); // 实际API没有stop方法
        delete worker;
        delete interface;
    }

    SlotIPCInterface* interface;
    SlotIPCInterfaceWorker* worker;
};

TEST_F(InterfaceWorkerTest, BasicTest) {
    EXPECT_NE(worker, nullptr);
    // 简单测试可用方法
    EXPECT_NO_FATAL_FAILURE(worker->isConnected());
    EXPECT_NO_FATAL_FAILURE(worker->connectionId());
}

// 简单的连接测试，不实际连接到服务器
TEST_F(InterfaceWorkerTest, ConnectionMethods) {
    EXPECT_NO_FATAL_FAILURE({
        worker->connectToServer("test.server", nullptr);
        worker->disconnectFromServer();
    });
}

// 测试发送请求方法
TEST_F(InterfaceWorkerTest, SendRequest) {
    QByteArray testRequest("test request");
    EXPECT_NO_FATAL_FAILURE(worker->sendCallRequest(testRequest));
}