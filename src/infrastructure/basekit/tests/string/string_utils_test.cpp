#include <gtest/gtest.h>
#include "string/string_utils.h"

using namespace BaseKit;

// 测试空白字符检查
TEST(StringUtilsTest, IsBlank) {
    // 测试字符
    EXPECT_TRUE(StringUtils::IsBlank(' '));
    EXPECT_TRUE(StringUtils::IsBlank('\t'));
    EXPECT_TRUE(StringUtils::IsBlank('\n'));
    EXPECT_TRUE(StringUtils::IsBlank('\r'));
    EXPECT_FALSE(StringUtils::IsBlank('a'));
    EXPECT_FALSE(StringUtils::IsBlank('0'));
    
    // 测试C字符串
    EXPECT_TRUE(StringUtils::IsBlank(""));
    EXPECT_TRUE(StringUtils::IsBlank("   "));
    EXPECT_TRUE(StringUtils::IsBlank("\t\n\r"));
    EXPECT_FALSE(StringUtils::IsBlank("a"));
    EXPECT_FALSE(StringUtils::IsBlank("   a   "));
    
    // 测试string_view
    EXPECT_TRUE(StringUtils::IsBlank(std::string_view("")));
    EXPECT_TRUE(StringUtils::IsBlank(std::string_view("   ")));
    EXPECT_FALSE(StringUtils::IsBlank(std::string_view("abc")));
}

// 测试模式匹配
TEST(StringUtilsTest, IsPatternMatch) {
    EXPECT_TRUE(StringUtils::IsPatternMatch("Demo.*;Live.*", "DemoAccount"));
    EXPECT_TRUE(StringUtils::IsPatternMatch("Demo.*;Live.*", "LiveAccount"));
    EXPECT_FALSE(StringUtils::IsPatternMatch("Demo.*;Live.*", "UnknownAccount"));
    EXPECT_FALSE(StringUtils::IsPatternMatch("!Demo.*;!Live.*", "DemoAccount"));
    EXPECT_FALSE(StringUtils::IsPatternMatch("!Demo.*;!Live.*", "LiveAccount"));
    EXPECT_TRUE(StringUtils::IsPatternMatch("!Demo.*;!Live.*", "UnknownAccount"));
}

// 测试大小写转换
TEST(StringUtilsTest, CaseConversion) {
    // 测试单个字符
    EXPECT_EQ(StringUtils::ToLower('A'), 'a');
    EXPECT_EQ(StringUtils::ToUpper('a'), 'A');
    EXPECT_EQ(StringUtils::ToLower('1'), '1'); // 数字不变
    
    // 测试字符串
    EXPECT_EQ(StringUtils::ToLower("AbCdEf"), "abcdef");
    EXPECT_EQ(StringUtils::ToUpper("aBcDeF"), "ABCDEF");
    
    // 测试原地转换
    std::string lower = "AbCdEf";
    std::string upper = "aBcDeF";
    StringUtils::Lower(lower);
    StringUtils::Upper(upper);
    EXPECT_EQ(lower, "abcdef");
    EXPECT_EQ(upper, "ABCDEF");
}

// 测试修剪字符串
TEST(StringUtilsTest, Trim) {
    // 测试左侧修剪
    EXPECT_EQ(StringUtils::ToLTrim("  abc  "), "abc  ");
    EXPECT_EQ(StringUtils::ToLTrim("abc  "), "abc  ");
    
    // 测试右侧修剪
    EXPECT_EQ(StringUtils::ToRTrim("  abc  "), "  abc");
    EXPECT_EQ(StringUtils::ToRTrim("  abc"), "  abc");
    
    // 测试两侧修剪
    EXPECT_EQ(StringUtils::ToTrim("  abc  "), "abc");
    EXPECT_EQ(StringUtils::ToTrim("abc"), "abc");
    
    // 测试原地修剪
    std::string left = "  abc  ";
    std::string right = "  abc  ";
    std::string both = "  abc  ";
    StringUtils::LTrim(left);
    StringUtils::RTrim(right);
    StringUtils::Trim(both);
    EXPECT_EQ(left, "abc  ");
    EXPECT_EQ(right, "  abc");
    EXPECT_EQ(both, "abc");
}

// 测试空白字符删除
TEST(StringUtilsTest, RemoveBlank) {
    EXPECT_EQ(StringUtils::RemoveBlank("a b c"), "abc");
    EXPECT_EQ(StringUtils::RemoveBlank(" a\tb\nc "), "abc");
    
    std::string str = " a\tb\nc ";
    StringUtils::RemoveBlank(str);
    EXPECT_EQ(str, "abc");
}

// 测试字符串比较
TEST(StringUtilsTest, Compare) {
    // 区分大小写的比较
    EXPECT_TRUE(StringUtils::Compare("abc", "abc"));
    EXPECT_FALSE(StringUtils::Compare("abc", "Abc"));
    
    // 不区分大小写的比较
    EXPECT_TRUE(StringUtils::CompareNoCase("abc", "abc"));
    EXPECT_TRUE(StringUtils::CompareNoCase("abc", "ABC"));
    EXPECT_FALSE(StringUtils::CompareNoCase("abc", "def"));
}

