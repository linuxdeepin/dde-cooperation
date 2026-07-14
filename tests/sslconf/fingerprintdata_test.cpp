// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "fingerprintdata.h"

using namespace sslconf;

TEST(FingerprintDataTest, EqualitySame)
{
    FingerprintData a, b;
    a.algorithm = "sha256";
    a.data = {1, 2, 3};
    b.algorithm = "sha256";
    b.data = {1, 2, 3};
    EXPECT_TRUE(a == b);
}

TEST(FingerprintDataTest, EqualityDifferentAlgo)
{
    FingerprintData a, b;
    a.algorithm = "sha256";
    b.algorithm = "sha1";
    EXPECT_FALSE(a == b);
}

TEST(FingerprintDataTest, EqualityDifferentData)
{
    FingerprintData a, b;
    a.algorithm = "sha256";
    a.data = {1};
    b.algorithm = "sha256";
    b.data = {2};
    EXPECT_FALSE(a == b);
}

TEST(FingerprintDataTest, ValidWhenAlgoSet)
{
    FingerprintData a;
    a.algorithm = "sha256";
    EXPECT_TRUE(a.valid());
}

TEST(FingerprintDataTest, InvalidWhenEmpty)
{
    FingerprintData a;
    EXPECT_FALSE(a.valid());
}

TEST(FingerprintDataTest, TypeToString)
{
    EXPECT_STREQ(fingerprint_type_to_string(FingerprintType::SHA1), "sha1");
    EXPECT_STREQ(fingerprint_type_to_string(FingerprintType::SHA256), "sha256");
    EXPECT_STREQ(fingerprint_type_to_string(FingerprintType::INVALID), "invalid");
}

TEST(FingerprintDataTest, StringToType)
{
    EXPECT_EQ(fingerprint_type_from_string("sha1"), FingerprintType::SHA1);
    EXPECT_EQ(fingerprint_type_from_string("sha256"), FingerprintType::SHA256);
    EXPECT_EQ(fingerprint_type_from_string("unknown"), FingerprintType::INVALID);
}
