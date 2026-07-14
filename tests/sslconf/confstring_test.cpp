// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "confstring.h"

#include <string>
#include <vector>

using namespace sslconf::string;

TEST(ConfStringTest, FormatPositionalArgs)
{
    auto r = format("%{1} and %{2}", "hello", "world");
    EXPECT_EQ(r, "hello and world");
}

TEST(ConfStringTest, FormatLiteralPercent)
{
    auto r = format("100%% done %{1}", "ok");
    EXPECT_EQ(r, "100% done ok");
}

TEST(ConfStringTest, FormatReorderedArgs)
{
    auto r = format("%{2} before %{1}", "A", "B");
    EXPECT_EQ(r, "B before A");
}

TEST(ConfStringTest, FormatRepeatedArg)
{
    auto r = format("%{1}%{1}%{1}", "X");
    EXPECT_EQ(r, "XXX");
}

TEST(ConfStringTest, FormatTrailingPercent)
{
    auto r = format("trailing%");
    EXPECT_EQ(r, "trailing%");
}

TEST(ConfStringTest, FormatImproperEscape)
{
    auto r = format("bad %x escape");
    EXPECT_EQ(r, "bad %x escape");
}

TEST(ConfStringTest, FormatInvalidIndex)
{
    auto r = format("inv %{abc}");
    EXPECT_EQ(r, "inv %{abc}");
}

TEST(ConfStringTest, FormatEmpty)
{
    auto r = format("");
    EXPECT_EQ(r, "");
}

TEST(ConfStringTest, FormatNoSubstitution)
{
    auto r = format("just text");
    EXPECT_EQ(r, "just text");
}

TEST(ConfStringTest, SprintfBasic)
{
    auto r = sslconf::string::sprintf("%d + %d = %d", 1, 2, 3);
    EXPECT_EQ(r, "1 + 2 = 3");
}

TEST(ConfStringTest, SprintfLongString)
{
    std::string big(2000, 'Z');
    auto r = sslconf::string::sprintf("%s", big.c_str());
    EXPECT_EQ(r, big);
}

TEST(ConfStringTest, SprintfEmpty)
{
    auto r = sslconf::string::sprintf("");
    EXPECT_EQ(r, "");
}

TEST(ConfStringTest, FindReplaceAll)
{
    std::string s = "foo bar foo baz foo";
    findReplaceAll(s, "foo", "qux");
    EXPECT_EQ(s, "qux bar qux baz qux");
}

TEST(ConfStringTest, FindReplaceAllNoMatch)
{
    std::string s = "hello";
    findReplaceAll(s, "xyz", "abc");
    EXPECT_EQ(s, "hello");
}

TEST(ConfStringTest, FindReplaceAllShorterReplacement)
{
    std::string s = "aaa bbb";
    findReplaceAll(s, "aaa", "X");
    EXPECT_EQ(s, "X bbb");
}

TEST(ConfStringTest, RemoveFileExt)
{
    EXPECT_EQ(removeFileExt("test.txt"), "test");
    EXPECT_EQ(removeFileExt("a.b.c"), "a.b");
    EXPECT_EQ(removeFileExt("noext"), "noext");
    EXPECT_EQ(removeFileExt(".hidden"), "");
}

TEST(ConfStringTest, ToHex)
{
    std::vector<std::uint8_t> data = {0x0A, 0xFF, 0x1};
    auto r = to_hex(data, 2);
    EXPECT_EQ(r, "0aff01");
}

TEST(ConfStringTest, ToHexEmpty)
{
    std::vector<std::uint8_t> data;
    EXPECT_EQ(to_hex(data, 2), "");
}

TEST(ConfStringTest, FromHex)
{
    auto r = from_hex("0aff01");
    ASSERT_EQ(r.size(), 3u);
    EXPECT_EQ(r[0], 0x0A);
    EXPECT_EQ(r[1], 0xFF);
    EXPECT_EQ(r[2], 0x01);
}

TEST(ConfStringTest, FromHexWithSeparators)
{
    auto r = from_hex("0a:ff:01");
    ASSERT_EQ(r.size(), 3u);
    EXPECT_EQ(r[0], 0x0A);
    EXPECT_EQ(r[2], 0x01);
}