// 测试字符串包含
TEST(StringUtilsTest, Contains) {
    // 测试字符
    EXPECT_TRUE(StringUtils::Contains("abc", 'a'));
    EXPECT_FALSE(StringUtils::Contains("abc", 'd'));
    
    // 测试C字符串
    EXPECT_TRUE(StringUtils::Contains("abc", "a"));
    EXPECT_TRUE(StringUtils::Contains("abc", "bc"));
    EXPECT_FALSE(StringUtils::Contains("abc", "d"));
    
    // 测试string_view
    EXPECT_TRUE(StringUtils::Contains(std::string_view("abc"), std::string_view("a")));
    EXPECT_FALSE(StringUtils::Contains(std::string_view("abc"), std::string_view("d")));
}

// 测试字符串计数
TEST(StringUtilsTest, CountAll) {
    EXPECT_EQ(StringUtils::CountAll("abcabc", "a"), 2);
    EXPECT_EQ(StringUtils::CountAll("abcabc", "bc"), 2);
    EXPECT_EQ(StringUtils::CountAll("abcabc", "d"), 0);
}

// 测试字符串替换
TEST(StringUtilsTest, Replace) {
    // 测试替换第一个
    std::string first = "abcabc";
    EXPECT_TRUE(StringUtils::ReplaceFirst(first, "a", "X"));
    EXPECT_EQ(first, "Xbcabc");
    
    // 测试替换最后一个
    std::string last = "abcabc";
    EXPECT_TRUE(StringUtils::ReplaceLast(last, "a", "X"));
    EXPECT_EQ(last, "abcXbc");
    
    // 测试替换所有
    std::string all = "abcabc";
    EXPECT_TRUE(StringUtils::ReplaceAll(all, "a", "X"));
    EXPECT_EQ(all, "XbcXbc");
    
    // 测试替换找不到的字符串
    std::string notFound = "abcabc";
    EXPECT_FALSE(StringUtils::ReplaceFirst(notFound, "d", "X"));
    EXPECT_EQ(notFound, "abcabc");
}

// 测试前缀和后缀
TEST(StringUtilsTest, StartsAndEndsWith) {
    // 测试前缀
    EXPECT_TRUE(StringUtils::StartsWith("abcdef", "abc"));
    EXPECT_FALSE(StringUtils::StartsWith("abcdef", "def"));
    
    // 测试后缀
    EXPECT_TRUE(StringUtils::EndsWith("abcdef", "def"));
    EXPECT_FALSE(StringUtils::EndsWith("abcdef", "abc"));
}

// 测试字符串分割
TEST(StringUtilsTest, Split) {
    // 使用字符分割
    std::vector<std::string> tokens1 = StringUtils::Split("a,b,c", ',');
    ASSERT_EQ(tokens1.size(), 3);
    EXPECT_EQ(tokens1[0], "a");
    EXPECT_EQ(tokens1[1], "b");
    EXPECT_EQ(tokens1[2], "c");
    
    // 使用字符串分割
    std::vector<std::string> tokens2 = StringUtils::Split("a::b::c", "::");
    ASSERT_EQ(tokens2.size(), 3);
    EXPECT_EQ(tokens2[0], "a");
    EXPECT_EQ(tokens2[1], "b");
    EXPECT_EQ(tokens2[2], "c");
    
    // 使用多个分隔符分割
    std::vector<std::string> tokens3 = StringUtils::SplitByAny("a,b;c", ",;");
    ASSERT_EQ(tokens3.size(), 3);
    EXPECT_EQ(tokens3[0], "a");
    EXPECT_EQ(tokens3[1], "b");
    EXPECT_EQ(tokens3[2], "c");
    
    // 测试跳过空白
    std::vector<std::string> tokens4 = StringUtils::Split("a,,c", ',', true);
    ASSERT_EQ(tokens4.size(), 2);
    EXPECT_EQ(tokens4[0], "a");
    EXPECT_EQ(tokens4[1], "c");
}

// 测试字符串连接
TEST(StringUtilsTest, Join) {
    std::vector<std::string> tokens = {"a", "b", "c"};
    
    // 基本连接
    EXPECT_EQ(StringUtils::Join(tokens), "abc");
    
    // 使用字符连接
    EXPECT_EQ(StringUtils::Join(tokens, ','), "a,b,c");
    
    // 使用字符串连接
    EXPECT_EQ(StringUtils::Join(tokens, "::"), "a::b::c");
    
    // 测试跳过空白
    std::vector<std::string> tokensWithEmpty = {"a", "", "c"};
    EXPECT_EQ(StringUtils::Join(tokensWithEmpty, ',', true), "a,c");
    
    // 测试跳过空白和空格
    std::vector<std::string> tokensWithBlank = {"a", " ", "c"};
    EXPECT_EQ(StringUtils::Join(tokensWithBlank, ',', false, true), "a,c");
}

// 测试数值转换
TEST(StringUtilsTest, Conversion) {
    // int 转 string
    EXPECT_EQ(StringUtils::ToString(123), "123");
    
    // double 转 string
    EXPECT_EQ(StringUtils::ToString(123.456), "123.456");
    
    // string 转 int
    EXPECT_EQ(StringUtils::FromString<int>("123"), 123);
    
    // string 转 double
    EXPECT_DOUBLE_EQ(StringUtils::FromString<double>("123.456"), 123.456);
} 