#include <catch2/catch_all.hpp>
#include <logging/layout.h>
#include <logging/layouts.h>
#include <logging/record.h>
#include <string>
#include <memory>
#include <sstream>

using namespace Logging;

// 辅助函数：创建测试记录
Record createTestLayoutRecord(Level level, const std::string& message) {
    Record record;
    record.timestamp = 123456789; // 固定时间戳以便测试
    record.thread = 1;            // 固定线程ID以便测试
    record.level = level;
    record.logger = "TestLogger";
    record.message = message;
    return record;
}

// 辅助函数：将Level转换为字符串
std::string levelToString(const Level& level) {
    std::ostringstream oss;
    oss << level;
    return oss.str();
}

// TextLayout 测试
TEST_CASE("Text Layout functionality", "[layout]") {
    SECTION("Format record with text layout") {
        // 创建文本布局
        auto layout = std::make_shared<TextLayout>();
        
        // 创建测试记录
        auto record = createTestLayoutRecord(Level::INFO, "Test message");
        
        // 格式化记录
        Record recordCopy = record;
        layout->LayoutRecord(recordCopy);
        
        // 将原始数据转换为字符串
        std::string formatted(reinterpret_cast<const char*>(recordCopy.raw.data()), recordCopy.raw.size());
        
        // 验证格式化结果包含必要的信息
        REQUIRE(formatted.find("INFO") != std::string::npos);
        REQUIRE(formatted.find("Test message") != std::string::npos);
    }
}

TEST_CASE("Layout customization", "[layout]") {
    SECTION("Custom layout implementation") {
        // 创建自定义布局类
        class CustomLayoutImpl : public Layout {
        public:
            CustomLayoutImpl() = default;
            
            void LayoutRecord(Record& record) override {
                std::string formatted = "CUSTOM: [" + levelToString(record.level) + "] " + record.message;
                record.raw.assign(formatted.begin(), formatted.end());
            }
        };
        
        // 创建自定义布局实例
        auto layout = std::make_shared<CustomLayoutImpl>();
        
        // 创建测试记录
        auto record = createTestLayoutRecord(Level::INFO, "Custom layout test");
        
        // 格式化记录
        Record recordCopy = record;
        layout->LayoutRecord(recordCopy);
        
        // 将原始数据转换为字符串
        std::string formatted(reinterpret_cast<const char*>(recordCopy.raw.data()), recordCopy.raw.size());
        
        // 验证格式化结果符合预期
        REQUIRE(formatted == "CUSTOM: [INFO] Custom layout test");
    }
}

TEST_CASE("CustomLayout functionality", "[layout]") {
    SECTION("Format record with custom layout using lambda") {
        // 创建使用lambda的自定义布局
        auto layout = std::make_shared<CustomLayout>(
            [](const Record& record) -> std::string {
                return "LAMBDA: [" + levelToString(record.level) + "] " + record.logger + " - " + record.message;
            }
        );
        
        // 创建测试记录
        auto record = createTestLayoutRecord(Level::WARN, "Warning message");
        
        // 应用布局
        Record recordCopy = record;
        layout->LayoutRecord(recordCopy);
        
        // 将原始数据转换为字符串
        std::string result(reinterpret_cast<const char*>(recordCopy.raw.data()), recordCopy.raw.size());
        
        // 验证raw字段包含正确的格式化字符串
        std::string expected = "LAMBDA: [WARN] TestLogger - Warning message";
        REQUIRE(result == expected);
    }
    
    SECTION("Format record with custom layout using function object") {
        // 创建格式化函数对象
        struct CustomFormatter {
            std::string operator()(const Record& record) {
                return "FUNCTOR: [" + levelToString(record.level) + "] " + record.message;
            }
        };
        
        // 创建使用函数对象的自定义布局
        auto layout = std::make_shared<CustomLayout>(CustomFormatter());
        
        // 创建测试记录
        auto record = createTestLayoutRecord(Level::ERROR, "Error message");
        
        // 应用布局
        Record recordCopy = record;
        layout->LayoutRecord(recordCopy);
        
        // 将原始数据转换为字符串
        std::string result(reinterpret_cast<const char*>(recordCopy.raw.data()), recordCopy.raw.size());
        
        // 验证raw字段包含正确的格式化字符串
        std::string expected = "FUNCTOR: [ERROR] Error message";
        REQUIRE(result == expected);
    }
} 