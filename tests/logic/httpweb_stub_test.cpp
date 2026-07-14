// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// Network-layer stub tests for httpweb: stub HTTPSClientEx::SendRequest
// (non-virtual) to return a mock response immediately, bypassing all
// SSL/network IO. This exercises FileClient's sendInfobyHeader, requestInfo,
// walkFolderEntry paths + HTTPFileClient's setResponseHandler.

#include <gtest/gtest.h>

#include "fileclient.h"

#include "tokencache.h"
#include "webbinder.h"
#include "webproto.h"
#include "syncstatus.h"

#include "asio/service.h"
#include "asio/ssl_context.h"
#include "http/https_client.h"

#include "stub.h"

#include <filesystem>
#include <future>

// Stub HTTPSClientEx::SendRequest to return immediately without network IO.
static std::future<NetUtil::HTTP::HTTPResponse> fakeSendRequest(
    const NetUtil::HTTP::HTTPRequest &request, const BaseKit::Timespan &timeout)
{
    std::promise<NetUtil::HTTP::HTTPResponse> p;
    NetUtil::HTTP::HTTPResponse resp;
    resp.SetBegin(404);
    p.set_value(std::move(resp));
    return p.get_future();
}

class FileClientStubTest : public ::testing::Test {
protected:
    std::shared_ptr<NetUtil::Asio::Service> service;
    std::shared_ptr<NetUtil::Asio::SSLContext> context;
    std::unique_ptr<FileClient> client;
    std::filesystem::path tmpDir;
    Stub stub;

    void SetUp() override
    {
        tmpDir = std::filesystem::temp_directory_path() / ("fc_stub_" + std::to_string(std::rand()));
        std::filesystem::create_directories(tmpDir);
        service = std::make_shared<NetUtil::Asio::Service>();
        service->Start();
        context = std::make_shared<NetUtil::Asio::SSLContext>(asio::ssl::context::tlsv12);
        client = std::make_unique<FileClient>(service, context, "127.0.0.1", 12399);
        client->setConfig("dummy_token", tmpDir.string());
        // Stub SendRequest globally for all tests in this suite
        stub.set((std::future<NetUtil::HTTP::HTTPResponse>(NetUtil::HTTP::HTTPSClientEx::*)(const NetUtil::HTTP::HTTPRequest&, const BaseKit::Timespan&))&NetUtil::HTTP::HTTPSClientEx::SendRequest, fakeSendRequest);
    }

    void TearDown() override
    {
        stub.reset((std::future<NetUtil::HTTP::HTTPResponse>(NetUtil::HTTP::HTTPSClientEx::*)(const NetUtil::HTTP::HTTPRequest&, const BaseKit::Timespan&))&NetUtil::HTTP::HTTPSClientEx::SendRequest);
        service->Stop();
        std::error_code ec;
        std::filesystem::remove_all(tmpDir, ec);
    }
};

TEST_F(FileClientStubTest, SendInfobyHeaderStart)
{
    client->sendInfobyHeader(INFO_WEB_START);
    SUCCEED();
}

TEST_F(FileClientStubTest, SendInfobyHeaderIndex)
{
    client->sendInfobyHeader(INFO_WEB_INDEX, "testfile.txt");
    SUCCEED();
}

TEST_F(FileClientStubTest, SendInfobyHeaderFinish)
{
    client->sendInfobyHeader(INFO_WEB_FINISH);
    SUCCEED();
}

TEST_F(FileClientStubTest, RequestInfoStubbed)
{
    auto info = client->requestInfo("testfile");
    EXPECT_EQ(info.name, "");
}

TEST_F(FileClientStubTest, WalkFolderEntryStubbed)
{
    std::queue<std::string> q;
    client->walkFolderEntry("subfolder", &q);
    EXPECT_TRUE(q.empty());
}

TEST_F(FileClientStubTest, HttpClientNotNull)
{
    EXPECT_NE(client->_httpClient, nullptr);
}

TEST_F(FileClientStubTest, DownloadFileStubbed)
{
    bool result = client->downloadFile("test.txt");
    SUCCEED();
}

TEST_F(FileClientStubTest, DownloadFileWithRenameStubbed)
{
    bool result = client->downloadFile("file.txt", "renamed.txt");
    SUCCEED();
}

TEST_F(FileClientStubTest, WalkDownloadStubbed)
{
    client->walkDownload({"file1.txt"});
    SUCCEED();
}

TEST_F(FileClientStubTest, WalkFolderStubbed)
{
    client->walkFolder("testdir");
    SUCCEED();
}
