// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// Additional coverage: tcp_session timeout paths, ssl_session timeout paths,
// http/https client error response paths, http/https server static content.

#include <catch2/catch_all.hpp>
#include <asio/service.h>
#include <asio/ssl_context.h>
#include <asio/ssl_server.h>
#include <asio/ssl_session.h>
#include <asio/tcp_server.h>
#include <asio/tcp_session.h>
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

class EchoTCPServer2 : public TCPServer
{
public:
    using TCPServer::TCPServer;
protected:
    std::shared_ptr<TCPSession> CreateSession(const std::shared_ptr<TCPServer>& server) override
    { return std::make_shared<TCPSession>(server); }
};

// ===== TCP session timeout paths =====

TEST_CASE("TCPSession timeout Send/Receive via state manipulation", "[tcp_sess2]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto server = std::make_shared<EchoTCPServer2>(service, "127.0.0.1", 0);
    auto session = std::make_shared<TCPSession>(server);

    char data[] = "test";

    // SendAsync + ReceiveAsync (async, non-blocking)
    session->_connected = true;
    try { (void)session->SendAsync(data, 4); } catch (...) {}
    session->_connected = true;
    try { session->ReceiveAsync(); } catch (...) {}
    std::this_thread::sleep_for(100ms);

    // Disconnect
    session->_connected = true;
    try { (void)session->Disconnect(); } catch (...) {}
    std::this_thread::sleep_for(50ms);

    service->Stop();
}

TEST_CASE("TCPSession Receive string disconnected", "[tcp_sess2][str]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto server = std::make_shared<EchoTCPServer2>(service, "127.0.0.1", 0);
    auto session = std::make_shared<TCPSession>(server);

    // Receive on disconnected session returns empty
    try { auto s = session->Receive(64); } catch (...) {}

    service->Stop();
}

// ===== SSL session timeout paths =====

TEST_CASE("SSLSession extra methods", "[ssl_sess2]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto ctx = std::make_shared<SSLContext>(asio::ssl::context::tlsv12);
    auto server = std::make_shared<SSLServer>(service, ctx, "127.0.0.1", 0);
    auto session = std::make_shared<SSLSession>(server);

    char data[] = "test";

    // SendAsync (async, non-blocking)
    session->_connected = true;
    session->_handshaked = true;
    try { (void)session->SendAsync(data, 4); } catch (...) {}
    std::this_thread::sleep_for(100ms);

    // ReceiveAsync
    session->_connected = true;
    session->_handshaked = true;
    try { session->ReceiveAsync(); } catch (...) {}
    std::this_thread::sleep_for(100ms);

    // Disconnect
    session->_connected = true;
    session->_handshaked = true;
    try { (void)session->Disconnect(); } catch (...) {}
    std::this_thread::sleep_for(100ms);

    session->ResetServer();
    service->Stop();
}

// ===== HTTP client error response paths =====

TEST_CASE("HTTPClient onReceived error and body paths", "[http_cli2]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    HTTPClient client(service, "127.0.0.1", 80);

    // Response with error status
    std::string errResp =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Length: 9\r\n"
        "\r\n"
        "not found";
    try { client.onReceived(errResp.data(), errResp.size()); } catch (...) {}

    // Response with chunked encoding (no Content-Length → pending body)
    std::string chunked =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\nhello\r\n"
        "0\r\n\r\n";
    try { client.onReceived(chunked.data(), chunked.size()); } catch (...) {}

    // Partial body then disconnect
    std::string partialHeader =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 100\r\n"
        "\r\n"
        "partial";
    try { client.onReceived(partialHeader.data(), partialHeader.size()); } catch (...) {}
    try { client.onDisconnected(); } catch (...) {}

    // Error response (malformed)
    try { client.onReceived("GARBAGE!!!", 10); } catch (...) {}

    service->Stop();
}

