// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QSignalSpy>

#include "protoserver.h"
#include "protoclient.h"
#include "asioservice.h"
#include "manager/secureconfig.h"
#include "session.h"

#include <memory>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

class TestCallback : public SessionCallInterface
{
public:
    int lastState = -999;
    std::string lastMsg;
    int msgCount = 0;

    void onReceivedMessage(const proto::OriginMessage &req, proto::OriginMessage *resp) override
    {
        msgCount++;
        resp->id = req.id;
        resp->mask = req.mask;
        resp->json_msg = R"({"reply":"ok"})";
    }

    bool onStateChanged(int state, std::string &msg) override
    {
        lastState = state;
        lastMsg = msg;
        return false;
    }
};

class SessionIntegrationTest : public ::testing::Test {
protected:
    std::shared_ptr<AsioService> service;
    std::shared_ptr<TestCallback> cb;

    void SetUp() override
    {
        service = std::make_shared<AsioService>();
        service->Start();
        cb = std::make_shared<TestCallback>();
    }
    void TearDown() override
    {
        if (service) service->Stop();
    }
};

// Test real server+client connection on localhost
TEST_F(SessionIntegrationTest, ServerStartStop)
{
    auto context = SecureConfig::serverContext();
    ProtoServer server(service, context, 15001);
    server.setCallbacks(cb);
    EXPECT_FALSE(server.hasConnected("127.0.0.1"));
}

TEST_F(SessionIntegrationTest, ServerHasConnectedDirectMatch)
{
    auto context = SecureConfig::serverContext();
    ProtoServer server(service, context, 15002);
    server.setCallbacks(cb);
    BaseKit::UUID id;
    server._session_ids.insert({"10.0.0.5", id});
    EXPECT_TRUE(server.hasConnected("10.0.0.5"));
}

TEST_F(SessionIntegrationTest, ServerHandleRealIPMapping)
{
    auto context = SecureConfig::serverContext();
    ProtoServer server(service, context, 15003);
    server.handleRealIPMapping("192.168.1.100", "10.0.0.1");
    EXPECT_EQ(server._real_to_remote_ip["192.168.1.100"], "10.0.0.1");
    EXPECT_EQ(server._remote_to_real_ip["10.0.0.1"], "192.168.1.100");
}

TEST_F(SessionIntegrationTest, ServerHandleRealIPMappingReplace)
{
    auto context = SecureConfig::serverContext();
    ProtoServer server(service, context, 15004);
    server.handleRealIPMapping("192.168.1.100", "10.0.0.1");
    server.handleRealIPMapping("192.168.1.100", "10.0.0.2");
    EXPECT_EQ(server._real_to_remote_ip["192.168.1.100"], "10.0.0.2");
    EXPECT_EQ(server._remote_to_real_ip.find("10.0.0.1"), server._remote_to_real_ip.end());
}

TEST_F(SessionIntegrationTest, ServerHandlePingNew)
{
    auto context = SecureConfig::serverContext();
    ProtoServer server(service, context, 15005);
    server.setCallbacks(cb);
    server.handlePing("10.0.0.99");
    EXPECT_TRUE(server._ping_remotes.find("10.0.0.99") != server._ping_remotes.end());
}

TEST_F(SessionIntegrationTest, ServerHandlePingExisting)
{
    auto context = SecureConfig::serverContext();
    ProtoServer server(service, context, 15006);
    server.setCallbacks(cb);
    server.handlePing("10.0.0.99");
    server.handlePing("10.0.0.99");
    SUCCEED();
}

TEST_F(SessionIntegrationTest, ServerHasConnectedViaIPMapping)
{
    auto context = SecureConfig::serverContext();
    ProtoServer server(service, context, 15007);
    server._real_to_remote_ip["100.100.100.100"] = "10.0.0.1";
    server._remote_to_real_ip["10.0.0.1"] = "100.100.100.100";
    BaseKit::UUID id;
    server._session_ids.insert({"10.0.0.1", id});
    EXPECT_TRUE(server.hasConnected("100.100.100.100"));
}

// ---- ProtoClient tests ----

TEST_F(SessionIntegrationTest, ClientHasConnectedDefault)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 16001);
    EXPECT_FALSE(client.hasConnected("192.168.1.1"));
}

TEST_F(SessionIntegrationTest, ClientConnectReplyedDefault)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 16002);
    EXPECT_FALSE(client.connectReplyed());
}

TEST_F(SessionIntegrationTest, ClientSetRealIP)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 16003);
    client.setRealIP("10.0.0.99");
    SUCCEED();
}

TEST_F(SessionIntegrationTest, ClientStartHeartbeatNotConnected)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 16004);
    EXPECT_FALSE(client.startHeartbeat());
}

TEST_F(SessionIntegrationTest, ClientPingTimerStop)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 16005);
    client.pingTimerStop();
    SUCCEED();
}

TEST_F(SessionIntegrationTest, ClientDisconnectAndStop)
{
    auto context = SecureConfig::clientContext();
    auto *client = new ProtoClient(service, context, "127.0.0.1", 16006);
    client->DisconnectAndStop();
    delete client;
    SUCCEED();
}

// ---- ProtoEndpoint via client ----

TEST_F(SessionIntegrationTest, ClientSyncRequestNoConnectionReturnsEmpty)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 16007);
    client.setCallbacks(cb);
    proto::OriginMessage msg;
    msg.mask = 1000;
    msg.json_msg = R"({"test":"data"})";
    auto result = client.syncRequest("127.0.0.1", msg);
    EXPECT_TRUE(result.json_msg.empty());
}

TEST_F(SessionIntegrationTest, ClientSendDisRequestNoConnection)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 16008);
    client.setCallbacks(cb);
    EXPECT_NO_FATAL_FAILURE(client.sendDisRequest());
}

TEST_F(SessionIntegrationTest, AsyncRequestWithHandlerTimeout)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 16009);
    client.setCallbacks(cb);

    proto::OriginMessage msg;
    msg.mask = 1000;
    msg.json_msg = R"({"test":"async"})";

    bool called = false;
    client.asyncRequestWithHandler("192.168.1.1", msg, [&](int32_t type, const std::string &) {
        called = true;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(3500));
    EXPECT_TRUE(called);
}

TEST_F(SessionIntegrationTest, SelfRequestFlagDefault)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 16010);
    EXPECT_FALSE(client._self_request.load());
}
