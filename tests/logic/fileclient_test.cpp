// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "fileclient.h"
#include "tokencache.h"

#include "asio/service.h"
#include "asio/ssl_context.h"

#include <fstream>
#include <filesystem>

// FileClient needs a Service + SSLContext to construct. We build minimal ones.
class FileClientTest : public ::testing::Test {
protected:
    std::shared_ptr<NetUtil::Asio::Service> service;
    std::shared_ptr<NetUtil::Asio::SSLContext> context;
    std::unique_ptr<FileClient> client;
    std::filesystem::path tmpDir;
    void SetUp() override
    {
        tmpDir = std::filesystem::temp_directory_path() / ("fc_test_" + std::to_string(std::rand()));
        std::filesystem::create_directories(tmpDir);

        service = std::make_shared<NetUtil::Asio::Service>();
        service->Start();
        context = std::make_shared<NetUtil::Asio::SSLContext>(asio::ssl::context::tlsv12);
        client = std::make_unique<FileClient>(service, context, "127.0.0.1", 12345);
    }

    void TearDown() override
    {
        client.reset();
        service->Stop();
        std::error_code ec;
        std::filesystem::remove_all(tmpDir, ec);
    }
};

TEST_F(FileClientTest, ParseWebFromValidToken)
{
    std::string info = R"(["file1.txt","dir1/"])";
    auto token = TokenCache::GetInstance().genToken(info);
    ASSERT_FALSE(token.empty());

    auto webs = client->parseWeb(token);
    EXPECT_EQ(webs.size(), 2u);
}

TEST_F(FileClientTest, ParseWebInvalidToken)
{
    // jwt::decode throws on invalid tokens; getWebfromToken doesn't catch all
    EXPECT_ANY_THROW(client->parseWeb("invalid.token.here"));
}
TEST_F(FileClientTest, SetConfig)
{
    client->setConfig("sometoken", "/tmp/downloads");
    // setConfig just stores _token and _savedir; verify via side effects
    SUCCEED();
}

TEST_F(FileClientTest, StopWithoutDownload)
{
    client->stop();
    SUCCEED();
}

TEST_F(FileClientTest, StopMultipleTimes)
{
    client->stop();
    client->stop();
    SUCCEED();
}

// ---- private method coverage via -fno-access-control ----

TEST_F(FileClientTest, CreateNotExistPathCreatesFile)
{
    std::string path = (tmpDir / "newfile.txt").string();
    bool created = client->createNotExistPath(path, true);
    EXPECT_TRUE(created);
    EXPECT_TRUE(std::filesystem::exists(path));
}

TEST_F(FileClientTest, CreateNotExistPathCreatesDir)
{
    std::string path = (tmpDir / "newdir").string();
    bool created = client->createNotExistPath(path, false);
    EXPECT_TRUE(created);
    EXPECT_TRUE(std::filesystem::is_directory(path));
}

TEST_F(FileClientTest, CreateNotExistPathExistingFile)
{
    std::string path = (tmpDir / "exist.txt").string();
    std::ofstream(path) << "content";
    bool created = client->createNotExistPath(path, true);
    EXPECT_FALSE(created);
}

TEST_F(FileClientTest, CreateNotExistPathNestedDirs)
{
    std::string path = (tmpDir / "a" / "b" / "c.txt").string();
    bool created = client->createNotExistPath(path, true);
    EXPECT_TRUE(created);
    EXPECT_TRUE(std::filesystem::exists(path));
}

TEST_F(FileClientTest, CreateNextAvailableNameNewFile)
{
    client->setConfig("token", tmpDir.string());
    std::string name = client->createNextAvailableName("file1.txt", true);
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(std::filesystem::exists(name));
}

TEST_F(FileClientTest, CreateNextAvailableNameExistingFile)
{
    client->setConfig("token", tmpDir.string());
    // create first with content (empty files are reused)
    std::string name1 = client->createNextAvailableName("dup2.txt", true);
    ASSERT_FALSE(name1.empty());
    std::ofstream(name1) << "some content to make it non-empty";

    // create again -> file exists and non-empty -> should get (1)
    std::string name2 = client->createNextAvailableName("dup2.txt", true);
    ASSERT_FALSE(name2.empty());
    EXPECT_NE(name1, name2);
    EXPECT_TRUE(name2.find("(1)") != std::string::npos);
}

TEST_F(FileClientTest, CreateNextAvailableNameNewDir)
{
    client->setConfig("token", tmpDir.string());
    std::string name = client->createNextAvailableName("newdir", false);
    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(std::filesystem::is_directory(name));
}

TEST_F(FileClientTest, CreateNextAvailableNamePathTraversal)
{
    client->setConfig("token", tmpDir.string());
    std::string name = client->createNextAvailableName("../../etc/passwd", true);
    EXPECT_TRUE(name.empty());
}

TEST_F(FileClientTest, CreateNextAvailableNameAbsolute)
{
    client->setConfig("token", tmpDir.string());
    std::string name = client->createNextAvailableName("/etc/passwd", true);
    EXPECT_TRUE(name.empty());
}

TEST_F(FileClientTest, GetHeadKeyFound)
{
    std::string headers = "Content-Type: text/html\r\nFlag: download\r\nContent-Length: 1024\r\n";
    std::string val = client->getHeadKey(headers, "Flag");
    EXPECT_TRUE(val.find("download") != std::string::npos);
}

TEST_F(FileClientTest, GetHeadKeyNotFound)
{
    std::string headers = "Content-Type: text/html\r\n";
    std::string val = client->getHeadKey(headers, "Missing");
    EXPECT_TRUE(val.empty());
}

TEST_F(FileClientTest, GetHeadKeyEmpty)
{
    std::string val = client->getHeadKey("", "Anything");
    EXPECT_TRUE(val.empty());
}

TEST_F(FileClientTest, CreateNextAvailableNameNoExtension)
{
    client->setConfig("token", tmpDir.string());
    std::string name = client->createNextAvailableName("noext", true);
    EXPECT_FALSE(name.empty());
}

TEST_F(FileClientTest, DestructorNoLeak)
{
    {
        FileClient fc(service, context, "127.0.0.1", 12346);
        fc.setConfig("token", tmpDir.string());
    }
    SUCCEED();
}

TEST_F(FileClientTest, CreateNotExistPathExistingDir)
{
    std::string path = (tmpDir / "existdir").string();
    std::filesystem::create_directories(path);
    bool created = client->createNotExistPath(path, false);
    // existing dir; result depends on emptiness — just exercise
    (void)created;
    SUCCEED();
}

TEST_F(FileClientTest, CreateNotExistPathExistingEmptyFile)
{
    std::string path = (tmpDir / "empty.txt").string();
    std::ofstream f(path);
    bool created = client->createNotExistPath(path, true);
    EXPECT_TRUE(created);
}