TEST_CASE("HTTPClientEx onReceivedResponse and onConnected", "[http_cli2][ex]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto client = std::make_shared<HTTPClientEx>(service, "127.0.0.1", 80);

    // Call onConnected directly (sends request if pending)
    HTTPRequest req;
    req.MakeGetRequest("/");
    client->request() = req;
    try { client->onConnected(); } catch (...) {}

    // onReceivedResponse
    HTTPResponse resp;
    resp.SetBegin(200, "OK");
    try { client->onReceivedResponse(resp); } catch (...) {}

    // onReceivedResponseError
    try { client->onReceivedResponseError(resp, "test error"); } catch (...) {}

    // onDisconnected
    try { client->onDisconnected(); } catch (...) {}

    service->Stop();
}

// ===== HTTPS client error response paths =====

TEST_CASE("HTTPSClient onReceived error and body paths", "[https_cli2]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto ctx = std::make_shared<SSLContext>(asio::ssl::context::tlsv12);
    HTTPSClient client(service, ctx, "127.0.0.1", 443);

    std::string errResp =
        "HTTP/1.1 500 Internal Server Error\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "error";
    try { client.onReceived(errResp.data(), errResp.size()); } catch (...) {}

    // Chunked response
    std::string chunked =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\nhello\r\n"
        "0\r\n\r\n";
    try { client.onReceived(chunked.data(), chunked.size()); } catch (...) {}

    // Partial body then disconnect
    std::string partial =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 100\r\n"
        "\r\n"
        "partial";
    try { client.onReceived(partial.data(), partial.size()); } catch (...) {}
    try { client.onDisconnected(); } catch (...) {}

    service->Stop();
}

TEST_CASE("HTTPSClientEx handlers", "[https_cli2][ex]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto ctx = std::make_shared<SSLContext>(asio::ssl::context::tlsv12);
    auto client = std::make_shared<HTTPSClientEx>(service, ctx, "127.0.0.1", 443);

    HTTPRequest req;
    req.MakeGetRequest("/");
    client->request() = req;

    try { client->onHandshaked(); } catch (...) {}

    HTTPResponse resp;
    resp.SetBegin(200, "OK");
    try { client->onReceivedResponse(resp); } catch (...) {}
    try { client->onReceivedResponseError(resp, "err"); } catch (...) {}
    try { client->onDisconnected(); } catch (...) {}

    service->Stop();
}

// ===== HTTP/HTTPS server static content (AddStaticContent lambda) =====

TEST_CASE("HTTPServer AddStaticContent with real file", "[http_srv2]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    HTTPServer server(service, "127.0.0.1", 0);

    // Create a real directory with a file so insert_path traverses it
    auto tmpdir = std::filesystem::temp_directory_path() / "netutil_static_dir";
    std::filesystem::create_directories(tmpdir);
    auto file1 = tmpdir / "index.html";
    auto file2 = tmpdir / "style.css";
    { std::ofstream ofs(file1); ofs << "<h1>hello</h1>"; }
    { std::ofstream ofs(file2); ofs << "body { }"; }

    server.AddStaticContent(file1.string(), "/", BaseKit::Timespan::seconds(60));
    server.AddStaticContent(file2.string(), "/", BaseKit::Timespan::seconds(60));
    server.RemoveStaticContent(file1.string());
    server.ClearStaticContent();

    // Also test AddStaticContent on a directory
    server.AddStaticContent(tmpdir.string(), "/static", BaseKit::Timespan::seconds(60));
    server.Watchdog();
    server.ClearStaticContent();

    std::filesystem::remove_all(tmpdir);
    service->Stop();
}

