// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "tunnelmanager.h"

#include "asioservice.h"
#include "fileclient.h"
#include "fileserver.h"
#include "manager/secureconfig.h"
#include "protoclient.h"
#include "protoserver.h"
#include "session.h"
#include "tunnelproto.h"

#include <QAbstractSocket>
#include <QFile>
#include <QHostAddress>
#include <QRandomGenerator>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

#include <atomic>
#include <memory>
#include <mutex>

TEST(TunnelManagerTest, EchoThroughReverseTunnel)
{
    QTcpServer echo;
    ASSERT_TRUE(echo.listen(QHostAddress::LocalHost, 0));
    QObject::connect(&echo, &QTcpServer::newConnection, &echo, [&echo] {
        while (QTcpSocket *socket = echo.nextPendingConnection()) {
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket] {
                socket->write(socket->readAll());
            });
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    });

    // "A" exposes the local echo service. "B" is only reachable from A, so B
    // listens on loopback and A dials the service through the tunnel.
    TunnelManager a;
    TunnelManager b;
    a.setPeer(QStringLiteral("B"));
    b.setPeer(QStringLiteral("A"));
    a.registerService(QStringLiteral("echo"), QStringLiteral("127.0.0.1"), echo.serverPort());

    a.setSendHandler([&b](const QString &, int mask, const QByteArray &payload) {
        b.onTunnelMessage(mask, payload, QStringLiteral("A"));
    });
    b.setSendHandler([&a](const QString &, int mask, const QByteArray &payload) {
        a.onTunnelMessage(mask, payload, QStringLiteral("B"));
    });

    const quint16 localPort = b.startLocalForward(QStringLiteral("echo"));
    ASSERT_NE(localPort, 0);

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, localPort);
    QTRY_COMPARE_WITH_TIMEOUT(client.state(), QAbstractSocket::ConnectedState, 3000);

    const QByteArray payload = QByteArrayLiteral("hello-tunnel");
    client.write(payload);
    QTRY_VERIFY_WITH_TIMEOUT(client.bytesAvailable() >= payload.size(), 3000);
    EXPECT_EQ(client.readAll(), payload);

    // Exercise chunking, the credit window and ACK handling.
    QByteArray large(512 * 1024, 'x');
    for (int i = 0; i < large.size(); ++i)
        large[i] = static_cast<char>('a' + (i % 26));

    client.write(large);
    QTRY_VERIFY_WITH_TIMEOUT(client.bytesAvailable() >= large.size(), 10000);
    EXPECT_EQ(client.readAll(), large);

    client.disconnectFromHost();
    b.stopAll();
    a.stopAll();
}

namespace {
class TunnelSessionCallback : public SessionCallInterface
{
public:
    void onReceivedMessage(const proto::OriginMessage &request, proto::OriginMessage *response) override
    {
        rpcCount.fetch_add(1);
        response->id = request.id;
        response->mask = request.mask;
        response->json_msg = (request.mask == silentMask.load()) ? "" : R"({"ok":true})";
    }

    // A request with this mask is deliberately left unanswered, keeping its
    // future pending on the sender side.
    std::atomic<int> silentMask { -1 };

    bool onStateChanged(int state, std::string &) override
    {
        if (state == RPC_CONNECTED)
            connected.fetch_add(1);
        return false;
    }

    bool onTunnelMessage(const std::string &remote, const proto::OriginMessage &msg) override
    {
        if (!isTunnelMessageType(msg.mask))
            return false;

        std::lock_guard<std::mutex> locker(mutex);
        lastTunnel = QByteArray(msg.json_msg.data(), static_cast<int>(msg.json_msg.size()));
        lastRemote = remote;
        lastMask = msg.mask;
        tunnelCount.fetch_add(1);
        return true;
    }

    QByteArray tunnelData() const
    {
        std::lock_guard<std::mutex> locker(mutex);
        return lastTunnel;
    }

    std::string remote() const
    {
        std::lock_guard<std::mutex> locker(mutex);
        return lastRemote;
    }

