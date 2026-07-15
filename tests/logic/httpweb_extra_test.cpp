// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "fileclient.h"
#include "fileserver.h"
#include "tokencache.h"
#include "webbinder.h"
#include "webproto.h"
#include "syncstatus.h"

#include "asio/service.h"
#include "asio/ssl_context.h"

#include <filesystem>
#include <fstream>
#include <random>
#include <string>

class HttpWebExtraTest : public ::testing::Test {
protected:
    std::shared_ptr<NetUtil::Asio::Service> service;
    std::shared_ptr<NetUtil::Asio::SSLContext> context;
    std::unique_ptr<FileClient> client;
    std::unique_ptr<FileServer> server;
    std::filesystem::path tmpDir;

    void SetUp() override
    {
        std::random_device rd;
        tmpDir = std::filesystem::temp_directory_path() / ("hw_extra_" + std::to_string(rd()));
        std::filesystem::create_directories(tmpDir);

        WebBinder::GetInstance().clear();
        TokenCache::GetInstance().clearTokens();

        service = std::make_shared<NetUtil::Asio::Service>();
        service->Start();
        context = std::make_shared<NetUtil::Asio::SSLContext>(asio::ssl::context::tlsv12);

        client = std::make_unique<FileClient>(service, context, "127.0.0.1", 13789);
        server = std::make_unique<FileServer>(service, context, "127.0.0.1", 13790);
    }

    void TearDown() override
    {
        client.reset();
        server.reset();
        service->Stop();
        WebBinder::GetInstance().clear();
        TokenCache::GetInstance().clearTokens();
        std::error_code ec;
        std::filesystem::remove_all(tmpDir, ec);
    }
};

TEST_F(HttpWebExtraTest, WebBinderReplaceAllSingle)
{
    std::string s = "hello world";
    WebBinder::GetInstance().replaceAll(s, "world", "there");
    EXPECT_EQ(s, "hello there");
}

TEST_F(HttpWebExtraTest, WebBinderReplaceAllMultiple)
{
    std::string s = "a.b.c.d";
    WebBinder::GetInstance().replaceAll(s, ".", "-");
    EXPECT_EQ(s, "a-b-c-d");
}

TEST_F(HttpWebExtraTest, WebBinderReplaceAllEmptyFrom)
{
    std::string s = "unchanged";
    WebBinder::GetInstance().replaceAll(s, "", "X");
    EXPECT_EQ(s, "unchanged");
}

TEST_F(HttpWebExtraTest, WebBinderReplaceAllToContainsFrom)
{
    std::string s = "x";
    WebBinder::GetInstance().replaceAll(s, "x", "yx");
    EXPECT_EQ(s, "yx");
}

TEST_F(HttpWebExtraTest, WebBinderReplaceAllNoMatch)
{
    std::string s = "abcdef";
    WebBinder::GetInstance().replaceAll(s, "zzz", "q");
    EXPECT_EQ(s, "abcdef");
}

TEST_F(HttpWebExtraTest, WebBinderReplaceFound)
{
    std::string s = "foo bar baz";
    bool ok = WebBinder::GetInstance().replace(s, "bar", "qux");
    EXPECT_TRUE(ok);
    EXPECT_EQ(s, "foo qux baz");
}

TEST_F(HttpWebExtraTest, WebBinderReplaceNotFound)
{
    std::string s = "foo bar baz";
    bool ok = WebBinder::GetInstance().replace(s, "nope", "x");
    EXPECT_FALSE(ok);
    EXPECT_EQ(s, "foo bar baz");
}

TEST_F(HttpWebExtraTest, WebBinderReplaceFirstOnly)
{
    std::string s = "a.a.a";
    bool ok = WebBinder::GetInstance().replace(s, "a", "Z");
    EXPECT_TRUE(ok);
    EXPECT_EQ(s, "Z.a.a");
}

TEST_F(HttpWebExtraTest, FileClientStartFileDownloadNoConfig)
{
    client->startFileDownload({"file1.txt"});
    SUCCEED();
}

TEST_F(HttpWebExtraTest, FileClientStartFileDownloadEmptyTokenOnly)
{
    client->setConfig("", tmpDir.string());
    client->startFileDownload({"file1.txt"});
    SUCCEED();
}

TEST_F(HttpWebExtraTest, FileClientStartFileDownloadEmptySaveDirOnly)
{
    client->setConfig("sometoken", "");
    client->startFileDownload({"file1.txt"});
    SUCCEED();
}

TEST_F(HttpWebExtraTest, FileClientSendInfobyHeaderInvalidMask)
{
    client->sendInfobyHeader(INFO_WEB_MAX);
    SUCCEED();
}

TEST_F(HttpWebExtraTest, FileClientSendInfobyHeaderLargeMask)
{
    client->sendInfobyHeader(255, "name");
    SUCCEED();
}

TEST_F(HttpWebExtraTest, FileClientStopMultipleTimesNoConfig)
{
    client->stop();
    client->stop();
    client->stop();
    SUCCEED();
}

TEST_F(HttpWebExtraTest, FileClientCreateNotExistPathLongNameTruncated)
{
    std::string longName(300, 'x');
    longName += ".txt";
    std::string path = (tmpDir / longName).string();
    bool created = client->createNotExistPath(path, true);
    EXPECT_TRUE(created);
    EXPECT_FALSE(path.empty());
    EXPECT_TRUE(std::filesystem::exists(path));
    EXPECT_LT(path.length(), (tmpDir / longName).string().length());
}

