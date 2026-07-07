// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "string/encoding.h"

#include <string>
#include <string_view>

using namespace BaseKit;

// ===== Base16 (hex) =====
TEST(EncodingBase16Test, RoundTripAscii)
{
    const std::string src = "Hello, World!";
    auto enc = Encoding::Base16Encode(src);
    EXPECT_EQ(enc, "48656C6C6F2C20576F726C6421");   // 输出大写
    auto dec = Encoding::Base16Decode(enc);
    EXPECT_EQ(dec, src);
}

TEST(EncodingBase16Test, EmptyAndSingleByte)
{
    EXPECT_EQ(Encoding::Base16Encode(""), "");
    EXPECT_EQ(Encoding::Base16Decode(""), "");
    EXPECT_EQ(Encoding::Base16Encode(std::string("\x00", 1)), "00");
    auto dec = Encoding::Base16Decode("00");
    ASSERT_EQ(dec.size(), 1u);
    EXPECT_EQ(dec[0], '\0');
}

TEST(EncodingBase16Test, AllBytesRange)
{
    std::string all;
    for (int i = 0; i < 256; ++i)
        all.push_back(static_cast<char>(i));
    auto enc = Encoding::Base16Encode(all);
    EXPECT_EQ(enc.size(), 512u);
    auto dec = Encoding::Base16Decode(enc);
    EXPECT_EQ(dec, all);
}

// ===== Base32 =====
TEST(EncodingBase32Test, RoundTrip)
{
    const std::string src = "some data here";
    auto enc = Encoding::Base32Encode(src);
    auto dec = Encoding::Base32Decode(enc);
    EXPECT_EQ(dec, src);
}

TEST(EncodingBase32Test, EmptyInput)
{
    EXPECT_EQ(Encoding::Base32Encode(""), "");
    EXPECT_EQ(Encoding::Base32Decode(""), "");
}

// ===== Base64 =====
TEST(EncodingBase64Test, RoundTrip)
{
    const std::string src = "The quick brown fox jumps over the lazy dog";
    auto enc = Encoding::Base64Encode(src);
    auto dec = Encoding::Base64Decode(enc);
    EXPECT_EQ(dec, src);
}

TEST(EncodingBase64Test, KnownVectors)
{
    EXPECT_EQ(Encoding::Base64Encode("Man"), "TWFu");
    EXPECT_EQ(Encoding::Base64Encode("Ma"), "TWE=");
    EXPECT_EQ(Encoding::Base64Encode("M"), "TQ==");
    EXPECT_EQ(Encoding::Base64Decode("TWFu"), "Man");
    EXPECT_EQ(Encoding::Base64Decode("TWE="), "Ma");
    EXPECT_EQ(Encoding::Base64Decode("TQ=="), "M");
}

TEST(EncodingBase64Test, EmptyInput)
{
    EXPECT_EQ(Encoding::Base64Encode(""), "");
    EXPECT_EQ(Encoding::Base64Decode(""), "");
}

TEST(EncodingBase64Test, BinaryRoundTrip)
{
    std::string bin;
    for (int i = 0; i < 256; ++i)
        bin.push_back(static_cast<char>(i));
    auto enc = Encoding::Base64Encode(bin);
    auto dec = Encoding::Base64Decode(enc);
    EXPECT_EQ(dec, bin);
}

// ===== URL =====
TEST(EncodingURLTest, RoundTripReservedChars)
{
    const std::string src = "hello world&foo=bar?x=1+2";
    auto enc = Encoding::URLEncode(src);
    EXPECT_NE(enc.find('%'), std::string::npos);
    auto dec = Encoding::URLDecode(enc);
    EXPECT_EQ(dec, src);
}

TEST(EncodingURLTest, SafeCharsNotEncoded)
{
    auto enc = Encoding::URLEncode("abc123");
    EXPECT_EQ(enc, "abc123");
}

TEST(EncodingURLTest, Empty)
{
    EXPECT_EQ(Encoding::URLEncode(""), "");
    EXPECT_EQ(Encoding::URLDecode(""), "");
}

TEST(EncodingURLTest, PlusAndPercentDecoding)
{
    // %20 → space
    EXPECT_EQ(Encoding::URLDecode("a%20b"), "a b");
}

// ===== UTF conversions =====
TEST(EncodingUTFTest, UTF8ToUTF16RoundTrip)
{
    const std::string utf8 = u8"你好，世界 Hello 123";
    auto u16 = Encoding::UTF8toUTF16(utf8);
    auto u32 = Encoding::UTF8toUTF32(utf8);
    auto back8_from16 = Encoding::UTF16toUTF8(u16);
    auto back8_from32 = Encoding::UTF32toUTF8(u32);
    EXPECT_EQ(back8_from16, utf8);
    EXPECT_EQ(back8_from32, utf8);

    auto u32_from16 = Encoding::UTF16toUTF32(u16);
    auto u16_from32 = Encoding::UTF32toUTF16(u32);
    EXPECT_EQ(u32_from16, u32);
    EXPECT_EQ(u16_from32, u16);
}

TEST(EncodingUTFTest, EmptyUTF8)
{
    EXPECT_TRUE(Encoding::UTF8toUTF16("").empty());
    EXPECT_TRUE(Encoding::UTF8toUTF32("").empty());
    EXPECT_EQ(Encoding::UTF16toUTF8(u""), "");
    EXPECT_EQ(Encoding::UTF32toUTF8(U""), "");
}

TEST(EncodingUTFTest, SystemWStringRoundTrip)
{
    const std::string utf8 = u8"测试 abc 123";
    std::wstring wide = Encoding::FromUTF8(utf8);
    std::string back = Encoding::ToUTF8(wide);
    EXPECT_EQ(back, utf8);
}
