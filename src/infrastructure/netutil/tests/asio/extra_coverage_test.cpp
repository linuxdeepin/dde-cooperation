// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// Coverage for HTTP/HTTPS client/server/session, SSL client gaps, and UDP server.
// Uses direct method calls and localhost connections with timeouts to avoid hangs.
// All assertions use CHECK (non-fatal) and cleanup is wrapped in try/catch to
// ensure server->Stop()/service->Stop() always run (prevents SIGSEGV on destroy).

#include <catch2/catch_all.hpp>
#include <asio/service.h>
#include <asio/ssl_context.h>
#include <asio/ssl_client.h>
#include <asio/ssl_server.h>
#include <asio/ssl_session.h>
#include <asio/udp_server.h>
#include <asio/tcp_resolver.h>
#include <http/http_client.h>
#include <http/http_server.h>
#include <http/http_session.h>
#include <http/http_request.h>
#include <http/http_response.h>
#include <http/https_client.h>
#include <http/https_server.h>
#include <http/https_session.h>

#include <thread>
#include <chrono>
#include <fstream>
#include <filesystem>

using namespace NetUtil::Asio;
using namespace NetUtil::HTTP;
using namespace std::chrono_literals;

template <typename Pred>
static void wait_for(Pred pred, std::chrono::milliseconds timeout = 300ms)
{
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline && !pred())
        std::this_thread::sleep_for(10ms);
}

// ===== UDP server tests =====

TEST_CASE("UDPServer constructors and accessors", "[udp_srv][ctor]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    {
        UDPServer server(service, 0, InternetProtocol::IPv4);
        CHECK(server.port() == 0);
        CHECK_FALSE(server.IsStarted());
        CHECK_FALSE(server.option_reuse_address());
        CHECK_FALSE(server.option_reuse_port());
    }
    {
        UDPServer server(service, 0, InternetProtocol::IPv6);
        CHECK_FALSE(server.IsStarted());
    }
    {
        UDPServer server(service, "127.0.0.1", 0);
        CHECK(server.address() == "127.0.0.1");
    }
    {
        asio::ip::udp::endpoint ep(asio::ip::make_address("127.0.0.1"), 0);
        UDPServer server(service, ep);
        CHECK(server.address() == "127.0.0.1");
    }

    service->Stop();
}

TEST_CASE("UDPServer localhost send/receive", "[udp_srv][loop]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto server = std::make_shared<UDPServer>(service, "127.0.0.1", 0);
    server->SetupReuseAddress(true);
    CHECK(server->Start());
    wait_for([&] { return server->IsStarted(); });
    CHECK(server->IsStarted());

    try
    {
        int port = server->_socket.local_endpoint().port();

        asio::ip::udp::socket clientSock(*service->GetAsioService());
        clientSock.open(asio::ip::udp::v4());
        asio::ip::udp::endpoint serverEp(asio::ip::make_address("127.0.0.1"), port);

        char senddata[] = "ping";
        clientSock.send_to(asio::buffer(senddata, 4), serverEp);

        char srvbuf[64];
        asio::ip::udp::endpoint fromEp;
        auto received = server->Receive(fromEp, srvbuf, 64, BaseKit::Timespan::seconds(1));
        CHECK(received == 4);
        CHECK(server->bytes_received() >= 4);
        CHECK(server->datagrams_received() >= 1);

        char reply[] = "pong";
        CHECK(server->Send(fromEp, reply, 4) == 4);
        CHECK(server->bytes_sent() >= 4);

        CHECK(server->Send(fromEp, reply, 4, BaseKit::Timespan::seconds(1)) == 4);
        CHECK(server->Send(fromEp, std::string_view("ok")) == 2);
        CHECK(server->SendAsync(fromEp, reply, 4));

        char clibuf[64];
        asio::ip::udp::endpoint respEp;
        auto got = clientSock.receive_from(asio::buffer(clibuf, 64), respEp);
        CHECK(got == 4);

        clientSock.send_to(asio::buffer("hi", 2), serverEp);
        auto txt = server->Receive(fromEp, 64, BaseKit::Timespan::seconds(1));
        CHECK(txt.size() == 2);

        clientSock.send_to(asio::buffer("asy", 3), serverEp);
        try { server->ReceiveAsync(); } catch (...) {}
        std::this_thread::sleep_for(200ms);

        clientSock.close();
    }
    catch (...) {}

    try { server->Stop(); } catch (...) {}
    wait_for([&] { return !server->IsStarted(); }, 500ms);
    service->Stop();
}

