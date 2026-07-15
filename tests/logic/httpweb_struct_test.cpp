// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "webproto.h"
#include "webbinder.h"
#include "tokencache.h"
#include "fileclient.h"
#include "fileserver.h"

#include "asio/service.h"
#include "asio/ssl_context.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <string>

namespace {

InfoEntry roundtripInfo(const InfoEntry& src)
{
    picojson::value j = src.as_json();
    std::string s;
    j.serialize(std::back_inserter(s));
    picojson::value parsed;
    std::string err = picojson::parse(parsed, s);
    EXPECT_TRUE(err.empty()) << err;
    InfoEntry dst;
    dst.from_json(parsed);
    return dst;
}

}

TEST(InfoEntryTest, FlatEntryRoundtrip)
{
    InfoEntry src;
    src.name = "flat.txt";
    src.size = 42;
    auto dst = roundtripInfo(src);
    EXPECT_EQ(dst.name, "flat.txt");
    EXPECT_EQ(dst.size, 42);
    EXPECT_TRUE(dst.datas.empty());
}

TEST(InfoEntryTest, NestedEntryRoundtrip)
{
    InfoEntry src;
    src.name = "dir";
    src.size = 0;
    InfoEntry child1;
    child1.name = "a.txt";
    child1.size = 10;
    InfoEntry child2;
    child2.name = "b.txt";
    child2.size = 20;
    src.datas.push_back(child1);
    src.datas.push_back(child2);

    auto dst = roundtripInfo(src);
    EXPECT_EQ(dst.name, "dir");
    EXPECT_EQ(dst.datas.size(), 2u);
    EXPECT_EQ(dst.datas[0].name, "a.txt");
    EXPECT_EQ(dst.datas[0].size, 10);
    EXPECT_EQ(dst.datas[1].name, "b.txt");
    EXPECT_EQ(dst.datas[1].size, 20);
}

TEST(InfoEntryTest, DeeplyNestedEntryRoundtrip)
{
    InfoEntry src;
    src.name = "root";
    InfoEntry mid;
    mid.name = "mid";
    InfoEntry leaf;
    leaf.name = "leaf.dat";
    leaf.size = 7;
    mid.datas.push_back(leaf);
    src.datas.push_back(mid);

    auto dst = roundtripInfo(src);
    ASSERT_EQ(dst.datas.size(), 1u);
    EXPECT_EQ(dst.datas[0].name, "mid");
    ASSERT_EQ(dst.datas[0].datas.size(), 1u);
    EXPECT_EQ(dst.datas[0].datas[0].name, "leaf.dat");
    EXPECT_EQ(dst.datas[0].datas[0].size, 7);
}

TEST(InfoEntryTest, FromJsonDatasNotArraySkips)
{
    std::string json = R"({"name":"x","size":1,"datas":"not-an-array"})";
    picojson::value v;
    std::string err = picojson::parse(v, json);
    ASSERT_TRUE(err.empty()) << err;
    InfoEntry dst;
    dst.from_json(v);
    EXPECT_EQ(dst.name, "x");
    EXPECT_EQ(dst.size, 1);
    EXPECT_TRUE(dst.datas.empty());
}

TEST(InfoEntryTest, FromJsonNonObjectElementSkipped)
{
    std::string json = R"({"name":"p","size":2,"datas":[42,"str",{"name":"ok","size":3}]})";
    picojson::value v;
    std::string err = picojson::parse(v, json);
    ASSERT_TRUE(err.empty()) << err;
    InfoEntry dst;
    dst.from_json(v);
    EXPECT_EQ(dst.name, "p");
    ASSERT_EQ(dst.datas.size(), 1u);
    EXPECT_EQ(dst.datas[0].name, "ok");
    EXPECT_EQ(dst.datas[0].size, 3);
}

TEST(InfoEntryTest, AsJsonProducesObjectString)
{
    InfoEntry src;
    src.name = "serialize.txt";
    src.size = 99;
    std::string s;
    src.as_json().serialize(std::back_inserter(s));
    EXPECT_NE(s.find("serialize.txt"), std::string::npos);
    EXPECT_NE(s.find("\"size\""), std::string::npos);
    EXPECT_NE(s.find("\"datas\""), std::string::npos);
}

class HttpWebStructFixture : public ::testing::Test {
protected:
    std::shared_ptr<NetUtil::Asio::Service> service;
    std::shared_ptr<NetUtil::Asio::SSLContext> context;
    std::unique_ptr<FileClient> client;
    std::unique_ptr<FileServer> server;
    std::filesystem::path tmpDir;

