// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "protoendpoint.h"
#include "protoclient.h"
#include "protoserver.h"
#include "session.h"

#include "asio/service.h"
#include "asio/ssl_context.h"

#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>

namespace {

class TestSessionCallbacks : public SessionCallInterface
{
public:
    int lastState = -100;
    std::string lastMsg;
    int callCount = 0;
    bool stateResult = true;

    void onReceivedMessage(const proto::OriginMessage &request, proto::OriginMessage *response) override
    {
        if (response) {
            response->id = request.id;
            response->mask = request.mask;
            response->json_msg = request.json_msg;
        }
    }

    bool onStateChanged(int state, std::string &msg) override
    {
        lastState = state;
        lastMsg = msg;
        callCount++;
        return stateResult;
    }
};

}

class SessionTest : public ::testing::Test {
protected:
    std::shared_ptr<NetUtil::Asio::Service> service;
    std::shared_ptr<NetUtil::Asio::SSLContext> context;
    std::shared_ptr<TestSessionCallbacks> callbacks;

    void SetUp() override
    {
        service = std::make_shared<NetUtil::Asio::Service>();
        service->Start();
        context = std::make_shared<NetUtil::Asio::SSLContext>(asio::ssl::context::tlsv12);
        callbacks = std::make_shared<TestSessionCallbacks>();
    }

    void TearDown() override
    {
        callbacks.reset();
        service->Stop();
    }

    void insertSession(ProtoServer &server, const std::string &ip)
    {
        BaseKit::UUID uid = BaseKit::UUID::Sequential();
        std::unique_lock<std::shared_mutex> locker(server._sessionids_lock);
        server._session_ids.insert(std::make_pair(ip, uid));
    }
};

TEST_F(SessionTest, ProtoServerConstruct)
{
    ProtoServer server(service, context, 19000);
    SUCCEED();
}

TEST_F(SessionTest, ProtoServerSetCallbacks)
{
    ProtoServer server(service, context, 19001);
    server.setCallbacks(callbacks);
    EXPECT_NE(server._callbacks, nullptr);
    EXPECT_EQ(server._callbacks.get(), callbacks.get());
}

TEST_F(SessionTest, ProtoServerHasConnectedEmpty)
{
    ProtoServer server(service, context, 19002);
    EXPECT_FALSE(server.hasConnected("192.168.1.1"));
    EXPECT_FALSE(server.hasConnected(""));
}

TEST_F(SessionTest, ProtoServerHasConnectedAfterSessionInsert)
{
    ProtoServer server(service, context, 19003);
    insertSession(server, "10.0.0.1");
    EXPECT_TRUE(server.hasConnected("10.0.0.1"));
}

TEST_F(SessionTest, ProtoServerHandleRealIPMappingInserts)
{
    ProtoServer server(service, context, 19004);
    server.handleRealIPMapping("203.0.113.5", "10.0.0.9");
    EXPECT_EQ(server._real_to_remote_ip.size(), 1u);
    EXPECT_EQ(server._remote_to_real_ip.size(), 1u);
    EXPECT_EQ(server._real_to_remote_ip["203.0.113.5"], "10.0.0.9");
    EXPECT_EQ(server._remote_to_real_ip["10.0.0.9"], "203.0.113.5");
}

TEST_F(SessionTest, ProtoServerHasConnectedViaMapping)
{
    ProtoServer server(service, context, 19005);
    server.handleRealIPMapping("203.0.113.5", "10.0.0.9");
    insertSession(server, "10.0.0.9");
    EXPECT_TRUE(server.hasConnected("203.0.113.5"));
}

TEST_F(SessionTest, ProtoServerHasConnectedMappingNoSession)
{
    ProtoServer server(service, context, 19006);
    server.handleRealIPMapping("203.0.113.5", "10.0.0.9");
    EXPECT_FALSE(server.hasConnected("203.0.113.5"));
}

TEST_F(SessionTest, ProtoServerHandleRealIPMappingOverwriteOld)
{
    ProtoServer server(service, context, 19007);
    server.handleRealIPMapping("203.0.113.5", "10.0.0.9");
    server.handleRealIPMapping("203.0.113.5", "10.0.0.10");
    EXPECT_EQ(server._real_to_remote_ip["203.0.113.5"], "10.0.0.10");
    EXPECT_EQ(server._remote_to_real_ip.size(), 1u);
    EXPECT_EQ(server._remote_to_real_ip["10.0.0.10"], "203.0.113.5");
}

TEST_F(SessionTest, ProtoServerHandleRealIPMappingOverwriteRemote)
{
    ProtoServer server(service, context, 19008);
    server.handleRealIPMapping("203.0.113.5", "10.0.0.9");
    server.handleRealIPMapping("203.0.113.6", "10.0.0.9");
    EXPECT_EQ(server._remote_to_real_ip["10.0.0.9"], "203.0.113.6");
    EXPECT_EQ(server._real_to_remote_ip.size(), 1u);
    EXPECT_EQ(server._real_to_remote_ip["203.0.113.6"], "10.0.0.9");
}