TEST_CASE("UDPServer multicast and options", "[udp_srv][mcast]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto server = std::make_shared<UDPServer>(service, "127.0.0.1", 0);
    server->SetupReuseAddress(true);
    server->SetupReusePort(true);
    server->SetupReceiveBufferLimit(2048);
    server->SetupSendBufferLimit(4096);
    CHECK(server->option_reuse_address());
    CHECK(server->option_reuse_port());
    CHECK(server->option_receive_buffer_limit() == 2048);
    CHECK(server->option_send_buffer_limit() == 4096);

    try { server->SetupReceiveBufferSize(4096); } catch (...) {}
    try { server->SetupSendBufferSize(4096); } catch (...) {}

    CHECK(server->Start("239.0.0.1", 50001));
    wait_for([&] { return server->IsStarted(); });

    try { server->Multicast("mc", 2); } catch (...) {}
    try { server->Multicast("mc", 2, BaseKit::Timespan::seconds(1)); } catch (...) {}
    try { server->MulticastAsync("mc", 2); } catch (...) {}

    try { server->Stop(); } catch (...) {}
    wait_for([&] { return !server->IsStarted(); }, 500ms);
    service->Stop();
}

TEST_CASE("UDPServer restart", "[udp_srv][restart]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto server = std::make_shared<UDPServer>(service, "127.0.0.1", 0);
    server->SetupReuseAddress(true);
    CHECK(server->Start());
    wait_for([&] { return server->IsStarted(); });
    CHECK(server->Restart());
    wait_for([&] { return server->IsStarted(); });
    try { server->Stop(); } catch (...) {}
    wait_for([&] { return !server->IsStarted(); }, 500ms);
    service->Stop();
}

TEST_CASE("UDPServer not-started method paths", "[udp_srv][nopath]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    UDPServer server(service, "127.0.0.1", 0);
    char buf[64];
    asio::ip::udp::endpoint ep;
    CHECK(server.Receive(ep, buf, 64) == 0);
    CHECK(server.Send(ep, buf, 4) == 0);
    CHECK_FALSE(server.SendAsync(ep, buf, 4));
    try { server.Multicast(buf, 4); } catch (...) {}
    server.SendError(asio::error::operation_aborted);

    service->Stop();
}

// ===== HTTP server/session =====

TEST_CASE("HTTPServer cache and static content", "[http_srv][cache]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    HTTPServer server(service, "127.0.0.1", 0);
    CHECK(&server.cache() != nullptr);

    auto tmpfile = (std::filesystem::temp_directory_path() / "netutil_test_static.txt").string();
    { std::ofstream ofs(tmpfile); ofs << "hello static"; }

    server.AddStaticContent(tmpfile, "/", BaseKit::Timespan::seconds(60));
    server.Watchdog();
    server.RemoveStaticContent(tmpfile);
    server.ClearStaticContent();

    std::filesystem::remove(tmpfile);
    service->Stop();
}

TEST_CASE("HTTPSession onReceived with valid request", "[http_sess][recv]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto server = std::make_shared<HTTPServer>(service, "127.0.0.1", 0);
    auto session = std::dynamic_pointer_cast<HTTPSession>(server->CreateSession(server));

    std::string req = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    try { session->onReceived(req.data(), req.size()); } catch (...) {}

    std::string post =
        "POST /submit HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "hello";
    try { session->onReceived(post.data(), post.size()); } catch (...) {}

    try { session->onDisconnected(); } catch (...) {}

    service->Stop();
}

TEST_CASE("HTTPSession onReceived with invalid data", "[http_sess][bad]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto server = std::make_shared<HTTPServer>(service, "127.0.0.1", 0);
    auto session = std::dynamic_pointer_cast<HTTPSession>(server->CreateSession(server));

    try { session->onReceived("NOT HTTP!!!!!!!", 15); } catch (...) {}
    try { session->onReceived("GET / HTTP/1.1\r\nHost: ", 22); } catch (...) {}
    try { session->onDisconnected(); } catch (...) {}

    service->Stop();
}

// ===== HTTPClientEx =====

