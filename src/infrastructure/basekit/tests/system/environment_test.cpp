#include <gtest/gtest.h>
#include "system/environment.h"
#include <cstdlib>
#include <string>

using namespace BaseKit;

// 测试系统位数检测
TEST(EnvironmentTest, SystemBitness) {
    // 32位和64位应该是互斥的
    EXPECT_NE(Environment::Is32BitOS(), Environment::Is64BitOS());
    
    // 在现代系统中，64位操作系统更为常见
    #if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
    EXPECT_TRUE(Environment::Is64BitOS());
    EXPECT_FALSE(Environment::Is32BitOS());
    #endif
    
    // 当前进程位数检测
    EXPECT_NE(Environment::Is32BitProcess(), Environment::Is64BitProcess());
    
    // 在现代系统中，64位进程更为常见
    #if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
    EXPECT_TRUE(Environment::Is64BitProcess());
    EXPECT_FALSE(Environment::Is32BitProcess());
    #endif
}

// 测试编译模式检测
TEST(EnvironmentTest, BuildMode) {
    // Debug和Release应该是互斥的
    EXPECT_NE(Environment::IsDebug(), Environment::IsRelease());
    
    // 根据实际编译模式检查
    #ifdef NDEBUG
    EXPECT_TRUE(Environment::IsRelease());
    EXPECT_FALSE(Environment::IsDebug());
    #else
    EXPECT_TRUE(Environment::IsDebug());
    EXPECT_FALSE(Environment::IsRelease());
    #endif
}

// 测试系统字节序检测
TEST(EnvironmentTest, Endianness) {
    // 大端和小端应该是互斥的
    EXPECT_NE(Environment::IsBigEndian(), Environment::IsLittleEndian());
    
    // 大多数现代处理器使用小端字节序
    #if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    EXPECT_TRUE(Environment::IsLittleEndian());
    EXPECT_FALSE(Environment::IsBigEndian());
    #elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    EXPECT_TRUE(Environment::IsBigEndian());
    EXPECT_FALSE(Environment::IsLittleEndian());
    #endif
}

// 测试系统版本获取
TEST(EnvironmentTest, OSVersion) {
    // 操作系统版本字符串不应为空
    std::string version = Environment::OSVersion();
    EXPECT_FALSE(version.empty());
    
    // 输出版本信息以便调试
    std::cout << "OS Version: " << version << std::endl;
}

// 测试行尾符
TEST(EnvironmentTest, EndLine) {
    // 行尾符不应为空
    EXPECT_FALSE(Environment::EndLine().empty());
    
    // Unix和Windows行尾符应不同
    EXPECT_NE(Environment::UnixEndLine(), Environment::WindowsEndLine());
    
    // Unix行尾符应为\n
    EXPECT_EQ(Environment::UnixEndLine(), "\n");
    
    // Windows行尾符应为\r\n
    EXPECT_EQ(Environment::WindowsEndLine(), "\r\n");
    
    // 当前系统的行尾符应该与Unix或Windows其中之一匹配
    EXPECT_TRUE(Environment::EndLine() == Environment::UnixEndLine() || 
                Environment::EndLine() == Environment::WindowsEndLine());
}

// 测试环境变量操作
TEST(EnvironmentTest, EnvironmentVariables) {
    // 设置一个唯一的测试环境变量
    std::string testVarName = "BASEKIT_TEST_VAR";
    std::string testVarValue = "测试值" + std::to_string(rand());
    
    // 设置环境变量
    Environment::SetEnvar(testVarName, testVarValue);
    
    // 获取所有环境变量
    auto allVars = Environment::envars();
    EXPECT_FALSE(allVars.empty());
    
    // 确保测试变量被设置
    EXPECT_EQ(Environment::GetEnvar(testVarName), testVarValue);
    EXPECT_TRUE(allVars.find(testVarName) != allVars.end());
    if (allVars.find(testVarName) != allVars.end()) {
        EXPECT_EQ(allVars[testVarName], testVarValue);
    }
    
    // 清除环境变量
    Environment::ClearEnvar(testVarName);
    
    // 确保环境变量已被清除（可能返回空字符串或抛出异常，取决于实现）
    try {
        std::string clearedValue = Environment::GetEnvar(testVarName);
        EXPECT_TRUE(clearedValue.empty());
    } catch (const Exception& ex) {
        // 如果实现是抛出异常，也认为测试通过
        SUCCEED();
    }
}

// 测试特定的环境变量
TEST(EnvironmentTest, SpecificEnvironmentVariables) {
    // 测试常见环境变量（如PATH）
    std::string path = Environment::GetEnvar("PATH");
    EXPECT_FALSE(path.empty());
    
    // 测试主目录环境变量
    std::string home;
    #ifdef _WIN32
    home = Environment::GetEnvar("USERPROFILE");
    #else
    home = Environment::GetEnvar("HOME");
    #endif
    EXPECT_FALSE(home.empty());
    
    // 输出一些关键环境变量以便调试
    std::cout << "PATH: " << path << std::endl;
    std::cout << "HOME: " << home << std::endl;
} 