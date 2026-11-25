// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "fileserver.h"
#include "fileclient.h"
#include "tokencache.h"
#include "webbinder.h"
#include "webproto.h"

#include "asio/service.h"
#include "asio/ssl_context.h"
#include "sslcertconf.h"

#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <future>

class HttpWebIntegrationTest : public ::testing::Test {
protected:
    std::shared_ptr<NetUtil::Asio::Service> service;
    std::shared_ptr<NetUtil::Asio::SSLContext> srvCtx;
    std::shared_ptr<FileServer> server;
    std::filesystem::path srvDir;
    std::filesystem::path cliDir;
    int port {0};

    void SetUp() override
    {
        // SO_REUSEPORT lets a lingering listener from the previous test share
        // the port, so each test instance must use its own port.
        static std::atomic<int> nextPort {18500};
        port = nextPort.fetch_add(1);

        srvDir = std::filesystem::temp_directory_path() / "hw_srv_int";
        cliDir = std::filesystem::temp_directory_path() / "hw_cli_int";
        std::filesystem::remove_all(srvDir);
        std::filesystem::remove_all(cliDir);
        std::filesystem::create_directories(srvDir);
        std::filesystem::create_directories(cliDir);

        WebBinder::GetInstance().clear();
        TokenCache::GetInstance().clearTokens();

        std::string testData = "Hello HTTPWeb!";
        std::ofstream(srvDir / "test.txt") << testData;

        SslCertConf::ins()->generateCertificate(srvDir.string());
        std::string certPath = srvDir.string() + "/SSL/Barrier.pem";

        service = std::make_shared<NetUtil::Asio::Service>();
        service->Start();

        srvCtx = std::make_shared<NetUtil::Asio::SSLContext>(asio::ssl::context::tlsv12);
        srvCtx->use_certificate_chain_file(certPath);
        srvCtx->use_private_key_file(certPath, asio::ssl::context::pem);

        server = std::make_shared<FileServer>(service, srvCtx, "127.0.0.1", port);
    }

    void TearDown() override
    {
        ::setenv("DDE_COOPERATION_HTTPWEB_TOKEN_DTP", "", 1);
        ::setenv("DDE_COOPERATION_HTTPWEB_STREAM", "", 1);
        ::setenv("DDE_COOPERATION_HTTPWEB_SERVER_STREAM", "", 1);
        if (server && server->IsStarted()) server->stop();
        service->Stop();
        server.reset();
        WebBinder::GetInstance().clear();
        TokenCache::GetInstance().clearTokens();
        std::error_code ec;
        std::filesystem::remove_all(srvDir, ec);
        std::filesystem::remove_all(cliDir, ec);
    }
};

TEST_F(HttpWebIntegrationTest, ServerStartAndBind)
{
    EXPECT_NO_THROW(server->webBind("/files/", srvDir.string() + "/"));
    bool ok = server->start();
    EXPECT_TRUE(ok);
}

