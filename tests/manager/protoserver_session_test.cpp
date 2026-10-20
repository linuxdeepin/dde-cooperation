// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "asioservice.h"
#include "protoclient.h"
#include "protoserver.h"
#include "manager/secureconfig.h"

#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QTest>

#include <functional>
#include <memory>

namespace {
bool waitFor(const std::function<bool()> &condition, int timeoutMs = 3000)
{
    QElapsedTimer timer;
    timer.start();
    while (!condition() && timer.elapsed() < timeoutMs)
        QTest::qWait(10);
    return condition();
}

class ServerCallbacks : public SessionCallInterface
{
public:
    void onReceivedMessage(const proto::OriginMessage &request, proto::OriginMessage *response) override
    {
        response->id = request.id;
        response->mask = request.mask;
        response->json_msg = R"({"ok":true})";
    }

    bool onStateChanged(int state, std::string &msg) override
    {
        Q_UNUSED(state)
        Q_UNUSED(msg)
        return false;
    }
};
}

// A peer that reconnects can register the new session before the old one is
// torn down. Dropping the old session must not remove the routing entry that
// points at the new one, otherwise B -> A RPCs start disappearing.
TEST(ProtoServerSessionTest, ReconnectKeepsRoutingToNewestSession)
{
    auto service = std::make_shared<AsioService>();
    service->Start();

    std::shared_ptr<ProtoServer> server;
    int port = 0;
    bool started = false;
    for (int attempt = 0; attempt < 10 && !started; ++attempt) {
        port = 18000 + QRandomGenerator::global()->bounded(1000);
        server = std::make_shared<ProtoServer>(service, SecureConfig::serverContext(), port);
        server->SetupReuseAddress(true);
        server->SetupReusePort(true);
        server->setCallbacks(std::make_shared<ServerCallbacks>());
        started = server->Start();
    }
    ASSERT_TRUE(started);

    auto client1 = std::make_shared<ProtoClient>(
            service, SecureConfig::clientContext(), "127.0.0.1", port);
    client1->ConnectAsync();
    ASSERT_TRUE(waitFor([&] { return client1->IsHandshaked(); }));
    ASSERT_TRUE(waitFor([&] { return server->hasConnected("127.0.0.1"); }));

    // Both connections come from the same address, like a reconnect.
    auto client2 = std::make_shared<ProtoClient>(
            service, SecureConfig::clientContext(), "127.0.0.1", port);
    client2->ConnectAsync();
    ASSERT_TRUE(waitFor([&] { return client2->IsHandshaked(); }));

    // Tear down the older session. The newer one must stay routable.
    client1->DisconnectAndStop();
    QTest::qWait(500);
    EXPECT_TRUE(server->hasConnected("127.0.0.1"));

    client2->DisconnectAndStop();
    server->Stop();
    service->Stop();
}