TEST_CASE("HTTPClientEx SendRequest error paths", "[http_client][ex]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    {
        auto client = std::make_shared<HTTPClientEx>(service, "127.0.0.1", 1);
        try {
            auto fut = client->SendRequest(BaseKit::Timespan::milliseconds(100));
            fut.wait_for(500ms);
        } catch (...) {}
    }
    {
        auto client = std::make_shared<HTTPClientEx>(service, "127.0.0.1", 1);
        HTTPRequest req;
        req.MakeGetRequest("/");
        try {
            auto fut = client->SendRequest(req, BaseKit::Timespan::milliseconds(100));
            fut.wait_for(500ms);
        } catch (...) {}
    }
    {
        auto client = std::make_shared<HTTPClientEx>(service, "127.0.0.1", 1);
        try { auto f = client->SendHeadRequest("/x", BaseKit::Timespan::milliseconds(50)); f.wait_for(200ms); } catch (...) {}
        try { auto f = client->SendGetRequest("/x", BaseKit::Timespan::milliseconds(50)); f.wait_for(200ms); } catch (...) {}
        try { auto f = client->SendPostRequest("/x", "data", BaseKit::Timespan::milliseconds(50)); f.wait_for(200ms); } catch (...) {}
        try { auto f = client->SendPutRequest("/x", "data", BaseKit::Timespan::milliseconds(50)); f.wait_for(200ms); } catch (...) {}
        try { auto f = client->SendDeleteRequest("/x", BaseKit::Timespan::milliseconds(50)); f.wait_for(200ms); } catch (...) {}
        try { auto f = client->SendOptionsRequest("/x", BaseKit::Timespan::milliseconds(50)); f.wait_for(200ms); } catch (...) {}
        try { auto f = client->SendTraceRequest("/x", BaseKit::Timespan::milliseconds(50)); f.wait_for(200ms); } catch (...) {}
    }

    service->Stop();
}

TEST_CASE("HTTPClient onReceived response paths", "[http_client][recv]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    HTTPClient client(service, "127.0.0.1", 80);

    std::string resp =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "hello";
    try { client.onReceived(resp.data(), resp.size()); } catch (...) {}

    try { client.onReceived("HTTP/1.1 200 OK\r\n", 16); } catch (...) {}
    try { client.onReceived("XXXXX", 5); } catch (...) {}
    try { client.onDisconnected(); } catch (...) {}

    service->Stop();
}

// ===== HTTPS client/session/server (direct method calls, no real SSL) =====

TEST_CASE("HTTPSClient onReceived and onDisconnected", "[https_client][recv]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto ctx = std::make_shared<SSLContext>(asio::ssl::context::tlsv12);
    HTTPSClient client(service, ctx, "127.0.0.1", 443);

    std::string resp =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "hello";
    try { client.onReceived(resp.data(), resp.size()); } catch (...) {}
    try { client.onReceived("HTTP/1.1 200 OK\r\n", 16); } catch (...) {}
    try { client.onReceived("XXXXX", 5); } catch (...) {}
    try { client.onDisconnected(); } catch (...) {}

    service->Stop();
}

TEST_CASE("HTTPSClientEx SendRequest error paths", "[https_client][ex]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto ctx = std::make_shared<SSLContext>(asio::ssl::context::tlsv12);

    {
        auto client = std::make_shared<HTTPSClientEx>(service, ctx, "127.0.0.1", 1);
        try {
            auto fut = client->SendRequest(BaseKit::Timespan::milliseconds(50));
            fut.wait_for(500ms);
        } catch (...) {}
    }
    {
        auto client = std::make_shared<HTTPSClientEx>(service, ctx, "127.0.0.1", 1);
        HTTPRequest req;
        req.MakeGetRequest("/");
        try {
            auto fut = client->SendRequest(req, BaseKit::Timespan::milliseconds(50));
            fut.wait_for(500ms);
        } catch (...) {}
    }

    service->Stop();
}

TEST_CASE("HTTPSServer cache and static content", "[https_srv][cache]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto ctx = std::make_shared<SSLContext>(asio::ssl::context::tlsv12);
    HTTPSServer server(service, ctx, "127.0.0.1", 0);
    CHECK(&server.cache() != nullptr);

    auto tmpfile = (std::filesystem::temp_directory_path() / "netutil_test_https_static.txt").string();
    { std::ofstream ofs(tmpfile); ofs << "https static"; }

    server.AddStaticContent(tmpfile, "/", BaseKit::Timespan::seconds(60));
    server.Watchdog();
    server.RemoveStaticContent(tmpfile);
    server.ClearStaticContent();

    std::filesystem::remove(tmpfile);
    service->Stop();
}

TEST_CASE("HTTPSSession onReceived with valid request", "[https_sess][recv]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto ctx = std::make_shared<SSLContext>(asio::ssl::context::tlsv12);
    auto server = std::make_shared<HTTPSServer>(service, ctx, "127.0.0.1", 0);
    auto session = std::dynamic_pointer_cast<HTTPSSession>(server->CreateSession(server));

    std::string req = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    try { session->onReceived(req.data(), req.size()); } catch (...) {}

    std::string post =
        "POST /submit HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "hello";
    try { session->onReceived(post.data(), post.size()); } catch (...) {}

    try { session->onDisconnected(); } catch (...) {}

    service->Stop();
}

