#include <gtest/gtest.h>
#include <QThread>
#include <QSignalSpy>
#include "slotipc/service.h"
#include "serviceconnection_p.h"

class ServiceConnectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        service = new SlotIPCService;
        connection = new SlotIPCServiceConnection(static_cast<QLocalSocket*>(nullptr), service);
    }

    void TearDown() override {
        delete connection;
        delete service;
    }

    SlotIPCService* service;
    SlotIPCServiceConnection* connection;
};

TEST_F(ServiceConnectionTest, Constructor) {
    EXPECT_NE(connection, nullptr);
}

TEST_F(ServiceConnectionTest, BasicTest) {
    EXPECT_NE(connection, nullptr);
    QObject subject;
    connection->setSubject(&subject);
}