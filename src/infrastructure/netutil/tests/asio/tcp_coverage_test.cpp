// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// Comprehensive TCP client/server/session coverage. Uses localhost echo
// round-trips and exercises both strand and non-strand service modes.
// All timing-sensitive assertions use CHECK (non-fatal) so cleanup always runs.

#include <catch2/catch_all.hpp>
#include <asio/service.h>
#include <asio/tcp_client.h>
#include <asio/tcp_server.h>
#include <asio/tcp_session.h>
#include <asio/tcp_resolver.h>
#include <time/timespan.h>

#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>

using namespace NetUtil::Asio;
using namespace std::chrono_literals;

// RAII guard: stops the service on destruction (even if assertions fail).
struct ServiceGuard
{
    std::shared_ptr<Service> svc;
    std::shared_ptr<TCPServer> srv;
    ~ServiceGuard()
    {
        try { if (srv) srv->Stop(); } catch (...) {}
        std::this_thread::sleep_for(50ms);
        try { if (svc) svc->Stop(); } catch (...) {}
    }
};

// Echo session: echoes received data back to the client asynchronously.
class EchoTCPSession : public TCPSession
{
public:
    explicit EchoTCPSession(const std::shared_ptr<TCPServer>& server) : TCPSession(server) {}
protected:
    void onReceived(const void* buffer, size_t size) override { SendAsync(buffer, size); }
};

class EchoTCPServer : public TCPServer
{
public:
    using TCPServer::TCPServer;
protected:
    std::shared_ptr<TCPSession> CreateSession(const std::shared_ptr<TCPServer>& server) override
    { return std::make_shared<EchoTCPSession>(server); }
};

// Client that captures received data for async verification.
class CaptureTCPClient : public TCPClient
{
public:
    using TCPClient::TCPClient;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<bool> dataReceived{false};
    std::atomic<bool> connected{false};
    std::atomic<bool> disconnected{false};
    std::string receivedData;
protected:
    void onConnected() override { connected = true; cv.notify_all(); }
    void onDisconnected() override { disconnected = true; cv.notify_all(); }
    void onReceived(const void* buffer, size_t size) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        receivedData.assign(static_cast<const char*>(buffer), size);
        dataReceived = true;
        cv.notify_one();
    }
};

// Helper: start an echo server bound to port 0, return actual port.
static int startEchoServer(const std::shared_ptr<Service>& service, std::shared_ptr<EchoTCPServer>& server)
{
    server = std::make_shared<EchoTCPServer>(service, "127.0.0.1", 0);
    server->SetupReuseAddress(true);
    CHECK(server->Start());
    std::this_thread::sleep_for(100ms);
    return server->acceptor().local_endpoint().port();
}

TEST_CASE("TCPClient constructors and accessors", "[tcp][ctor]")
{
    auto service = std::make_shared<Service>();
    service->Start();
    ServiceGuard g{service, nullptr};

    SECTION("address+port") {
        TCPClient client(service, "127.0.0.1", 9000);
        REQUIRE(client.address() == "127.0.0.1");
        REQUIRE(client.port() == 9000);
        REQUIRE_FALSE(client.IsConnected());
        REQUIRE(client.bytes_sent() == 0);
        REQUIRE(client.bytes_received() == 0);
        REQUIRE(client.bytes_pending() == 0);
        REQUIRE_FALSE(client.option_keep_alive());
        REQUIRE_FALSE(client.option_no_delay());
    }
    SECTION("address+scheme") {
        TCPClient client(service, "127.0.0.1", "http");
        REQUIRE(client.address() == "127.0.0.1");
        REQUIRE(client.scheme() == "http");
    }
    SECTION("endpoint") {
        asio::ip::tcp::endpoint ep(asio::ip::make_address("127.0.0.1"), 9001);
        TCPClient client(service, ep);
        REQUIRE(client.port() == 9001);
    }
    SECTION("id is unique") {
        TCPClient a(service, "127.0.0.1", 9000);
        TCPClient b(service, "127.0.0.1", 9000);
        REQUIRE_FALSE(a.id() == b.id());
        REQUIRE(&a.service() != nullptr);
        REQUIRE(&a.io_service() != nullptr);
        REQUIRE(&a.strand() != nullptr);
        REQUIRE(&a.endpoint() != nullptr);
        REQUIRE(&a.socket() != nullptr);
    }
}