    void SetUp() override
    {
        std::random_device rd;
        tmpDir = std::filesystem::temp_directory_path() / ("hw_struct_" + std::to_string(rd()));
        std::filesystem::create_directories(tmpDir);
        WebBinder::GetInstance().clear();
        TokenCache::GetInstance().clearTokens();
        service = std::make_shared<NetUtil::Asio::Service>();
        service->Start();
        context = std::make_shared<NetUtil::Asio::SSLContext>(asio::ssl::context::tlsv12);
        client = std::make_unique<FileClient>(service, context, "127.0.0.1", 13793);
        server = std::make_unique<FileServer>(service, context, "127.0.0.1", 13794);
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

TEST_F(HttpWebStructFixture, WebBinderBindInsertsAtFront)
{
    ASSERT_EQ(WebBinder::GetInstance().bind("/a", "/home/a"), 0);
    ASSERT_EQ(WebBinder::GetInstance().bind("/b", "/home/b"), 0);
    EXPECT_EQ(WebBinder::GetInstance().getPath("/b"), "/home/b");
    EXPECT_EQ(WebBinder::GetInstance().containWeb("/a"), true);
    EXPECT_EQ(WebBinder::GetInstance().lastWeb("/a"), true);
}

TEST_F(HttpWebStructFixture, WebBinderGetPathPartialPrefixMatch)
{
    ASSERT_EQ(WebBinder::GetInstance().bind("/files", "/home/docs"), 0);
    EXPECT_EQ(WebBinder::GetInstance().getPath("/filesX"), "/home/docsX");
}

TEST_F(HttpWebStructFixture, WebBinderUnbindMiddleKeepsOthers)
{
    ASSERT_EQ(WebBinder::GetInstance().bind("/a", "/home/a"), 0);
    ASSERT_EQ(WebBinder::GetInstance().bind("/b", "/home/b"), 0);
    ASSERT_EQ(WebBinder::GetInstance().bind("/c", "/home/c"), 0);
    EXPECT_EQ(WebBinder::GetInstance().unbind("/b"), 0);
    EXPECT_TRUE(WebBinder::GetInstance().containWeb("/a"));
    EXPECT_TRUE(WebBinder::GetInstance().containWeb("/c"));
    EXPECT_FALSE(WebBinder::GetInstance().containWeb("/b"));
}

TEST_F(HttpWebStructFixture, FileClientGetHeadKeyMixedDelimiters)
{
    std::string headers =
        "Key-A: val1\n"
        "Key-B: val2\r\n"
        "Key-C:val3\n";
    EXPECT_TRUE(client->getHeadKey(headers, "Key-A").find("val1") != std::string::npos);
    EXPECT_TRUE(client->getHeadKey(headers, "Key-B").find("val2") != std::string::npos);
}

TEST_F(HttpWebStructFixture, FileClientGetHeadKeyLastLineWins)
{
    std::string headers = "X: first\nX: second\n";
    std::string v = client->getHeadKey(headers, "X");
    EXPECT_TRUE(v.find("second") != std::string::npos);
}

TEST_F(HttpWebStructFixture, FileClientCreateNotExistPathEmptyString)
{
    std::string empty;
    bool created = client->createNotExistPath(empty, true);
    EXPECT_FALSE(created);
}

TEST_F(HttpWebStructFixture, FileClientCreateNextAvailableNameDirThenFileReuse)
{
    client->setConfig("token", tmpDir.string());
    std::string n = client->createNextAvailableName("share", false);
    EXPECT_FALSE(n.empty());
    EXPECT_TRUE(std::filesystem::is_directory(n));
}

TEST_F(HttpWebStructFixture, FileServerGenTokenSingleAndVerify)
{
    auto token = server->genToken(R"(["/files/x"])");
    ASSERT_FALSE(token.empty());
    std::string mut = token;
    EXPECT_TRUE(server->verifyToken(mut));
}

TEST_F(HttpWebStructFixture, FileServerWebBindUnbindSequence)
{
    EXPECT_EQ(server->webBind("/d1", "/home/d1"), 0);
    EXPECT_EQ(server->webBind("/d2", "/home/d2"), 0);
    EXPECT_EQ(server->webUnbind("/d1"), 0);
    EXPECT_EQ(server->webUnbind("/d2"), 0);
    EXPECT_EQ(server->webUnbind("/d1"), -1);
}

TEST_F(HttpWebStructFixture, TokenCacheMultipleGenThenClear)
{
    auto t1 = TokenCache::GetInstance().genToken(R"(["a"])");
    auto t2 = TokenCache::GetInstance().genToken(R"(["b"])");
    auto t3 = TokenCache::GetInstance().genToken(R"(["c"])");
    EXPECT_FALSE(t1.empty());
    EXPECT_FALSE(t2.empty());
    EXPECT_FALSE(t3.empty());
    std::string m1 = t1, m2 = t2;
    EXPECT_TRUE(TokenCache::GetInstance().verifyToken(m1));
    EXPECT_TRUE(TokenCache::GetInstance().verifyToken(m2));
    TokenCache::GetInstance().clearTokens();
    std::string m3 = t3;
    EXPECT_FALSE(TokenCache::GetInstance().verifyToken(m3));
}
