#include <gtest/gtest.h>
#include "fmt_utils.h"

using namespace fmt::utils;

TEST(UtilsTest, Split) {
    auto result = split("a,b,c", ',');
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "a");
    EXPECT_EQ(result[1], "b");
    EXPECT_EQ(result[2], "c");
}

TEST(UtilsTest, Trim) {
    EXPECT_EQ(trim("  hello  "), "hello");
    EXPECT_EQ(trim("\t\nworld\r\n"), "world");
}

TEST(UtilsTest, StringChecks) {
    EXPECT_TRUE(starts_with("hello world", "hello"));
    EXPECT_TRUE(ends_with("hello world", "world"));
    EXPECT_FALSE(starts_with("hello world", "world"));
    EXPECT_FALSE(ends_with("hello world", "hello"));
}

TEST(UtilsTest, UTF8Conversion) {
    EXPECT_EQ(to_utf8(0x24), "$");
    EXPECT_EQ(to_utf8(0x20AC), "€");
}

TEST(UtilsTest, EmptyString) {
    EXPECT_TRUE(split("", ',').empty());
    EXPECT_EQ(trim(""), "");
    EXPECT_FALSE(starts_with("", "test"));
    EXPECT_FALSE(ends_with("", "test"));
}

TEST(UtilsTest, UTF8Handling) {
    std::string utf8_str = to_utf8(0x20AC);  // Euro sign
    EXPECT_EQ(utf8_str.length(), 3);
    EXPECT_EQ(count_code_points(utf8_str.c_str()), 1);
}