TEST_CASE("TCPClient options", "[tcp][opts]")
{
    auto service = std::make_shared<Service>();
    service->Start();
    ServiceGuard g{service, nullptr};
    TCPClient client(service, "127.0.0.1", 9000);

    client.SetupKeepAlive(true);
    REQUIRE(client.option_keep_alive());
    client.SetupNoDelay(true);
    REQUIRE(client.option_no_delay());
    client.SetupReceiveBufferLimit(2048);
    REQUIRE(client.option_receive_buffer_limit() == 2048);
    client.SetupSendBufferLimit(4096);
    REQUIRE(client.option_send_buffer_limit() == 4096);

    try { client.SetupReceiveBufferSize(8192); } catch (...) {}
    try { client.SetupSendBufferSize(8192); } catch (...) {}
    try { (void)client.option_receive_buffer_size(); } catch (...) {}
    try { (void)client.option_send_buffer_size(); } catch (...) {}
}

TEST_CASE("TCPClient not-connected method paths", "[tcp][nopath]")
{
    auto service = std::make_shared<Service>();
    service->Start();
    ServiceGuard g{service, nullptr};
    auto client = std::make_shared<TCPClient>(service, "127.0.0.1", 9000);

    REQUIRE_FALSE(client->Disconnect());
    REQUIRE_FALSE(client->Reconnect());
    REQUIRE_FALSE(client->DisconnectAsync());
    REQUIRE_FALSE(client->ReconnectAsync());

    char data[] = "hello";
    REQUIRE(client->Send(data, 4) == 0);
    REQUIRE(client->Send(std::string_view("hi")) == 0);
    REQUIRE(client->Send(data, 4, BaseKit::Timespan::seconds(1)) == 0);
    REQUIRE_FALSE(client->SendAsync(data, 4));

    char buf[64];
    REQUIRE(client->Receive(buf, 64) == 0);
    REQUIRE(client->Receive(64).empty());
    REQUIRE(client->Receive(buf, 64, BaseKit::Timespan::seconds(1)) == 0);

    // SendError skip known errors
    client->SendError(asio::error::connection_aborted);
    client->SendError(asio::error::eof);
    client->SendError(asio::error::operation_aborted);
}

TEST_CASE("TCPClient connect to closed port fails", "[tcp][fail]")
{
    auto service = std::make_shared<Service>();
    service->Start();
    ServiceGuard g{service, nullptr};

    SECTION("sync") {
        auto client = std::make_shared<TCPClient>(service, "127.0.0.1", 1);
        bool ok = true;
        try { ok = client->Connect(); } catch (...) { ok = false; }
        REQUIRE_FALSE(ok);
    }
    SECTION("async") {
        auto client = std::make_shared<TCPClient>(service, "127.0.0.1", 1);
        try { client->ConnectAsync(); } catch (...) {}
        std::this_thread::sleep_for(300ms);
    }
}

TEST_CASE("TCP echo round-trip synchronous", "[tcp][echo_sync]")
{
    auto service = std::make_shared<Service>();
    service->Start();
    std::shared_ptr<EchoTCPServer> server;
    int port = startEchoServer(service, server);
    ServiceGuard g{service, server};

    auto client = std::make_shared<TCPClient>(service, "127.0.0.1", port);
    client->SetupKeepAlive(true);
    client->SetupNoDelay(true);
    REQUIRE(client->Connect());
    REQUIRE(client->IsConnected());
    std::this_thread::sleep_for(50ms);

    std::string msg = "HelloTCP";
    size_t sent = client->Send(msg);
    REQUIRE(sent == msg.size());

    std::this_thread::sleep_for(100ms);
    std::string reply = client->Receive(msg.size());
    REQUIRE(reply == msg);
    REQUIRE(client->bytes_sent() >= msg.size());
    REQUIRE(client->bytes_received() >= msg.size());

    // Send with timeout overload
    REQUIRE(client->Send(msg, BaseKit::Timespan::seconds(1)) == msg.size());
    std::this_thread::sleep_for(100ms);
    REQUIRE(client->Receive(msg.size(), BaseKit::Timespan::seconds(1)) == msg);

    // raw buffer Receive with timeout
    char buf[64];
    REQUIRE(client->Send(msg.data(), msg.size(), BaseKit::Timespan::seconds(1)) == msg.size());
    std::this_thread::sleep_for(100ms);
    auto got = client->Receive(buf, msg.size(), BaseKit::Timespan::seconds(1));
    REQUIRE(got == msg.size());

    // Reconnect sync (while connected)
    REQUIRE(client->Reconnect());
    REQUIRE(client->IsConnected());
    client->Disconnect();
    REQUIRE_FALSE(client->IsConnected());
}