    mutable std::mutex mutex;
    QByteArray lastTunnel;
    std::string lastRemote;
    int lastMask { 0 };
    std::atomic<int> tunnelCount { 0 };
    std::atomic<int> rpcCount { 0 };
    std::atomic<int> connected { 0 };
};
}

TEST(TunnelManagerTest, SessionCarriesTunnelFrames)
{
    auto service = std::make_shared<AsioService>();
    service->Start();

    // A random port keeps repeated test runs from colliding with sockets that
    // are still in TIME_WAIT.
    const int port = 17000 + QRandomGenerator::global()->bounded(2000);
    auto server = std::make_shared<ProtoServer>(service, SecureConfig::serverContext(), port);
    server->SetupReuseAddress(true);
    server->SetupReusePort(true);
    auto serverCb = std::make_shared<TunnelSessionCallback>();
    server->setCallbacks(serverCb);
    ASSERT_TRUE(server->Start());

    auto client = std::make_shared<ProtoClient>(service, SecureConfig::clientContext(), "127.0.0.1", port);
    auto clientCb = std::make_shared<TunnelSessionCallback>();
    client->setCallbacks(clientCb);
    client->ConnectAsync();

    QTRY_VERIFY_WITH_TIMEOUT(serverCb->connected.load() > 0, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(clientCb->connected.load() > 0, 5000);

    const QByteArray upstream = QByteArrayLiteral("client-to-server-tunnel-frame");
    client->sendMessage("127.0.0.1", TUNNEL_DATA,
                        std::string(upstream.constData(), static_cast<size_t>(upstream.size())));
    QTRY_VERIFY_WITH_TIMEOUT(serverCb->tunnelCount.load() > 0, 3000);
    EXPECT_EQ(serverCb->tunnelData(), upstream);
    EXPECT_EQ(serverCb->remote(), std::string("127.0.0.1"));

    const QByteArray downstream = QByteArrayLiteral("server-to-client-tunnel-frame");
    server->sendMessage("127.0.0.1", TUNNEL_DATA,
                        std::string(downstream.constData(), static_cast<size_t>(downstream.size())));
    QTRY_VERIFY_WITH_TIMEOUT(clientCb->tunnelCount.load() > 0, 3000);
    EXPECT_EQ(clientCb->tunnelData(), downstream);

    // The tunnel must not disturb the normal request/response path.
    proto::OriginMessage request;
    request.mask = 1234;
    request.json_msg = R"({"hello":"rpc"})";
    const auto response = client->syncRequest("127.0.0.1", request);
    EXPECT_EQ(response.json_msg, std::string(R"({"ok":true})"));
    EXPECT_GT(serverCb->rpcCount.load(), 0);

    client->DisconnectAndStop();
    server->Stop();
    service->Stop();
}

TEST(TunnelManagerTest, MultipleForwardsRunIndependently)
{
    auto makeEcho = [](QTcpServer &server) {
        EXPECT_TRUE(server.listen(QHostAddress::LocalHost, 0));
        QObject::connect(&server, &QTcpServer::newConnection, &server, [&server] {
            while (QTcpSocket *socket = server.nextPendingConnection()) {
                QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket] {
                    socket->write(socket->readAll());
                });
                QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
    };

    QTcpServer echo1;
    QTcpServer echo2;
    makeEcho(echo1);
    makeEcho(echo2);

    TunnelManager a;
    TunnelManager b;
    a.setPeer(QStringLiteral("B"));
    b.setPeer(QStringLiteral("A"));
    a.registerService(QStringLiteral("one"), QStringLiteral("127.0.0.1"), echo1.serverPort());
    a.registerService(QStringLiteral("two"), QStringLiteral("127.0.0.1"), echo2.serverPort());

    a.setSendHandler([&b](const QString &, int mask, const QByteArray &payload) {
        b.onTunnelMessage(mask, payload, QStringLiteral("A"));
    });
    b.setSendHandler([&a](const QString &, int mask, const QByteArray &payload) {
        a.onTunnelMessage(mask, payload, QStringLiteral("B"));
    });

    const quint16 port1 = b.startLocalForward(QStringLiteral("one"));
    const quint16 port2 = b.startLocalForward(QStringLiteral("two"));
    ASSERT_NE(port1, 0);
    ASSERT_NE(port2, 0);
    EXPECT_NE(port1, port2);
    EXPECT_TRUE(b.isForwarding());

    QTcpSocket client1;
    QTcpSocket client2;
    client1.connectToHost(QHostAddress::LocalHost, port1);
    client2.connectToHost(QHostAddress::LocalHost, port2);
    QTRY_COMPARE_WITH_TIMEOUT(client1.state(), QAbstractSocket::ConnectedState, 3000);
    QTRY_COMPARE_WITH_TIMEOUT(client2.state(), QAbstractSocket::ConnectedState, 3000);

    client1.write(QByteArrayLiteral("service-one"));
    client2.write(QByteArrayLiteral("service-two"));
    QTRY_VERIFY_WITH_TIMEOUT(client1.bytesAvailable() >= 11, 3000);
    QTRY_VERIFY_WITH_TIMEOUT(client2.bytesAvailable() >= 11, 3000);
    EXPECT_EQ(client1.readAll(), QByteArrayLiteral("service-one"));
    EXPECT_EQ(client2.readAll(), QByteArrayLiteral("service-two"));

    // Stopping one service must not disturb the other one.
    b.stopLocalForward(QStringLiteral("one"));
    EXPECT_EQ(b.localPort(QStringLiteral("one")), 0);
    EXPECT_NE(b.localPort(QStringLiteral("two")), 0);
    QTRY_COMPARE_WITH_TIMEOUT(client1.state(), QAbstractSocket::UnconnectedState, 3000);

    client2.write(QByteArrayLiteral("still-alive"));
    QTRY_VERIFY_WITH_TIMEOUT(client2.bytesAvailable() >= 11, 3000);
    EXPECT_EQ(client2.readAll(), QByteArrayLiteral("still-alive"));

    b.stopAll();
    a.stopAll();
}

TEST(TunnelManagerTest, IncomingRequestIsNotSwallowedByPendingAsyncRequest)
{
    auto service = std::make_shared<AsioService>();
    service->Start();

    const int port = 17000 + QRandomGenerator::global()->bounded(2000);
    auto server = std::make_shared<ProtoServer>(service, SecureConfig::serverContext(), port);
    server->SetupReuseAddress(true);
    server->SetupReusePort(true);
    auto serverCb = std::make_shared<TunnelSessionCallback>();
    server->setCallbacks(serverCb);
    ASSERT_TRUE(server->Start());

    auto client = std::make_shared<ProtoClient>(service, SecureConfig::clientContext(), "127.0.0.1", port);
    auto clientCb = std::make_shared<TunnelSessionCallback>();
    client->setCallbacks(clientCb);
    client->ConnectAsync();

    QTRY_VERIFY_WITH_TIMEOUT(serverCb->connected.load() > 0, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(clientCb->connected.load() > 0, 5000);

    // The server never answers this request, so the client keeps a pending
    // async request while the server starts its own request below.
    serverCb->silentMask = 4242;
    clientCb->silentMask = 7777;

    std::atomic<bool> handlerDone { false };
    proto::OriginMessage pending;
    pending.mask = 4242;
    pending.json_msg = R"({"pending":true})";
    client->asyncRequestWithHandler("127.0.0.1", pending,
                                    [&handlerDone](int32_t, const std::string &) {
                                        handlerDone.store(true);
                                    });
    QTest::qWait(100);

    const int before = clientCb->rpcCount.load();
    server->sendMessage("127.0.0.1", 7777, R"({"peer":"request"})");

    // The peer initiated request must reach onReceivedMessage instead of
    // being consumed as the answer to the pending 4242 request.
    QTRY_VERIFY_WITH_TIMEOUT(clientCb->rpcCount.load() > before, 2000);

    // The async handler thread holds the client instance until its 3s timeout.
    QTRY_VERIFY_WITH_TIMEOUT(handlerDone.load(), 6000);

    client->DisconnectAndStop();
    server->Stop();
    service->Stop();
}

TEST(TunnelManagerTest, FileServerDownloadsThroughTunnel)
{
    QTemporaryDir sourceDir;
    QTemporaryDir destDir;
    ASSERT_TRUE(sourceDir.isValid());
    ASSERT_TRUE(destDir.isValid());

    QByteArray payload(256 * 1024, '\0');
    for (int i = 0; i < payload.size(); ++i)
        payload[i] = static_cast<char>('a' + (i % 26));

    const QString sourcePath = sourceDir.filePath(QStringLiteral("payload.txt"));
    {
        QFile file(sourcePath);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly));
        ASSERT_EQ(file.write(payload), payload.size());
    }

    // Reserve a port for the HTTPS file server, then release it before the
    // server binds.
    quint16 filePort = 0;
    {
        QTcpServer probe;
        ASSERT_TRUE(probe.listen(QHostAddress::LocalHost, 0));
        filePort = probe.serverPort();
    }
    ASSERT_NE(filePort, 0);

    auto service = std::make_shared<AsioService>();
    service->Start();

    auto fileServer = std::make_shared<FileServer>(service, SecureConfig::serverContext(),
                                                   std::string("127.0.0.1"), filePort);
    fileServer->clearBind();
    fileServer->webBind("payload.txt", sourcePath.toStdString());
    const std::string token = fileServer->genToken(R"(["payload.txt"])");
    ASSERT_TRUE(fileServer->start());

    TunnelManager a;
    TunnelManager b;
    a.setPeer(QStringLiteral("B"));
    b.setPeer(QStringLiteral("A"));
    // A owns the file server, B can only reach it through the tunnel.
    ASSERT_TRUE(a.registerService(QStringLiteral("file"),
                                  QStringLiteral("127.0.0.1"), filePort));
    a.setSendHandler([&b](const QString &, int mask, const QByteArray &frame) {
        b.onTunnelMessage(mask, frame, QStringLiteral("A"));
    });
    b.setSendHandler([&a](const QString &, int mask, const QByteArray &frame) {
        a.onTunnelMessage(mask, frame, QStringLiteral("B"));
    });

    const quint16 localPort = b.startLocalForward(QStringLiteral("file"));
    ASSERT_NE(localPort, 0);

    FileClient client(service, SecureConfig::clientContext(), "127.0.0.1", localPort);
    client.setConfig(token, destDir.path().toStdString());
    client.startFileDownload({ "payload.txt" });

    const QString destPath = destDir.filePath(QStringLiteral("payload.txt"));
    QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(destPath), 10000);
    QTRY_COMPARE_WITH_TIMEOUT(QFile(destPath).size(), static_cast<qint64>(payload.size()), 15000);
    QTest::qWait(200);

    {
        QFile file(destPath);
        ASSERT_TRUE(file.open(QIODevice::ReadOnly));
        EXPECT_EQ(file.readAll(), payload);
    }

    client.stop();
    b.stopAll();
    a.stopAll();
    fileServer->stop();
    fileServer->clearBind();
    service->Stop();
}

TEST(TunnelManagerTest, RejectUnknownService)
{
    TunnelManager a;
    TunnelManager b;
    a.setPeer(QStringLiteral("B"));
    b.setPeer(QStringLiteral("A"));

    a.setSendHandler([&b](const QString &, int mask, const QByteArray &payload) {
        b.onTunnelMessage(mask, payload, QStringLiteral("A"));
    });
    b.setSendHandler([&a](const QString &, int mask, const QByteArray &payload) {
        a.onTunnelMessage(mask, payload, QStringLiteral("B"));
    });

    QSignalSpy closed(&b, &TunnelManager::streamClosed);
    const quint16 localPort = b.startLocalForward(QStringLiteral("missing"));
    ASSERT_NE(localPort, 0);

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, localPort);
    QTRY_COMPARE_WITH_TIMEOUT(client.state(), QAbstractSocket::ConnectedState, 3000);

    QTRY_VERIFY_WITH_TIMEOUT(closed.count() > 0, 3000);
    QTRY_COMPARE_WITH_TIMEOUT(client.state(), QAbstractSocket::UnconnectedState, 3000);

    b.stopAll();
    a.stopAll();
}