TEST_F(SessionTest, ProtoServerHandleRealIPMappingMultiple)
{
    ProtoServer server(service, context, 19009);
    server.handleRealIPMapping("1.1.1.1", "10.0.0.1");
    server.handleRealIPMapping("2.2.2.2", "10.0.0.2");
    server.handleRealIPMapping("3.3.3.3", "10.0.0.3");
    EXPECT_EQ(server._real_to_remote_ip.size(), 3u);
    EXPECT_EQ(server._remote_to_real_ip.size(), 3u);
    insertSession(server, "10.0.0.2");
    EXPECT_TRUE(server.hasConnected("2.2.2.2"));
    EXPECT_FALSE(server.hasConnected("1.1.1.1"));
    EXPECT_FALSE(server.hasConnected("3.3.3.3"));
}

TEST_F(SessionTest, ProtoServerHandleRealIPMappingEmptyStrings)
{
    ProtoServer server(service, context, 19010);
    server.handleRealIPMapping("", "");
    EXPECT_EQ(server._real_to_remote_ip.size(), 1u);
    EXPECT_EQ(server._real_to_remote_ip[""], "");
}

TEST_F(SessionTest, ProtoServerPingRemotesInitiallyEmpty)
{
    ProtoServer server(service, context, 19011);
    EXPECT_TRUE(server._ping_remotes.empty());
    EXPECT_EQ(server._ping_timer, nullptr);
}

TEST_F(SessionTest, ProtoClientConstruct)
{
    ProtoClient client(service, context, "127.0.0.1", 19012);
    SUCCEED();
}

TEST_F(SessionTest, ProtoClientSetCallbacks)
{
    ProtoClient client(service, context, "127.0.0.1", 19013);
    client.setCallbacks(callbacks);
    EXPECT_NE(client._callbacks, nullptr);
    EXPECT_EQ(client._callbacks.get(), callbacks.get());
}

TEST_F(SessionTest, ProtoClientHasConnectedDefaultEmpty)
{
    ProtoClient client(service, context, "127.0.0.1", 19014);
    EXPECT_FALSE(client.hasConnected("127.0.0.1"));
    EXPECT_TRUE(client.hasConnected(""));
}

TEST_F(SessionTest, ProtoClientHasConnectedMatch)
{
    ProtoClient client(service, context, "127.0.0.1", 19015);
    client._connected_host = "192.168.0.50";
    EXPECT_TRUE(client.hasConnected("192.168.0.50"));
    EXPECT_FALSE(client.hasConnected("192.168.0.51"));
}

TEST_F(SessionTest, ProtoClientSetRealIP)
{
    ProtoClient client(service, context, "127.0.0.1", 19016);
    EXPECT_TRUE(client._real_ip.empty());
    client.setRealIP("203.0.113.99");
    EXPECT_EQ(client._real_ip, "203.0.113.99");
}

TEST_F(SessionTest, ProtoClientSetRealIPEmpty)
{
    ProtoClient client(service, context, "127.0.0.1", 19017);
    client.setRealIP("");
    EXPECT_TRUE(client._real_ip.empty());
}

TEST_F(SessionTest, ProtoClientConnectReplyedDefault)
{
    ProtoClient client(service, context, "127.0.0.1", 19018);
    EXPECT_FALSE(client.connectReplyed());
}

TEST_F(SessionTest, ProtoClientConnectReplyedAfterSet)
{
    ProtoClient client(service, context, "127.0.0.1", 19019);
    client._connect_replay = true;
    EXPECT_TRUE(client.connectReplyed());
}

TEST_F(SessionTest, ProtoClientDisconnectAndStop)
{
    ProtoClient client(service, context, "127.0.0.1", 19020);
    client.DisconnectAndStop();
    EXPECT_TRUE(client._stop.load());
    EXPECT_FALSE(client._connect_replay.load());
}

TEST_F(SessionTest, ProtoClientStopFlagDefault)
{
    ProtoClient client(service, context, "127.0.0.1", 19021);
    EXPECT_FALSE(client._stop.load());
}

TEST_F(SessionTest, ProtoClientNopongDefault)
{
    ProtoClient client(service, context, "127.0.0.1", 19022);
    EXPECT_EQ(client._nopong_count.load(), 0);
}

TEST_F(SessionTest, ProtoClientPingTimerInitiallyNull)
{
    ProtoClient client(service, context, "127.0.0.1", 19023);
    EXPECT_EQ(client._ping_timer, nullptr);
}

TEST_F(SessionTest, ProtoEndpointActiveTargetDefault)
{
    ProtoClient client(service, context, "127.0.0.1", 19024);
    EXPECT_TRUE(client._active_target.empty());
    client._active_target = "1.2.3.4";
    EXPECT_EQ(client._active_target, "1.2.3.4");
}

TEST_F(SessionTest, ProtoEndpointSelfRequestDefault)
{
    ProtoServer server(service, context, 19025);
    EXPECT_FALSE(server._self_request.load());
    server._self_request.store(true);
    EXPECT_TRUE(server._self_request.load());
}

TEST_F(SessionTest, ProtoEndpointHasConnectedServerOverride)
{
    ProtoServer server(service, context, 19026);
    EXPECT_FALSE(server.hasConnected("9.9.9.9"));
    insertSession(server, "9.9.9.9");
    EXPECT_TRUE(server.hasConnected("9.9.9.9"));
}

TEST_F(SessionTest, ProtoClientRealIPOverride)
{
    ProtoClient client(service, context, "127.0.0.1", 19027);
    client.setRealIP("10.1.2.3");
    client.setRealIP("10.4.5.6");
    EXPECT_EQ(client._real_ip, "10.4.5.6");
}
