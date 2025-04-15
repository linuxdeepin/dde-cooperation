#include <gtest/gtest.h>
#include "string/string_utils.h"

using namespace BaseKit;

TEST(StringTest, BasicOperations) {
    // 测试字符串工具函数
    std::string str1 = "Hello";
    EXPECT_EQ(str1.length(), 5);
    
    // 测试字符串连接
    std::string str2 = str1 + " World";
    EXPECT_EQ(str2, "Hello World");
    
    // 测试字符串比较
    EXPECT_TRUE(str1 == "Hello");
    EXPECT_FALSE(str1 == "World");
} 