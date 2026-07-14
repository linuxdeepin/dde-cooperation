// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "protoserver.h"
#include "protoclient.h"
#include "asioservice.h"
#include "manager/secureconfig.h"
#include "session.h"

#include <memory>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

class MockCallback : public SessionCallInterface
{
public:
    void onReceivedMessage(const proto::OriginMessage &, proto::OriginMessage *resp) override {}
    bool onStateChanged(int, std::string &) override { return false; }
};

class ProtoClientServerTest : public ::testing::Test {
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

// ---- ProtoClient tests ----

TEST_F(ProtoClientServerTest, ClientCreate)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 13001);
    SUCCEED();
}

TEST_F(ProtoClientServerTest, ClientHasConnected)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 13002);
    // _connected_host defaults to empty, so any non-empty IP returns false
    EXPECT_FALSE(client.hasConnected("192.168.1.1"));
}

TEST_F(ProtoClientServerTest, ClientConnectReplyedDefault)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 13003);
    EXPECT_FALSE(client.connectReplyed());
}

TEST_F(ProtoClientServerTest, ClientSetRealIP)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 13004);
    client.setRealIP("10.0.0.1");
    SUCCEED();
}

TEST_F(ProtoClientServerTest, ClientSetCallbacks)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 13005);
    auto cb = std::make_shared<MockCallback>();
    client.setCallbacks(cb);
    EXPECT_NE(client._callbacks, nullptr);
}

TEST_F(ProtoClientServerTest, ClientStartHeartbeatNotConnected)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 13006);
    // pingMessageStart returns false when _connected_host is empty or not handshaked
    EXPECT_FALSE(client.startHeartbeat());
}

TEST_F(ProtoClientServerTest, ClientPingTimerStop)
{
    auto context = SecureConfig::clientContext();
    ProtoClient client(service, context, "127.0.0.1", 13007);
    client.pingTimerStop();
    SUCCEED();
}

TEST_F(ProtoClientServerTest, ClientDisconnectAndStop)
{
    auto context = SecureConfig::clientContext();
    auto *client = new ProtoClient(service, context, "127.0.0.1", 13008);
    client->DisconnectAndStop();
    delete client;
    SUCCEED();
}

// ---- ProtoServer tests ----

TEST_F(ProtoClientServerTest, ServerCreate)
{
    auto context = SecureConfig::serverContext();
    ProtoServer server(service, context, 14001);
    SUCCEED();
}

TEST_F(ProtoClientServerTest, ServerHasConnectedEmpty)
{
    auto context = SecureConfig::serverContext();
    ProtoServer server(service, context, 14002);
    EXPECT_FALSE(server.hasConnected("192.168.1.1"));
}

TEST_F(ProtoClientServerTest, ServerHasConnectedDirectMatch)
{
    auto context = SecureConfig::serverContext();
    ProtoServer server(service, context, 14003);
    // Use -fno-access-control to set private member
    BaseKit::UUID id;
    server._session_ids.insert({"10.0.0.5", id});
    EXPECT_TRUE(server.hasConnected("10.0.0.5"));
}

TEST_F(ProtoClientServerTest, ServerHasConnectedViaIPMapping)
{
    auto context = SecureConfig::serverContext();
    ProtoServer server(service, context, 14004);
    // Set up IP mapping: real_ip -> remote_ip, remote_ip has session
    server._real_to_remote_ip["100.100.100.100"] = "10.0.0.1";
    server._remote_to_real_ip["10.0.0.1"] = "100.100.100.100";
    BaseKit::UUID id;
    server._session_ids.insert({"10.0.0.1", id});
    EXPECT_TRUE(server.hasConnected("100.100.100.100"));
}

TEST_F(ProtoClientServerTest, ServerHandleRealIPMapping)
{
    auto context = SecureConfig::serverContext();
    ProtoServer server(service, context, 14005);
    // Test IP mapping creation
    server.handleRealIPMapping("192.168.1.100", "10.0.0.1");
    EXPECT_EQ(server._real_to_remote_ip["192.168.1.100"], "10.0.0.1");
    EXPECT_EQ(server._remote_to_real_ip["10.0.0.1"], "192.168.1.100");
}

TEST_F(ProtoClientServerTest, ServerHandleRealIPMappingReplaceOld)
{
    auto context = SecureConfig::serverContext();
    ProtoServer server(service, context, 14006);
    // First mapping
    server.handleRealIPMapping("192.168.1.100", "10.0.0.1");
    // Replace with new mapping
    server.handleRealIPMapping("192.168.1.100", "10.0.0.2");
    EXPECT_EQ(server._real_to_remote_ip["192.168.1.100"], "10.0.0.2");
    EXPECT_EQ(server._remote_to_real_ip["10.0.0.2"], "192.168.1.100");
    // Old remote should be cleaned
    EXPECT_EQ(server._remote_to_real_ip.find("10.0.0.1"), server._remote_to_real_ip.end());
}

TEST_F(ProtoClientServerTest, ServerSetCallbacks)
{
    auto context = SecureConfig::serverContext();
    ProtoServer server(service, context, 14007);
    auto cb = std::make_shared<MockCallback>();
    server.setCallbacks(cb);
    EXPECT_NE(server._callbacks, nullptr);
}

TEST_F(ProtoClientServerTest, ServerHandlePingNewRemote)
{
    auto context = SecureConfig::serverContext();
    ProtoServer server(service, context, 14008);
    server.handlePing("10.0.0.99");
    // After handlePing, the remote should be in _ping_remotes
    EXPECT_TRUE(server._ping_remotes.find("10.0.0.99") != server._ping_remotes.end());
}

TEST_F(ProtoClientServerTest, ServerHandlePingExistingRemote)
{
    auto context = SecureConfig::serverContext();
    ProtoServer server(service, context, 14009);
    server.handlePing("10.0.0.99");
    server.handlePing("10.0.0.99");  // second ping should reset count
    SUCCEED();
}