TEST(ConfStringTest, FromHexUppercase)
{
    auto r = from_hex("ABCDEF");
    ASSERT_EQ(r.size(), 3u);
    EXPECT_EQ(r[0], 0xAB);
    EXPECT_EQ(r[2], 0xEF);
}

TEST(ConfStringTest, FromHexInvalidChar)
{
    auto r = from_hex("GG");
    EXPECT_TRUE(r.empty());
}

TEST(ConfStringTest, FromHexOddLength)
{
    auto r = from_hex("0a0");
    EXPECT_TRUE(r.empty());
}

TEST(ConfStringTest, FromHexEmpty)
{
    auto r = from_hex("");
    EXPECT_TRUE(r.empty());
}

TEST(ConfStringTest, Uppercase)
{
    std::string s = "Hello World 123";
    uppercase(s);
    EXPECT_EQ(s, "HELLO WORLD 123");
}

TEST(ConfStringTest, RemoveChar)
{
    std::string s = "a:b:c:d";
    removeChar(s, ':');
    EXPECT_EQ(s, "abcd");
}

TEST(ConfStringTest, RemoveCharNotFound)
{
    std::string s = "hello";
    removeChar(s, 'z');
    EXPECT_EQ(s, "hello");
}

TEST(ConfStringTest, SizeTypeToString)
{
    EXPECT_EQ(sizeTypeToString(0), "0");
    EXPECT_EQ(sizeTypeToString(42), "42");
    EXPECT_EQ(sizeTypeToString(1000000), "1000000");
}

TEST(ConfStringTest, StringToSizeType)
{
    EXPECT_EQ(stringToSizeType("0"), 0u);
    EXPECT_EQ(stringToSizeType("42"), 42u);
    EXPECT_EQ(stringToSizeType("  123abc"), 123u);
}

TEST(ConfStringTest, SplitString)
{
    auto r = splitString("a,b,c", ',');
    ASSERT_EQ(r.size(), 3u);
    EXPECT_EQ(r[0], "a");
    EXPECT_EQ(r[1], "b");
    EXPECT_EQ(r[2], "c");
}

TEST(ConfStringTest, SplitStringEmptySegments)
{
    auto r = splitString("a,,b", ',');
    ASSERT_EQ(r.size(), 2u);
    EXPECT_EQ(r[0], "a");
    EXPECT_EQ(r[1], "b");
}

TEST(ConfStringTest, SplitStringTrailingSeparator)
{
    auto r = splitString("a,b,", ',');
    ASSERT_EQ(r.size(), 2u);
    EXPECT_EQ(r[0], "a");
    EXPECT_EQ(r[1], "b");
}

TEST(ConfStringTest, SplitStringNoSeparator)
{
    auto r = splitString("hello", ',');
    ASSERT_EQ(r.size(), 1u);
    EXPECT_EQ(r[0], "hello");
}

TEST(ConfStringTest, CaselessCmpEqual)
{
    EXPECT_TRUE(CaselessCmp::equal("HELLO", "hello"));
    EXPECT_TRUE(CaselessCmp::equal("AbCd", "aBcD"));
    EXPECT_FALSE(CaselessCmp::equal("abc", "abcd"));
    EXPECT_FALSE(CaselessCmp::equal("abc", "abd"));
}

TEST(ConfStringTest, CaselessCmpLess)
{
    EXPECT_TRUE(CaselessCmp::less("abc", "abd"));
    EXPECT_FALSE(CaselessCmp::less("abc", "abc"));
    EXPECT_FALSE(CaselessCmp::less("abd", "abc"));
    EXPECT_TRUE(CaselessCmp::less("ABC", "abd"));
}

TEST(ConfStringTest, CaselessCmpOperator)
{
    CaselessCmp cmp;
    EXPECT_TRUE(cmp("abc", "abd"));
    EXPECT_FALSE(cmp("abc", "abc"));
}

TEST(ConfStringTest, CaselessCmpCharCompare)
{
    EXPECT_TRUE(CaselessCmp::cmpEqual('A', 'a'));
    EXPECT_FALSE(CaselessCmp::cmpEqual('a', 'b'));
    EXPECT_TRUE(CaselessCmp::cmpLess('a', 'B'));
    EXPECT_FALSE(CaselessCmp::cmpLess('B', 'a'));
}
