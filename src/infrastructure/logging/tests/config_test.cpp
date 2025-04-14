#include <catch2/catch_all.hpp>
#include <logging/config.h>
#include <logging/logger.h>
#include <logging/appenders.h>
#include <logging/layouts.h>
#include <memory>
#include <string>
#include <filesystem>
#include <fstream>

using namespace Logging;

// 辅助函数来创建测试配置文件
void createTestConfigFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    file << content;
    file.close();
}

// 清理测试文件
void removeTestFile(const std::string& filename) {
    if (std::filesystem::exists(filename)) {
        std::filesystem::remove(filename);
    }
}

TEST_CASE("Config basic functionality", "[config]") {
    SECTION("Static configuration methods") {
        // 测试Config静态方法的可用性
        
        // 测试创建默认日志器
        Logger defaultLogger = Config::CreateLogger();
        
        // 测试创建命名日志器
        Logger namedLogger = Config::CreateLogger("TestLogger");
        
        // 向日志中写入一些信息
        defaultLogger.Info("Default logger test message");
        namedLogger.Info("Named logger test message");
        
        // 启动和关闭日志基础设施
        Config::Startup();
        Config::Shutdown();
    }
}

TEST_CASE("Logger configuration", "[config]") {
    SECTION("Configure default logger") {
        // 创建一个处理器，使用文本布局
        auto layout = std::make_shared<TextLayout>();
        auto processor = std::make_shared<SyncProcessor>(layout);
        processor->appenders().push_back(std::make_shared<ConsoleAppender>());
        
        // 配置默认日志器
        Config::ConfigLogger(processor);
        
        // 创建默认日志器的实例
        Logger logger = Config::CreateLogger();
        
        // 写入一些日志
        logger.Info("This is a test message to console from default logger");
        
        // 刷新
        logger.Flush();
    }
    
    SECTION("Configure named logger") {
        // 创建一个处理器，使用文本布局
        auto layout = std::make_shared<TextLayout>();
        auto processor = std::make_shared<SyncProcessor>(layout);
        processor->appenders().push_back(std::make_shared<ConsoleAppender>());
        
        // 配置命名日志器
        Config::ConfigLogger("NamedLogger", processor);
        
        // 创建命名日志器的实例
        Logger logger = Config::CreateLogger("NamedLogger");
        
        // 写入一些日志
        logger.Info("This is a test message to console from named logger");
        
        // 刷新
        logger.Flush();
    }
}

// 注释掉使用了不存在API的测试
/*
TEST_CASE("Configuration serialization", "[config]") {
    SECTION("ToString produces a non-empty string") {
        Config& config = Config::Get();
        
        std::string configStr = config.ToString();
        REQUIRE(!configStr.empty());
    }
    
    SECTION("Configuration load from string") {
        Config& config = Config::Get();
        
        // 创建一个简单的配置字符串
        std::string configStr = R"(
        {
            "processors": [
                {
                    "name": "test_console_processor",
                    "appenders": [
                        {
                            "type": "ConsoleAppender"
                        }
                    ]
                }
            ],
            "loggers": [
                {
                    "name": "test_logger",
                    "processor": "test_console_processor"
                }
            ]
        }
        )";
        
        // 尝试从字符串加载配置
        REQUIRE_NOTHROW(config.FromString(configStr));
        
        // 获取并使用配置的日志器
        Logger logger("test_logger");
        logger.Info("Test message after config load");
        logger.Flush();
    }
}

TEST_CASE("Configuration update and dynamic changes", "[config]") {
    SECTION("Update logger processor") {
        Config& config = Config::Get();
        
        // 创建两个处理器
        auto processor1 = config.CreateProcessor("Processor1");
        processor1->appenders().push_back(config.CreateAppender<ConsoleAppender>());
        
        auto processor2 = config.CreateProcessor("Processor2");
        processor2->appenders().push_back(config.CreateAppender<ConsoleAppender>());
        
        // 创建一个日志器，使用第一个处理器
        config.CreateLogger("DynamicLogger", processor1);
        
        // 获取日志器并使用
        Logger logger("DynamicLogger");
        logger.Info("Using processor 1");
        
        // 更新日志器的处理器
        config.UpdateLogger("DynamicLogger", processor2);
        
        // 更新日志器
        logger.Update();
        logger.Info("Using processor 2");
        
        // 刷新
        logger.Flush();
    }
}
*/ 