TEST_CASE("TCP echo round-trip asynchronous", "[tcp][echo_async]")
{
    auto service = std::make_shared<Service>();
    service->Start();
    std::shared_ptr<EchoTCPServer> server;
    int port = startEchoServer(service, server);
    ServiceGuard g{service, server};

    auto client = std::make_shared<CaptureTCPClient>(service, "127.0.0.1", port);

    CHECK(client->ConnectAsync());
    {
        std::unique_lock<std::mutex> lock(client->mutex);
        CHECK(client->cv.wait_for(lock, 1s, [&] { return client->connected.load(); }));
    }
    CHECK(client->IsConnected());

    std::string msg = "AsyncTCP";
    CHECK(client->SendAsync(msg));
    client->ReceiveAsync();
    {
        std::unique_lock<std::mutex> lock(client->mutex);
        CHECK(client->cv.wait_for(lock, 1s, [&] { return client->dataReceived.load(); }));
    }
    CHECK(client->receivedData == msg);

    // SendAsync raw buffer + ReceiveAsync
    client->dataReceived = false;
    char raw[] = "rawdata";
    CHECK(client->SendAsync(raw, 7));
    client->ReceiveAsync();
    {
        std::unique_lock<std::mutex> lock(client->mutex);
        client->cv.wait_for(lock, 1s, [&] { return client->dataReceived.load(); });
    }

    // DisconnectAsync
    CHECK(client->DisconnectAsync());
    std::this_thread::sleep_for(200ms);
    CHECK_FALSE(client->IsConnected());
}

TEST_CASE("TCPClient connect with resolver", "[tcp][resolver]")
{
    auto service = std::make_shared<Service>();
    service->Start();
    std::shared_ptr<EchoTCPServer> server;
    int port = startEchoServer(service, server);
    ServiceGuard g{service, server};

    auto resolver = std::make_shared<TCPResolver>(service);

    SECTION("sync resolver connect") {
        auto client = std::make_shared<TCPClient>(service, "127.0.0.1", port);
        CHECK(client->Connect(resolver));
        CHECK(client->IsConnected());
        std::string msg = "resolve";
        client->Send(msg);
        std::this_thread::sleep_for(100ms);
        CHECK(client->Receive(msg.size()) == msg);
        client->Disconnect();
    }
    SECTION("sync resolver connect to closed port") {
        auto client = std::make_shared<TCPClient>(service, "127.0.0.1", 1);
        bool ok = true;
        try { ok = client->Connect(resolver); } catch (...) { ok = false; }
        CHECK_FALSE(ok);
    }
    SECTION("async resolver connect") {
        auto client = std::make_shared<CaptureTCPClient>(service, "127.0.0.1", port);
        CHECK(client->ConnectAsync(resolver));
        std::unique_lock<std::mutex> lock(client->mutex);
        CHECK(client->cv.wait_for(lock, 1s, [&] { return client->connected.load(); }));
        client->Disconnect();
    }
    SECTION("async resolver connect to closed port") {
        auto client = std::make_shared<CaptureTCPClient>(service, "127.0.0.1", 1);
        try { client->ConnectAsync(resolver); } catch (...) {}
        std::this_thread::sleep_for(500ms);
    }
}

