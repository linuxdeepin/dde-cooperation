#include <catch2/catch_all.hpp>
#include <logging/version.h>
#include <string>
#include <sstream>

using namespace Logging;

TEST_CASE("Version information", "[version]") {
    SECTION("Version constants") {
        // 测试版本号常量是否有效
        REQUIRE(VERSION_MAJOR >= 0);
        REQUIRE(VERSION_MINOR >= 0);
        REQUIRE(VERSION_PATCH >= 0);
        
        // 测试版本字符串不为空
        REQUIRE(!VERSION_STRING.empty());
        
        // 测试版本字符串格式
        REQUIRE(VERSION_STRING.find_first_of("0123456789") != std::string::npos);
        
        // 确认至少有一个点分隔符
        REQUIRE(VERSION_STRING.find('.') != std::string::npos);
    }
    
    SECTION("Version class functionality") {
        // 测试默认构造函数
        Version defaultVersion;
        REQUIRE(defaultVersion.major == 0);
        REQUIRE(defaultVersion.minor == 0);
        REQUIRE(defaultVersion.patch == 0);
        
        // 测试完整构造函数
        Version version(1, 2, 3);
        REQUIRE(version.major == 1);
        REQUIRE(version.minor == 2);
        REQUIRE(version.patch == 3);
        
        // 测试从字符串解析
        Version versionFromString("2.3.4");
        REQUIRE(versionFromString.major == 2);
        REQUIRE(versionFromString.minor == 3);
        REQUIRE(versionFromString.patch == 4);
        
        // 测试不完整版本字符串
        Version partialVersion("5.6");
        REQUIRE(partialVersion.major == 5);
        REQUIRE(partialVersion.minor == 6);
        REQUIRE(partialVersion.patch == 0);
    }
    
    SECTION("Version comparison") {
        // 创建版本实例进行比较
        Version v1(1, 0, 0);
        Version v2(1, 1, 0);
        Version v3(1, 1, 1);
        Version v4(2, 0, 0);
        Version v5(1, 0, 0);  // 与v1相同
        
        // 测试相等和不等
        REQUIRE(v1 == v5);
        REQUIRE_FALSE(v1 == v2);
        REQUIRE(v1 != v2);
        REQUIRE_FALSE(v1 != v5);
        
        // 测试比较运算符
        REQUIRE(v1 < v2);
        REQUIRE(v2 < v3);
        REQUIRE(v3 < v4);
        REQUIRE_FALSE(v2 < v1);
        
        REQUIRE(v2 > v1);
        REQUIRE(v3 > v2);
        REQUIRE(v4 > v3);
        REQUIRE_FALSE(v1 > v2);
        
        REQUIRE(v1 <= v5);
        REQUIRE(v1 <= v2);
        REQUIRE_FALSE(v2 <= v1);
        
        REQUIRE(v1 >= v5);
        REQUIRE(v2 >= v1);
        REQUIRE_FALSE(v1 >= v2);
    }
    
    SECTION("Version to string conversion") {
        // 测试普通版本转字符串
        Version version(3, 4, 5);
        std::string versionStr = version.string();
        REQUIRE(versionStr == "3.4.5");
        
        // 测试输出流运算符
        std::stringstream ss;
        ss << version;
        REQUIRE(ss.str() == "3.4.5");
        
        // 测试无补丁版本
        Version v2(1, 2, 0);
        REQUIRE(v2.string() == "1.2.0");
    }
}

TEST_CASE("Library version", "[version]") {
    SECTION("Get library version") {
        // 获取库当前版本
        Version libraryVersion = version();
        
        // 验证版本号匹配常量
        REQUIRE(libraryVersion.major == VERSION_MAJOR);
        REQUIRE(libraryVersion.minor == VERSION_MINOR);
        REQUIRE(libraryVersion.patch == VERSION_PATCH);
        
        // 验证版本字符串匹配
        REQUIRE(libraryVersion.string() == VERSION_STRING);
    }
} 