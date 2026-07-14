// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "protoendpoint.h"
#include "protoserver.h"
#include "protoclient.h"
#include "asioservice.h"
#include "manager/secureconfig.h"
#include "session.h"

#include <memory>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

// Simple mock callback for SessionCallInterface
class MockSessionCallback : public SessionCallInterface
{
public:
    void onReceivedMessage(const proto::OriginMessage &, proto::OriginMessage *) override {}
    bool onStateChanged(int, std::string &) override { return false; }
};

class ProtoEndpointTest : public ::testing::Test {
protected:
    std::shared_ptr<AsioService> service;
    void SetUp() override
    {
        service = std::make_shared<AsioService>();
        service->Start();
    }
    void TearDown() override
    {
        if (service) service->Stop();
    }
};

// ProtoEndpoint methods tested through ProtoClient/ProtoServer instances

TEST_F(ProtoEndpointTest, SetCallbacks)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 12345);
    auto cb = std::make_shared<MockSessionCallback>();
    client.setCallbacks(cb);
    EXPECT_NE(client._callbacks, nullptr);
}

TEST_F(ProtoEndpointTest, HasConnectedDefaultFalse)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 12346);
    EXPECT_FALSE(client.hasConnected("192.168.1.1"));
}

TEST_F(ProtoEndpointTest, ConnectReplyedDefaultFalse)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 12347);
    EXPECT_FALSE(client.connectReplyed());
}

TEST_F(ProtoEndpointTest, SetRealIP)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 12348);
    client.setRealIP("10.0.0.99");
    SUCCEED();
}

TEST_F(ProtoEndpointTest, ServerHasConnectedDefaultFalse)
{
    auto context = SecureConfig::serverContext();
    ProtoServer server(service, context, 12349);
    EXPECT_FALSE(server.hasConnected("192.168.1.1"));
}

TEST_F(ProtoEndpointTest, ServerSetCallbacks)
{
    auto context = SecureConfig::serverContext();
    ProtoServer server(service, context, 12350);
    auto cb = std::make_shared<MockSessionCallback>();
    server.setCallbacks(cb);
    EXPECT_NE(server._callbacks, nullptr);
}

TEST_F(ProtoEndpointTest, SyncRequestNoConnectionReturnsEmpty)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 12351);
    auto cb = std::make_shared<MockSessionCallback>();
    client.setCallbacks(cb);

    proto::OriginMessage msg;
    msg.mask = 1000;
    msg.json_msg = R"({"test":"data"})";
    auto result = client.syncRequest("127.0.0.1", msg);
    EXPECT_TRUE(result.json_msg.empty());
}

TEST_F(ProtoEndpointTest, SendDisRequestNoConnection)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 12352);
    auto cb = std::make_shared<MockSessionCallback>();
    client.setCallbacks(cb);
    EXPECT_NO_FATAL_FAILURE(client.sendDisRequest());
}

TEST_F(ProtoEndpointTest, HasConnectedVirtualDefault)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 12353);
    // ProtoEndpoint::hasConnected virtual returns false by default
    EXPECT_FALSE(client.hasConnected("any_ip"));
}
