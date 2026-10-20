// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "asioservice.h"
#include "http/https_client.h"
#include "manager/secureconfig.h"

#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

#include <chrono>

// A connection dropped before the response arrives must complete the pending
// request future. Otherwise FileClient::downloadFile() stays blocked in
// future::get() and FileClient::~FileClient(), which joins the download
// thread on the GUI thread, freezes the application on cancel.
TEST(HTTPClientExTest, DisconnectResolvesPendingRequest)
{
    QTcpServer server;
    ASSERT_TRUE(server.listen(QHostAddress::LocalHost, 0));
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&server] {
        // Accept the TCP connection but never complete the TLS handshake.
        while (QTcpSocket *socket = server.nextPendingConnection())
            socket->abort();
    });

    auto service = std::make_shared<AsioService>();
    service->Start();

    auto client = std::make_shared<NetUtil::HTTP::HTTPSClientEx>(
            service, SecureConfig::clientContext(),
            "127.0.0.1", server.serverPort());
    auto future = client->SendGetRequest("download/payload.txt&token=test",
                                         BaseKit::Timespan::seconds(30));

    // Keep the event loop alive so QTcpServer accepts and drops the socket;
    // the request timeout is 30 seconds, so a ready future here can only come
    // from the disconnect handler.
    for (int i = 0; i < 50
         && future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready;
         ++i) {
        QTest::qWait(100);
    }

    ASSERT_EQ(future.wait_for(std::chrono::milliseconds(0)), std::future_status::ready);
    EXPECT_THROW(future.get(), std::runtime_error);

    client->DisconnectAsync();
    client.reset();
    service->Stop();
}