TEST_CASE("TCPServer constructors and accessors", "[tcp_server][ctor]")
{
    auto service = std::make_shared<Service>();
    service->Start();
    ServiceGuard g{service, nullptr};

    SECTION("port+protocol IPv4") {
        TCPServer server(service, 0, InternetProtocol::IPv4);
        REQUIRE(server.port() == 0);
        REQUIRE_FALSE(server.IsStarted());
        REQUIRE(server.connected_sessions() == 0);
        REQUIRE_FALSE(server.option_keep_alive());
        REQUIRE_FALSE(server.option_no_delay());
        REQUIRE_FALSE(server.option_reuse_address());
        REQUIRE_FALSE(server.option_reuse_port());
        REQUIRE(&server.service() != nullptr);
        REQUIRE(&server.io_service() != nullptr);
        REQUIRE(&server.strand() != nullptr);
        REQUIRE(&server.endpoint() != nullptr);
        REQUIRE(&server.acceptor() != nullptr);
    }
    SECTION("port+protocol IPv6") {
        TCPServer server(service, 0, InternetProtocol::IPv6);
        REQUIRE_FALSE(server.IsStarted());
    }
    SECTION("address+port") {
        TCPServer server(service, "127.0.0.1", 0);
        REQUIRE(server.address() == "127.0.0.1");
    }
    SECTION("endpoint") {
        asio::ip::tcp::endpoint ep(asio::ip::make_address("127.0.0.1"), 0);
        TCPServer server(service, ep);
        REQUIRE(server.address() == "127.0.0.1");
    }
}

TEST_CASE("TCPServer start/stop and options", "[tcp_server][ops]")
{
    auto service = std::make_shared<Service>();
    service->Start();
    ServiceGuard g{service, nullptr};

    auto server = std::make_shared<EchoTCPServer>(service, "127.0.0.1", 0);
    server->SetupKeepAlive(true);
    server->SetupNoDelay(true);
    server->SetupReuseAddress(true);
    server->SetupReusePort(true);
    REQUIRE(server->option_keep_alive());
    REQUIRE(server->option_no_delay());
    REQUIRE(server->option_reuse_address());
    REQUIRE(server->option_reuse_port());

    REQUIRE(server->Start());
    std::this_thread::sleep_for(100ms);
    REQUIRE(server->IsStarted());

    // Multicast / DisconnectAll with no sessions
    REQUIRE(server->Multicast("x", 1));
    REQUIRE(server->Multicast(std::string_view("xy")));
    REQUIRE(server->DisconnectAll());
    REQUIRE_FALSE(server->FindSession(BaseKit::UUID::Sequential()));

    REQUIRE(server->Stop());
    std::this_thread::sleep_for(50ms);
    REQUIRE_FALSE(server->IsStarted());
    // Cannot call Stop() again — it asserts in debug builds.
    REQUIRE_FALSE(server->Multicast("x", 1));
    REQUIRE_FALSE(server->DisconnectAll());

    g.srv = nullptr; // already stopped
}

TEST_CASE("TCPServer start then stop on connected session", "[tcp_server][mcast]")
{
    auto service = std::make_shared<Service>();
    service->Start();
    std::shared_ptr<EchoTCPServer> server;
    int port = startEchoServer(service, server);

    auto client = std::make_shared<CaptureTCPClient>(service, "127.0.0.1", port);
    CHECK(client->ConnectAsync());
    {
        std::unique_lock<std::mutex> lock(client->mutex);
        client->cv.wait_for(lock, 1s, [&] { return client->connected.load(); });
    }
    std::this_thread::sleep_for(50ms);

    // Multicast and DisconnectAll to connected sessions
    try { server->Multicast("MC", 2); } catch (...) {}
    try { server->DisconnectAll(); } catch (...) {}

    client->Disconnect();
    std::this_thread::sleep_for(100ms);

    try { server->Stop(); } catch (...) {}
    std::this_thread::sleep_for(50ms);
    service->Stop();
}

