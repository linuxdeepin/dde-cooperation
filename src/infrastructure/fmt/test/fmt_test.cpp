#include <gtest/gtest.h>
#include "fmt.h"

class FmtTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(FmtTest, BasicFormatting) {
    EXPECT_EQ(fmt::format("Hello, {}", "world"), "Hello, world");
    EXPECT_EQ(fmt::format("{} + {} = {}", 1, 2, 3), "1 + 2 = 3");
    EXPECT_EQ(fmt::format("{}", ""), "");
}

TEST_F(FmtTest, NumberFormatting) {
    EXPECT_EQ(fmt::format("{:d}", 42), "42");
    EXPECT_EQ(fmt::format("{:05d}", 42), "00042");
}

TEST_F(FmtTest, StringFormatting) {
    EXPECT_EQ(fmt::format("{:<10}", "left"), "left      ");
    EXPECT_EQ(fmt::format("{:>10}", "right"), "     right");
}

TEST_F(FmtTest, CustomTypeFormatting) {
    struct Point {
        int x, y;
    };
    Point p{10, 20};
    EXPECT_EQ(fmt::format("Point({},{})", p.x, p.y), "Point(10,20)");
}

TEST_F(FmtTest, EdgeCases) {
    EXPECT_EQ(fmt::format("{}", ""), "");
    EXPECT_EQ(fmt::format("{:5}", ""), "     ");
    EXPECT_THROW(fmt::format("{", "test"), fmt::FormatError);
    EXPECT_THROW(fmt::format("}", "test"), fmt::FormatError);
}