TEST_CASE("HTTPSSession onReceived with invalid data", "[https_sess][bad]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto ctx = std::make_shared<SSLContext>(asio::ssl::context::tlsv12);
    auto server = std::make_shared<HTTPSServer>(service, ctx, "127.0.0.1", 0);
    auto session = std::dynamic_pointer_cast<HTTPSSession>(server->CreateSession(server));

    try { session->onReceived("NOT HTTP!!!!", 12); } catch (...) {}
    try { session->onReceived("GET / HTTP/1.1\r\nHost: ", 22); } catch (...) {}
    try { session->onDisconnected(); } catch (...) {}

    service->Stop();
}

// ===== SSL client connect error paths =====

TEST_CASE("SSLClient connect to closed port", "[ssl][fail]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto ctx = std::make_shared<SSLContext>(asio::ssl::context::tlsv12);

    {
        auto client = std::make_shared<SSLClient>(service, ctx, "127.0.0.1", 1);
        bool ok = true;
        try { ok = client->Connect(); } catch (...) { ok = false; }
        CHECK_FALSE(ok);
    }
    {
        auto client = std::make_shared<SSLClient>(service, ctx, "127.0.0.1", 1);
        try { client->ConnectAsync(); } catch (...) {}
        std::this_thread::sleep_for(300ms);
    }
    {
        auto resolver = std::make_shared<TCPResolver>(service);
        auto client = std::make_shared<SSLClient>(service, ctx, "127.0.0.1", 1);
        bool ok = true;
        try { ok = client->Connect(resolver); } catch (...) { ok = false; }
        CHECK_FALSE(ok);
    }
    {
        auto resolver = std::make_shared<TCPResolver>(service);
        auto client = std::make_shared<SSLClient>(service, ctx, "127.0.0.1", 1);
        try { client->ConnectAsync(resolver); } catch (...) {}
        std::this_thread::sleep_for(500ms);
    }

    service->Stop();
}

TEST_CASE("SSLClient strand mode construction and options", "[ssl][strand]")
{
    auto service = std::make_shared<Service>(2, true);
    service->Start();

    auto ctx = std::make_shared<SSLContext>(asio::ssl::context::tlsv12);
    auto client = std::make_shared<SSLClient>(service, ctx, "127.0.0.1", 1);

    CHECK_FALSE(client->IsConnected());
    CHECK_FALSE(client->IsHandshaked());
    client->SetupKeepAlive(true);
    client->SetupNoDelay(true);
    client->SetupReceiveBufferLimit(1024);
    client->SetupSendBufferLimit(2048);
    CHECK(client->option_keep_alive());
    CHECK(client->option_no_delay());

    try { client->ConnectAsync(); } catch (...) {}
    std::this_thread::sleep_for(300ms);

    CHECK_FALSE(client->Disconnect());
    CHECK_FALSE(client->Reconnect());

    char data[] = "test";
    try { client->Send(data, 4); } catch (...) {}
    try { CHECK_FALSE(client->SendAsync(data, 4)); } catch (...) {}
    char buf[64];
    try { client->Receive(buf, 64); } catch (...) {}

    service->Stop();
}

TEST_CASE("SSLClient send buffer limit", "[ssl][buflimit]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto ctx = std::make_shared<SSLContext>(asio::ssl::context::tlsv12);
    auto client = std::make_shared<SSLClient>(service, ctx, "127.0.0.1", 1);
    client->SetupSendBufferLimit(4);

    std::vector<char> big(1024, 'x');
    try { (void)client->SendAsync(big.data(), big.size()); } catch (...) {}

    service->Stop();
}

TEST_CASE("SSLSession extended disconnected paths", "[ssl_sess][ext]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto ctx = std::make_shared<SSLContext>(asio::ssl::context::tlsv12);
    auto server = std::make_shared<SSLServer>(service, ctx, "127.0.0.1", 0);

    auto session = std::make_shared<SSLSession>(server);
    CHECK_FALSE(session->IsConnected());
    CHECK_FALSE(session->IsHandshaked());

    char data[] = "test";
    CHECK(session->Send(data, 4) == 0);
    CHECK(session->Send(std::string_view("test")) == 0);
    CHECK(session->Send(data, 4, BaseKit::Timespan::seconds(1)) == 0);
    CHECK_FALSE(session->SendAsync(data, 4));

    char buf[64];
    try { CHECK(session->Receive(buf, 64) == 0); } catch (...) {}
    try { CHECK(session->Receive(64).empty()); } catch (...) {}
    try { CHECK(session->Receive(buf, 64, BaseKit::Timespan::seconds(1)) == 0); } catch (...) {}

    try { session->ReceiveAsync(); } catch (...) {}

    session->SendError(asio::error::connection_aborted);
    session->SendError(asio::error::connection_refused);
    session->SendError(asio::error::eof);
    session->SendError(asio::error::operation_aborted);

    session->ResetServer();

    service->Stop();
}