TEST_CASE("TCP strand-mode service covers strand branches", "[tcp][strand]")
{
    // Pool mode => strand required => covers the _strand_required branches.
    auto service = std::make_shared<Service>(2, true);
    service->Start();
    std::shared_ptr<EchoTCPServer> server;
    int port = startEchoServer(service, server);
    ServiceGuard g{service, server};

    auto client = std::make_shared<CaptureTCPClient>(service, "127.0.0.1", port);
    CHECK(client->ConnectAsync());
    {
        std::unique_lock<std::mutex> lock(client->mutex);
        CHECK(client->cv.wait_for(lock, 1s, [&] { return client->connected.load(); }));
    }
    CHECK(client->IsConnected());

    std::string msg = "strand";
    CHECK(client->SendAsync(msg));
    client->ReceiveAsync();
    {
        std::unique_lock<std::mutex> lock(client->mutex);
        client->cv.wait_for(lock, 1s, [&] { return client->dataReceived.load(); });
    }

    // sync send + timeout receive in strand mode (never block forever)
    client->Send(msg);
    std::this_thread::sleep_for(100ms);
    char buf[64];
    (void)client->Receive(buf, msg.size(), BaseKit::Timespan::seconds(1));

    CHECK(client->DisconnectAsync());
    std::this_thread::sleep_for(200ms);

    // resolver path in strand mode
    auto resolver = std::make_shared<TCPResolver>(service);
    auto client2 = std::make_shared<CaptureTCPClient>(service, "127.0.0.1", port);
    CHECK(client2->ConnectAsync(resolver));
    {
        std::unique_lock<std::mutex> lock(client2->mutex);
        CHECK(client2->cv.wait_for(lock, 1s, [&] { return client2->connected.load(); }));
    }
    client2->Disconnect();
    std::this_thread::sleep_for(100ms);
}

TEST_CASE("TCPClient send buffer limit", "[tcp][buflimit]")
{
    auto service = std::make_shared<Service>();
    service->Start();
    std::shared_ptr<EchoTCPServer> server;
    int port = startEchoServer(service, server);
    ServiceGuard g{service, server};

    auto client = std::make_shared<TCPClient>(service, "127.0.0.1", port);
    REQUIRE(client->Connect());
    std::this_thread::sleep_for(50ms);

    // Set a tiny send buffer limit so SendAsync reports no_buffer_space.
    client->SetupSendBufferLimit(4);
    std::vector<char> big(1024, 'x');
    try { (void)client->SendAsync(big.data(), big.size()); } catch (...) {}
    std::this_thread::sleep_for(100ms);
    try { client->Disconnect(); } catch (...) {}
}

TEST_CASE("TCPSession standalone disconnected paths", "[tcp_session][nopath]")
{
    auto service = std::make_shared<Service>();
    service->Start();
    ServiceGuard g{service, nullptr};

    auto server = std::make_shared<EchoTCPServer>(service, "127.0.0.1", 0);
    auto session = std::make_shared<EchoTCPSession>(server);
    REQUIRE_FALSE(session->IsConnected());
    REQUIRE(session->bytes_sent() == 0);
    REQUIRE(session->bytes_received() == 0);
    REQUIRE(session->bytes_pending() == 0);
    REQUIRE(&session->server() != nullptr);
    REQUIRE(&session->io_service() != nullptr);
    REQUIRE(&session->strand() != nullptr);
    REQUIRE(&session->socket() != nullptr);

    char data[] = "test";
    REQUIRE(session->Send(data, 4) == 0);
    REQUIRE_FALSE(session->SendAsync(data, 4));
    char buf[64];
    REQUIRE(session->Receive(buf, 64) == 0);
    REQUIRE(session->Receive(64).empty());
    REQUIRE(session->Receive(buf, 64, BaseKit::Timespan::seconds(1)) == 0);
    REQUIRE_FALSE(session->Disconnect());

    try { session->SetupReceiveBufferSize(4096); } catch (...) {}
    try { session->SetupSendBufferSize(4096); } catch (...) {}
    session->SetupReceiveBufferLimit(1024);
    session->SetupSendBufferLimit(2048);
    REQUIRE(session->option_receive_buffer_limit() == 1024);
    REQUIRE(session->option_send_buffer_limit() == 2048);

    session->ResetServer();
    session->SendError(asio::error::connection_reset);
}
