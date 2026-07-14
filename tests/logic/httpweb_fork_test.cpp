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

    void SetUp() override
    {
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

        server = std::make_shared<FileServer>(service, srvCtx, "127.0.0.1", 18456);
    }

    void TearDown() override
    {
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
        bool onProgress(uint64_t) override { return false; }
        void onWebChanged(int state, std::string, uint64_t) override {
            if (state == WEB_TRANS_FINISH || state == WEB_DISCONNECTED || state == WEB_IO_ERROR) {
                try { done.set_value(); } catch (...) {}
            }
        }
    };

    auto cb = std::make_shared<WaitCallback>();
    FileClient client(service, cliCtx, "127.0.0.1", 18456);
    client.setCallback(cb);
    std::string token = server->genToken(R"(["/files/test.txt"])");
    client.setConfig(token, cliDir.string());
    client.startFileDownload({"/files/test.txt"});

    auto fut = cb->done.get_future();
    ASSERT_EQ(fut.wait_for(std::chrono::seconds(10)), std::future_status::ready);
    client.stop();

    SUCCEED();
}