TEST_F(HttpWebExtraTest, FileClientCreateNotExistPathLongNameDirTruncated)
{
    std::string longName(300, 'd');
    std::string path = (tmpDir / longName).string();
    bool created = client->createNotExistPath(path, false);
    EXPECT_TRUE(created);
    EXPECT_FALSE(path.empty());
    EXPECT_TRUE(std::filesystem::exists(path));
}

TEST_F(HttpWebExtraTest, FileClientCreateNextAvailableNameTrailingSlashSaveDir)
{
    client->setConfig("token", tmpDir.string() + "/");
    std::string name = client->createNextAvailableName("trailfile.txt", true);
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(std::filesystem::exists(name));
}

TEST_F(HttpWebExtraTest, FileClientCreateNextAvailableNameTrailingSlashDir)
{
    client->setConfig("token", tmpDir.string() + "/");
    std::string name = client->createNextAvailableName("traildir", false);
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(std::filesystem::is_directory(name));
}

TEST_F(HttpWebExtraTest, FileClientCreateNextAvailableNameMultipleDupes)
{
    client->setConfig("token", tmpDir.string());
    std::string n1 = client->createNextAvailableName("multidup.txt", true);
    ASSERT_FALSE(n1.empty());
    std::ofstream(n1) << "content one";
    std::string n2 = client->createNextAvailableName("multidup.txt", true);
    ASSERT_FALSE(n2.empty());
    std::ofstream(n2) << "content two";
    std::string n3 = client->createNextAvailableName("multidup.txt", true);
    ASSERT_FALSE(n3.empty());
    EXPECT_NE(n1, n2);
    EXPECT_NE(n2, n3);
    EXPECT_NE(n1, n3);
}

TEST_F(HttpWebExtraTest, FileClientGetHeadKeyMultipleHeaders)
{
    std::string headers =
        "Host: example.com\n"
        "Content-Type: application/json\n"
        "X-Custom: myvalue\n"
        "Content-Length: 42\n";
    std::string v = client->getHeadKey(headers, "Content-Type");
    EXPECT_TRUE(v.find("application/json") != std::string::npos);
    std::string v2 = client->getHeadKey(headers, "X-Custom");
    EXPECT_TRUE(v2.find("myvalue") != std::string::npos);
}

TEST_F(HttpWebExtraTest, FileClientGetHeadKeyNoDelimiter)
{
    std::string headers = "this line has no colon\nanother\n";
    std::string v = client->getHeadKey(headers, "anything");
    EXPECT_TRUE(v.empty());
}

TEST_F(HttpWebExtraTest, FileServerVerifyTokenInvalid)
{
    std::string bad = "not.a.valid.jwt";
    EXPECT_FALSE(server->verifyToken(bad));
}

TEST_F(HttpWebExtraTest, FileServerVerifyTokenEmpty)
{
    std::string empty;
    EXPECT_FALSE(server->verifyToken(empty));
}

TEST_F(HttpWebExtraTest, FileServerVerifyTokenGarbage)
{
    std::string garbage = "!!!garbage!!!";
    EXPECT_FALSE(server->verifyToken(garbage));
}

TEST_F(HttpWebExtraTest, FileServerVerifyTokenAfterClear)
{
    auto token = server->genToken(R"(["file1"])");
    ASSERT_FALSE(token.empty());
    server->clearBind();
    std::string mut = token;
    EXPECT_FALSE(server->verifyToken(mut));
}

TEST_F(HttpWebExtraTest, FileServerGenTokenEmptyInfo)
{
    auto token = server->genToken("[]");
    EXPECT_FALSE(token.empty());
}

TEST_F(HttpWebExtraTest, FileServerGenTokenMultipleEntries)
{
    auto token = server->genToken(R"(["a","b","c"])");
    EXPECT_FALSE(token.empty());
    auto webs = client->parseWeb(token);
    EXPECT_EQ(webs.size(), 3u);
}

TEST_F(HttpWebExtraTest, FileServerWebBindMismatchedBothDirections)
{
    EXPECT_THROW(server->webBind("/dir/", "/home/user/docs"), std::invalid_argument);
    EXPECT_THROW(server->webBind("/dir", "/home/user/docs/"), std::invalid_argument);
}

TEST_F(HttpWebExtraTest, FileServerWebUnbindAfterClear)
{
    ASSERT_EQ(server->webBind("/files", "/home/user/docs"), 0);
    server->clearBind();
    EXPECT_EQ(server->webUnbind("/files"), -1);
}

TEST_F(HttpWebExtraTest, FileServerClearBindIdempotent)
{
    server->clearBind();
    server->clearBind();
    SUCCEED();
}

TEST_F(HttpWebExtraTest, FileServerVerifyValidTokenDelegation)
{
    auto token = server->genToken(R"(["/files/a.txt"])");
    std::string mut = token;
    EXPECT_TRUE(server->verifyToken(mut));
}

TEST_F(HttpWebExtraTest, FileClientDestructorAfterSetConfig)
{
    {
        FileClient fc(service, context, "127.0.0.1", 13791);
        fc.setConfig("tk", tmpDir.string());
        fc.stop();
    }
    SUCCEED();
}
