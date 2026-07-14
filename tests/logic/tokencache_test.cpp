// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "tokencache.h"

#include <chrono>
#include <thread>

class TokenCacheTest : public ::testing::Test {
protected:
    void SetUp() override { TokenCache::GetInstance().clearTokens(); }
    void TearDown() override { TokenCache::GetInstance().clearTokens(); }
};

TEST_F(TokenCacheTest, GenTokenNonEmpty)
{
    auto token = TokenCache::GetInstance().genToken(R"(["file1","file2"])");
    EXPECT_FALSE(token.empty());
}

TEST_F(TokenCacheTest, GenTokenDifferentEachCall)
{
    auto t1 = TokenCache::GetInstance().genToken(R"(["a"])");
    auto t2 = TokenCache::GetInstance().genToken(R"(["b"])");
    EXPECT_NE(t1, t2);
}

TEST_F(TokenCacheTest, GenAndVerifyValidToken)
{
    auto token = TokenCache::GetInstance().genToken(R"(["file1"])");
    EXPECT_FALSE(token.empty());
    std::string mut = token;
    EXPECT_TRUE(TokenCache::GetInstance().verifyToken(mut));
}

TEST_F(TokenCacheTest, VerifyInvalidTokenReturnsFalse)
{
    std::string bad = "not.a.valid.jwt";
    EXPECT_FALSE(TokenCache::GetInstance().verifyToken(bad));
}

TEST_F(TokenCacheTest, VerifyEmptyTokenReturnsFalse)
{
    std::string empty;
    EXPECT_FALSE(TokenCache::GetInstance().verifyToken(empty));
}

TEST_F(TokenCacheTest, VerifyGarbageStringReturnsFalse)
{
    std::string garbage = "!!!garbage!!!";
    EXPECT_FALSE(TokenCache::GetInstance().verifyToken(garbage));
}

TEST_F(TokenCacheTest, GetWebfromTokenValid)
{
    std::string info = R"(["file1.txt","dir1/"])";
    auto token = TokenCache::GetInstance().genToken(info);
    EXPECT_FALSE(token.empty());

    auto webs = TokenCache::GetInstance().getWebfromToken(token);
    EXPECT_EQ(webs.size(), 2u);
    EXPECT_EQ(webs[0], "file1.txt");
    EXPECT_EQ(webs[1], "dir1/");
}
TEST_F(TokenCacheTest, GetWebfromTokenInvalid)
{
    // "invalid.jwt.token" has 3 parts so jwt::decode succeeds but payload
    // parsing fails; getWebfromToken catches and returns empty or throws.
    try {
        auto webs = TokenCache::GetInstance().getWebfromToken("invalid.jwt.token");
        EXPECT_TRUE(webs.empty());
    } catch (...) {
        SUCCEED();
    }
}

TEST_F(TokenCacheTest, GetWebfromTokenEmpty)
{
    EXPECT_ANY_THROW(TokenCache::GetInstance().getWebfromToken(""));
}

TEST_F(TokenCacheTest, ClearTokensInvalidatesCached)
{
    auto token = TokenCache::GetInstance().genToken(R"(["x"])");
    EXPECT_FALSE(token.empty());

    // genToken caches the key pair; clear removes it -> verify should fail
    TokenCache::GetInstance().clearTokens();
    std::string mut = token;
    EXPECT_FALSE(TokenCache::GetInstance().verifyToken(mut));
}

TEST_F(TokenCacheTest, GenTokenMultipleWebEntries)
{
    std::string info = R"(["a","b","c","d"])";
    auto token = TokenCache::GetInstance().genToken(info);
    auto webs = TokenCache::GetInstance().getWebfromToken(token);
    EXPECT_EQ(webs.size(), 4u);
}

TEST_F(TokenCacheTest, GenTokenEmptyInfoArray)
{
    auto token = TokenCache::GetInstance().genToken("[]");
    EXPECT_FALSE(token.empty());
    auto webs = TokenCache::GetInstance().getWebfromToken(token);
    EXPECT_TRUE(webs.empty());
}

TEST_F(TokenCacheTest, GetWebfromTokenMalformedPayload)
{
    auto token = TokenCache::GetInstance().genToken(R"(["ok"])");
    std::string broken = token.substr(0, token.size() / 2);
    // truncated JWT -> decode may throw; both outcomes acceptable
    try {
        auto webs = TokenCache::GetInstance().getWebfromToken(broken);
        EXPECT_TRUE(webs.empty());
    } catch (...) {
        SUCCEED();
    }
}