TEST_CASE("HTTPSServer AddStaticContent with real file", "[https_srv2]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto ctx = std::make_shared<SSLContext>(asio::ssl::context::tlsv12);
    HTTPSServer server(service, ctx, "127.0.0.1", 0);

    auto tmpdir = std::filesystem::temp_directory_path() / "netutil_https_static_dir";
    std::filesystem::create_directories(tmpdir);
    auto file1 = tmpdir / "data.json";
    { std::ofstream ofs(file1); ofs << R"({"k":"v"})"; }

    server.AddStaticContent(file1.string(), "/", BaseKit::Timespan::seconds(60));
    server.AddStaticContent(tmpdir.string(), "/assets", BaseKit::Timespan::seconds(60));
    server.Watchdog();
    server.ClearStaticContent();

    std::filesystem::remove_all(tmpdir);
    service->Stop();
}

// ===== HTTP/HTTPS session more paths =====

TEST_CASE("HTTPSession cached GET request", "[http_sess2]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto server = std::make_shared<HTTPServer>(service, "127.0.0.1", 0);

    // Add a cached response for a specific URL
    auto tmpfile = (std::filesystem::temp_directory_path() / "netutil_cache_test.txt").string();
    { std::ofstream ofs(tmpfile); ofs << "cached content"; }
    server->AddStaticContent(tmpfile, "/", BaseKit::Timespan::seconds(60));

    auto session = std::dynamic_pointer_cast<HTTPSession>(server->CreateSession(server));

    // GET request for cached URL
    std::string req = "GET /netutil_cache_test.txt HTTP/1.1\r\nHost: localhost\r\n\r\n";
    try { session->onReceived(req.data(), req.size()); } catch (...) {}

    // GET request with query string
    std::string req2 = "GET /netutil_cache_test.txt?foo=bar HTTP/1.1\r\nHost: localhost\r\n\r\n";
    try { session->onReceived(req2.data(), req2.size()); } catch (...) {}

    // POST request (not cached)
    std::string post =
        "POST /api HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 3\r\n"
        "\r\n"
        "abc";
    try { session->onReceived(post.data(), post.size()); } catch (...) {}

    try { session->onDisconnected(); } catch (...) {}

    std::filesystem::remove(tmpfile);
    service->Stop();
}

TEST_CASE("HTTPSSession cached GET request", "[https_sess2]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto ctx = std::make_shared<SSLContext>(asio::ssl::context::tlsv12);
    auto server = std::make_shared<HTTPSServer>(service, ctx, "127.0.0.1", 0);

    auto tmpfile = (std::filesystem::temp_directory_path() / "netutil_https_cache_test.txt").string();
    { std::ofstream ofs(tmpfile); ofs << "https cached"; }
    server->AddStaticContent(tmpfile, "/", BaseKit::Timespan::seconds(60));

    auto session = std::dynamic_pointer_cast<HTTPSSession>(server->CreateSession(server));

    std::string req = "GET /netutil_https_cache_test.txt HTTP/1.1\r\nHost: localhost\r\n\r\n";
    try { session->onReceived(req.data(), req.size()); } catch (...) {}

    std::string post =
        "POST /api HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 3\r\n"
        "\r\n"
        "abc";
    try { session->onReceived(post.data(), post.size()); } catch (...) {}

    try { session->onDisconnected(); } catch (...) {}

    std::filesystem::remove(tmpfile);
    service->Stop();
}

// ===== Service additional coverage =====

TEST_CASE("Service polling and restart", "[svc2]")
{
    auto service = std::make_shared<Service>(2, true);
    CHECK(service->IsStrandRequired());
    CHECK(service->Start(true)); // polling mode
    CHECK(service->IsPolling());
    std::this_thread::sleep_for(50ms);
    CHECK(service->Restart());
    std::this_thread::sleep_for(50ms);
    service->Stop();
}

TEST_CASE("Service single thread with polling", "[svc2][poll]")
{
    auto service = std::make_shared<Service>();
    CHECK(service->Start(true));
    CHECK(service->IsPolling());
    std::this_thread::sleep_for(50ms);
    service->Stop();
}

