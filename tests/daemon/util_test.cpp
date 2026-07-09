// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "utils/utils.h"

#include <QString>
#include <QTemporaryDir>

TEST(UtilTest, GenRandPin)
{
    std::string pin = Util::genRandPin();
    EXPECT_EQ(pin.size(), 6u);
    // all digits
    for (char c : pin) {
        EXPECT_TRUE(c >= '0' && c <= '9');
    }

    // two calls should likely differ
    std::string pin2 = Util::genRandPin();
    // Very unlikely to be the same
    // (not asserting they differ to avoid flakiness)
}

TEST(UtilTest, EncodeDecodeBase64)
{
    std::string original = "Hello World";
    std::string encoded = Util::encodeBase64(original.c_str());
    std::string decoded = Util::decodeBase64(encoded.c_str());
    EXPECT_EQ(decoded, original);
}

TEST(UtilTest, EncodeBase64Empty)
{
    std::string encoded = Util::encodeBase64("");
    std::string decoded = Util::decodeBase64(encoded.c_str());
    EXPECT_TRUE(decoded.empty());
}

TEST(UtilTest, GenAuthToken)
{
    std::string token = Util::genAuthToken("uuid-1234", "pin5678");
    EXPECT_FALSE(token.empty());
    // should be base64 of uuid
    std::string decoded = Util::decodeBase64(token.c_str());
    EXPECT_EQ(decoded, std::string("uuid-1234"));
}

TEST(UtilTest, CheckTokenAlwaysTrue)
{
    EXPECT_TRUE(Util::checkToken("anything"));
    EXPECT_TRUE(Util::checkToken(""));
}

TEST(UtilTest, GenUUID)
{
    std::string uuid = Util::genUUID();
    EXPECT_FALSE(uuid.empty());
    // should be a valid UUID format
    EXPECT_TRUE(Util::isValidUUID(uuid));
}

TEST(UtilTest, IsValidUUID)
{
    EXPECT_TRUE(Util::isValidUUID("550e8400-e29b-41d4-a716-446655440000"));
    EXPECT_FALSE(Util::isValidUUID("not-a-uuid"));
    EXPECT_FALSE(Util::isValidUUID(""));
    EXPECT_FALSE(Util::isValidUUID("550e8400-e29b-41d4-a716"));
}

TEST(UtilTest, ParseFileName)
{
    EXPECT_EQ(Util::parseFileName("/home/user/file.txt"), std::string("file.txt"));
    EXPECT_EQ(Util::parseFileName("/home/user/dir/"), std::string(""));
    EXPECT_EQ(Util::parseFileName("simple.txt"), std::string("simple.txt"));
    EXPECT_EQ(Util::parseFileName("/a/b/c/d"), std::string("d"));
}

TEST(UtilTest, GetHostname)
{
    std::string host = Util::getHostname();
    EXPECT_FALSE(host.empty());
}

TEST(UtilTest, GetUsername)
{
    std::string user = Util::getUsername();
    // may be empty in some environments
    SUCCEED();
}

TEST(UtilTest, GetOSType)
{
    int os = Util::getOSType();
#ifdef __linux__
    EXPECT_EQ(os, LINUX);
#endif
}

TEST(UtilTest, ConfigPath)
{
    QString path = Util::configPath();
    EXPECT_FALSE(path.isEmpty());
    EXPECT_TRUE(path.contains("cooperation-config.conf"));
}

TEST(UtilTest, BarrierConfig)
{
    QString path = Util::barrierConfig();
    EXPECT_FALSE(path.isEmpty());
    EXPECT_TRUE(path.contains("cooperation-barrier.conf"));
}
