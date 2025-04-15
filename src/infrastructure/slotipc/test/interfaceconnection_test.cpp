#include <gtest/gtest.h>
#include "slotipc/service.h"
#include "slotipc/interface.h"
#include "interfaceconnection_p.h"

class InterfaceConnectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        service = new SlotIPCService;
        interface = new SlotIPCInterface(service);
        connection = new SlotIPCInterfaceConnection(static_cast<QLocalSocket*>(nullptr), service);
    }

    void TearDown() override {
        delete connection;
        delete interface;
        delete service;
    }

    SlotIPCService* service;
    SlotIPCInterface* interface;
    SlotIPCInterfaceConnection* connection;
};

TEST_F(InterfaceConnectionTest, Constructor) {
    EXPECT_NE(connection, nullptr);
}

TEST_F(InterfaceConnectionTest, BasicTest) {
    EXPECT_NE(connection, nullptr);
}