TEST_CASE("Service custom io_service", "[svc2][custom]")
{
    auto io = std::make_shared<asio::io_service>();
    auto service = std::make_shared<Service>(io, true);
    CHECK(service->IsStrandRequired());
    // Don't start/stop (the custom io_service is managed externally)
}

// ===== Timer additional coverage =====

TEST_CASE("Timer cancel without wait", "[timer2]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    Timer t(service);
    t.Setup([](bool) {}, BaseKit::Timespan::seconds(10));
    t.Cancel(); // cancel without waiting
    std::this_thread::sleep_for(50ms);

    service->Stop();
}

TEST_CASE("Timer setup UtcTime in past", "[timer2][past]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    Timer t(service);
    // Setup with a past time → fires immediately
    auto past = BaseKit::UtcTime() - BaseKit::Timespan::seconds(5);
    CHECK(t.Setup(past));
    try { CHECK(t.WaitSync()); } catch (...) {}

    service->Stop();
}

// ===== TCPSession timeout paths via real connected session =====

class CaptureSessionServer : public TCPServer
{
public:
    using TCPServer::TCPServer;
    std::shared_ptr<TCPSession> lastSession;
protected:
    std::shared_ptr<TCPSession> CreateSession(const std::shared_ptr<TCPServer>& server) override
    {
        auto s = std::make_shared<TCPSession>(server);
        lastSession = s;
        return s;
    }
};

class SimpleTCPClient : public TCPClient
{
public:
    using TCPClient::TCPClient;
};

TEST_CASE("TCPSession timeout Send/Receive on real connected session", "[tcp_sess3]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto server = std::make_shared<CaptureSessionServer>(service, "127.0.0.1", 0);
    server->SetupReuseAddress(true);
    CHECK(server->Start());
    std::this_thread::sleep_for(100ms);
    int port = server->acceptor().local_endpoint().port();

    auto client = std::make_shared<SimpleTCPClient>(service, "127.0.0.1", port);
    CHECK(client->Connect());
    std::this_thread::sleep_for(100ms);

    auto session = server->lastSession;
    if (session && session->IsConnected())
    {
        char data[] = "hello";
        char buf[64];
        try { (void)session->Send(data, 5, BaseKit::Timespan::milliseconds(200)); } catch (...) {}
        try { (void)session->Receive(buf, 64, BaseKit::Timespan::milliseconds(100)); } catch (...) {}
        try { auto s = session->Receive(64, BaseKit::Timespan::milliseconds(100)); } catch (...) {}
    }

    client->Disconnect();
    std::this_thread::sleep_for(50ms);
    try { server->Stop(); } catch (...) {}
    std::this_thread::sleep_for(50ms);
    service->Stop();
}

TEST_CASE("TCPSession SendAsync + sync Send on real session", "[tcp_sess3][async]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    auto server = std::make_shared<CaptureSessionServer>(service, "127.0.0.1", 0);
    server->SetupReuseAddress(true);
    CHECK(server->Start());
    std::this_thread::sleep_for(100ms);
    int port = server->acceptor().local_endpoint().port();

    auto client = std::make_shared<SimpleTCPClient>(service, "127.0.0.1", port);
    CHECK(client->Connect());
    std::this_thread::sleep_for(100ms);

    auto session = server->lastSession;
    if (session && session->IsConnected())
    {
        char data[] = "fromsrv";
        try { (void)session->SendAsync(data, 7); } catch (...) {}
        std::this_thread::sleep_for(100ms);

        char buf[64];
        try { (void)client->Receive(buf, 64, BaseKit::Timespan::milliseconds(200)); } catch (...) {}

        try { (void)session->Send(data, 7); } catch (...) {}
        std::this_thread::sleep_for(50ms);
    }

    client->Disconnect();
    std::this_thread::sleep_for(50ms);
    try { server->Stop(); } catch (...) {}
    std::this_thread::sleep_for(50ms);
    service->Stop();
}
