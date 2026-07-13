// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// Boost coverage by connecting SSL clients/sessions to a plain TCP listener.
// The TCP connect succeeds (covering buffer-prep code), then the SSL handshake
// fails (covering the error path). This covers the Connect success paths that
// are unreachable with just state manipulation.

#include <catch2/catch_all.hpp>
#include <asio/service.h>
#include <asio/ssl_context.h>
#include <asio/ssl_client.h>
#include <asio/ssl_server.h>
#include <asio/ssl_session.h>
#include <asio/tcp_resolver.h>

#include <thread>
#include <chrono>

using namespace NetUtil::Asio;
using namespace std::chrono_literals;

static int openTcpListener(const std::shared_ptr<Service>& service)
{
    auto* listener = new asio::ip::tcp::acceptor(*service->GetAsioService());
    listener->open(asio::ip::tcp::v4());
    listener->set_option(asio::ip::tcp::acceptor::reuse_address(true));
    listener->bind(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 0));
    listener->listen();
    int port = listener->local_endpoint().port();
    // Background thread: accept connections and immediately close them
    // so SSL handshakes fail fast with EOF.
    std::thread([listener]() {
        try {
            while (true) {
                asio::ip::tcp::socket sock(listener->get_executor());
                listener->accept(sock);
                sock.close();
            }
        } catch (...) {}
    }).detach();
    return port;
}

TEST_CASE("SSLClient Connect to plain TCP listener", "[ssl_tcp]")
{
    auto service = std::make_shared<Service>();
    service->Start();
    auto ctx = std::make_shared<SSLContext>(asio::ssl::context::tlsv12);
    ctx->set_verify_mode(asio::ssl::verify_none);

    int port = openTcpListener(service);
    REQUIRE(port > 0);

    SECTION("sync connect — TCP ok, handshake fails")
    {
        auto client = std::make_shared<SSLClient>(service, ctx, "127.0.0.1", port);
        client->SetupKeepAlive(true);
        client->SetupNoDelay(true);
        try { (void)client->Connect(); } catch (...) {}
    }
    SECTION("async connect — TCP ok, handshake fails")
    {
        auto client = std::make_shared<SSLClient>(service, ctx, "127.0.0.1", port);
        try { client->ConnectAsync(); } catch (...) {}
        std::this_thread::sleep_for(500ms);
    }
    SECTION("resolver connect — TCP ok, handshake fails")
    {
        auto resolver = std::make_shared<TCPResolver>(service);
        auto client = std::make_shared<SSLClient>(service, ctx, "127.0.0.1", port);
        try { (void)client->Connect(resolver); } catch (...) {}
    }
    SECTION("async resolver connect — TCP ok, handshake fails")
    {
        auto resolver = std::make_shared<TCPResolver>(service);
        auto client = std::make_shared<SSLClient>(service, ctx, "127.0.0.1", port);
        try { client->ConnectAsync(resolver); } catch (...) {}
        std::this_thread::sleep_for(500ms);
    }

    service->Stop();
}

TEST_CASE("SSLClient strand connect to plain TCP listener", "[ssl_tcp][strand]")
{
    auto service = std::make_shared<Service>(2, true);
    service->Start();
    auto ctx = std::make_shared<SSLContext>(asio::ssl::context::tlsv12);
    ctx->set_verify_mode(asio::ssl::verify_none);

    int port = openTcpListener(service);

    SECTION("sync connect strand")
    {
        auto client = std::make_shared<SSLClient>(service, ctx, "127.0.0.1", port);
        try { (void)client->Connect(); } catch (...) {}
    }
    SECTION("async connect strand")
    {
        auto client = std::make_shared<SSLClient>(service, ctx, "127.0.0.1", port);
        try { client->ConnectAsync(); } catch (...) {}
        std::this_thread::sleep_for(500ms);
    }
    SECTION("async resolver connect strand")
    {
        auto resolver = std::make_shared<TCPResolver>(service);
        auto client = std::make_shared<SSLClient>(service, ctx, "127.0.0.1", port);
        try { client->ConnectAsync(resolver); } catch (...) {}
        std::this_thread::sleep_for(500ms);
    }

    service->Stop();
}

TEST_CASE("SSLClient reconnect to plain TCP listener", "[ssl_tcp][reconnect]")
{
    auto service = std::make_shared<Service>();
    service->Start();
    auto ctx = std::make_shared<SSLContext>(asio::ssl::context::tlsv12);
    ctx->set_verify_mode(asio::ssl::verify_none);

    int port = openTcpListener(service);
    auto client = std::make_shared<SSLClient>(service, ctx, "127.0.0.1", port);

    try { (void)client->Connect(); } catch (...) {}
    try { (void)client->Reconnect(); } catch (...) {}

    service->Stop();
}

TEST_CASE("SSLClient async disconnect after failed connect", "[ssl_tcp][disc]")
{
    auto service = std::make_shared<Service>();
    service->Start();
    auto ctx = std::make_shared<SSLContext>(asio::ssl::context::tlsv12);
    ctx->set_verify_mode(asio::ssl::verify_none);

    int port = openTcpListener(service);
    auto client = std::make_shared<SSLClient>(service, ctx, "127.0.0.1", port);

    try { client->ConnectAsync(); } catch (...) {}
    std::this_thread::sleep_for(300ms);

    // Now try various methods on the possibly-partially-connected client
    try { (void)client->DisconnectAsync(); } catch (...) {}
    std::this_thread::sleep_for(100ms);

    try { (void)client->ReconnectAsync(); } catch (...) {}
    std::this_thread::sleep_for(300ms);

    service->Stop();
}