TEST_F(HttpWebIntegrationTest, FullDownloadRoundTrip)
{
    server->webBind("/files/", srvDir.string() + "/");
    ASSERT_TRUE(server->start());

    auto cliCtx = std::make_shared<NetUtil::Asio::SSLContext>(asio::ssl::context::tlsv12);
    cliCtx->set_verify_mode(asio::ssl::verify_none);
    try { cliCtx->load_verify_file((srvDir / "SSL" / "Barrier.pem").string()); } catch (...) {}

    class WaitCallback : public ProgressCallInterface {
    public:
        std::promise<void> done;
        std::atomic<int> lastState {0};
        bool onProgress(uint64_t) override { return false; }
        void onWebChanged(int state, std::string, uint64_t) override {
            lastState.store(state);
            if (state == WEB_TRANS_FINISH || state == WEB_DISCONNECTED || state == WEB_IO_ERROR) {
                try { done.set_value(); } catch (...) {}
            }
        }
    };

    auto cb = std::make_shared<WaitCallback>();
    FileClient client(service, cliCtx, "127.0.0.1", port);
    client.setCallback(cb);
    std::string token = server->genToken(R"(["/files/test.txt"])");
    client.setConfig(token, cliDir.string());
    client.startFileDownload({"/files/test.txt"});

    auto fut = cb->done.get_future();
    ASSERT_EQ(fut.wait_for(std::chrono::seconds(10)), std::future_status::ready);
    client.stop();

    // A disconnect used to count as success here, hiding a broken receiver.
    EXPECT_EQ(cb->lastState.load(), WEB_TRANS_FINISH);
    ASSERT_TRUE(std::filesystem::exists(cliDir / "test.txt"));
    std::ifstream in(cliDir / "test.txt", std::ios::binary);
    const std::string content((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());
    EXPECT_EQ(content, "Hello HTTPWeb!");
}

#ifndef _WIN32
TEST_F(HttpWebIntegrationTest, StreamRestoresInRootSymlink)
{
    std::filesystem::create_directories(srvDir / "linker");
    std::ofstream(srvDir / "linker" / "real.txt") << "payload";
    std::error_code ec;
    std::filesystem::create_symlink(srvDir / "linker" / "real.txt",
                                    srvDir / "linker" / "link.txt", ec);
    ASSERT_FALSE(ec) << ec.message();

    server->webBind("/files/", srvDir.string() + "/");
    ASSERT_TRUE(server->start());

    auto cliCtx = std::make_shared<NetUtil::Asio::SSLContext>(asio::ssl::context::tlsv12);
    cliCtx->set_verify_mode(asio::ssl::verify_none);
    try { cliCtx->load_verify_file((srvDir / "SSL" / "Barrier.pem").string()); } catch (...) {}

    class WaitCallback : public ProgressCallInterface {
    public:
        std::promise<void> done;
        std::atomic<int> lastState {0};
        bool onProgress(uint64_t) override { return false; }
        void onWebChanged(int state, std::string, uint64_t) override {
            lastState.store(state);
            if (state == WEB_TRANS_FINISH || state == WEB_DISCONNECTED || state == WEB_IO_ERROR) {
                try { done.set_value(); } catch (...) {}
            }
        }
    };

    auto cb = std::make_shared<WaitCallback>();
    FileClient client(service, cliCtx, "127.0.0.1", port);
    client.setCallback(cb);
    const std::string name = "/files/linker";
    client.setConfig(server->genToken(R"(["/files/linker"])"), cliDir.string());
    client.startFileDownload({name});

    auto fut = cb->done.get_future();
    ASSERT_EQ(fut.wait_for(std::chrono::seconds(10)), std::future_status::ready);
    client.stop();

    EXPECT_EQ(cb->lastState.load(), WEB_TRANS_FINISH);
    const std::filesystem::path out = cliDir / "linker";
    ASSERT_TRUE(std::filesystem::is_symlink(out / "link.txt"));
    EXPECT_EQ(std::filesystem::read_symlink(out / "link.txt").generic_u8string(), "real.txt");
    std::ifstream in(out / "real.txt", std::ios::binary);
    const std::string content((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());
    EXPECT_EQ(content, "payload");
}
#endif

// Old peers issue tokens without the `dtp` claim. A new client must go straight
// to the legacy per-file protocol instead of paying a failed stream request.
TEST_F(HttpWebIntegrationTest, LegacyTokenUsesLegacyProtocol)
{
    std::ofstream(srvDir / "second.txt") << "second";
    server->webBind("files", srvDir.string());
    ASSERT_TRUE(server->start());

    auto cliCtx = std::make_shared<NetUtil::Asio::SSLContext>(asio::ssl::context::tlsv12);
    cliCtx->set_verify_mode(asio::ssl::verify_none);
    try { cliCtx->load_verify_file((srvDir / "SSL" / "Barrier.pem").string()); } catch (...) {}

    class WaitCallback : public ProgressCallInterface {
    public:
        std::promise<void> done;
        std::atomic<int> lastState {0};
        bool onProgress(uint64_t) override { return false; }
        void onWebChanged(int state, std::string, uint64_t) override {
            lastState.store(state);
            if (state == WEB_TRANS_FINISH || state == WEB_DISCONNECTED || state == WEB_IO_ERROR) {
                try { done.set_value(); } catch (...) {}
            }
        }
    };

    // Simulate a pre-DTP2 token: no `dtp` claim. The relative web name keeps
    // the legacy path resolver inside the save directory.
    ::setenv("DDE_COOPERATION_HTTPWEB_TOKEN_DTP", "0", 1);
    const std::string token = server->genToken(R"(["files"])");
    ::setenv("DDE_COOPERATION_HTTPWEB_TOKEN_DTP", "", 1);

    auto cb = std::make_shared<WaitCallback>();
    FileClient client(service, cliCtx, "127.0.0.1", port);
    client.setCallback(cb);
    client.setConfig(token, cliDir.string());
    client.startFileDownload({"files"});

    auto fut = cb->done.get_future();
    ASSERT_EQ(fut.wait_for(std::chrono::seconds(10)), std::future_status::ready);
    client.stop();

    EXPECT_EQ(cb->lastState.load(), WEB_TRANS_FINISH);
    const auto counters = client.requestCounters();
    // Legacy needs a directory listing and one request per file; the stream
    // path would have used one download request and zero info requests.
    EXPECT_GE(counters.infoRequests, 1u);
    EXPECT_GE(counters.downloadRequests, 2u);
    EXPECT_TRUE(std::filesystem::exists(cliDir / "files" / "test.txt"));
    EXPECT_TRUE(std::filesystem::exists(cliDir / "files" / "second.txt"));
}

// A pre-DTP2 client ignores the `dtp` claim and only knows info/download.
// `DDE_COOPERATION_HTTPWEB_STREAM=0` reproduces that behavior against a new
// server, proving the legacy endpoints stay usable during rollout.
TEST_F(HttpWebIntegrationTest, StreamDisabledByEnvUsesLegacyProtocol)
{
    std::ofstream(srvDir / "second.txt") << "second";
    server->webBind("files", srvDir.string());
    ASSERT_TRUE(server->start());

    auto cliCtx = std::make_shared<NetUtil::Asio::SSLContext>(asio::ssl::context::tlsv12);
    cliCtx->set_verify_mode(asio::ssl::verify_none);
    try { cliCtx->load_verify_file((srvDir / "SSL" / "Barrier.pem").string()); } catch (...) {}

    class WaitCallback : public ProgressCallInterface {
    public:
        std::promise<void> done;
        std::atomic<int> lastState {0};
        bool onProgress(uint64_t) override { return false; }
        void onWebChanged(int state, std::string, uint64_t) override {
            lastState.store(state);
            if (state == WEB_TRANS_FINISH || state == WEB_DISCONNECTED || state == WEB_IO_ERROR) {
                try { done.set_value(); } catch (...) {}
            }
        }
    };

    const std::string token = server->genToken(R"(["files"])");

    auto cb = std::make_shared<WaitCallback>();
    FileClient client(service, cliCtx, "127.0.0.1", port);
    client.setCallback(cb);
    client.setConfig(token, cliDir.string());
    ::setenv("DDE_COOPERATION_HTTPWEB_STREAM", "0", 1);
    client.startFileDownload({"files"});

    auto fut = cb->done.get_future();
    ASSERT_EQ(fut.wait_for(std::chrono::seconds(10)), std::future_status::ready);
    ::setenv("DDE_COOPERATION_HTTPWEB_STREAM", "", 1);
    client.stop();

    EXPECT_EQ(cb->lastState.load(), WEB_TRANS_FINISH);
    const auto counters = client.requestCounters();
    EXPECT_GE(counters.infoRequests, 1u);
    EXPECT_GE(counters.downloadRequests, 2u);
    EXPECT_TRUE(std::filesystem::exists(cliDir / "files" / "test.txt"));
    EXPECT_TRUE(std::filesystem::exists(cliDir / "files" / "second.txt"));
}

TEST_F(HttpWebIntegrationTest, ServerStreamDisabledTokenOmitsDtpClaim)
{
    ::setenv("DDE_COOPERATION_HTTPWEB_SERVER_STREAM", "0", 1);
    const std::string token = server->genToken(R"(["files"])");
    ::setenv("DDE_COOPERATION_HTTPWEB_SERVER_STREAM", "", 1);

    ASSERT_FALSE(token.empty());
    EXPECT_FALSE(TokenCache::GetInstance().getDtpFromToken(token).present);
}

// New client, new token, but the server's stream endpoint is switched off.
// The client must treat the 404 as "peer cannot stream" and finish via legacy.
TEST_F(HttpWebIntegrationTest, ServerStreamDisabledFallsBackToLegacy)
{
    std::ofstream(srvDir / "second.txt") << "second";
    server->webBind("files", srvDir.string());
    ASSERT_TRUE(server->start());

    auto cliCtx = std::make_shared<NetUtil::Asio::SSLContext>(asio::ssl::context::tlsv12);
    cliCtx->set_verify_mode(asio::ssl::verify_none);
    try { cliCtx->load_verify_file((srvDir / "SSL" / "Barrier.pem").string()); } catch (...) {}

    class WaitCallback : public ProgressCallInterface {
    public:
        std::promise<void> done;
        std::atomic<int> lastState {0};
        bool onProgress(uint64_t) override { return false; }
        void onWebChanged(int state, std::string, uint64_t) override {
            lastState.store(state);
            if (state == WEB_TRANS_FINISH || state == WEB_DISCONNECTED || state == WEB_IO_ERROR) {
                try { done.set_value(); } catch (...) {}
            }
        }
    };

    const std::string token = server->genToken(R"(["files"])");

    auto cb = std::make_shared<WaitCallback>();
    FileClient client(service, cliCtx, "127.0.0.1", port);
    client.setCallback(cb);
    client.setConfig(token, cliDir.string());
    // Server-side kill switch: stream/ returns 404.
    ::setenv("DDE_COOPERATION_HTTPWEB_SERVER_STREAM", "0", 1);
    client.startFileDownload({"files"});

    auto fut = cb->done.get_future();
    ASSERT_EQ(fut.wait_for(std::chrono::seconds(10)), std::future_status::ready);
    ::setenv("DDE_COOPERATION_HTTPWEB_SERVER_STREAM", "", 1);
    client.stop();

    EXPECT_EQ(cb->lastState.load(), WEB_TRANS_FINISH);
    const auto counters = client.requestCounters();
    EXPECT_GE(counters.infoRequests, 1u);
    EXPECT_GE(counters.downloadRequests, 2u);
    EXPECT_TRUE(std::filesystem::exists(cliDir / "files" / "test.txt"));
    EXPECT_TRUE(std::filesystem::exists(cliDir / "files" / "second.txt"));
}
