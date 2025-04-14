#include <catch2/catch_all.hpp>
#include <logging/logger.h>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

using namespace Logging;

// 辅助函数，检查给定字符串是否包含指定的子字符串
bool contains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

TEST_CASE("Logger construction", "[logger]") {
    SECTION("Default logger construction") {
        Logger logger;
        // 不做具体的断言，只验证构造不会抛出异常
    }
    
    SECTION("Named logger construction") {
        Logger logger("TestLogger");
        // 不做具体的断言，只验证构造不会抛出异常
    }
    
    SECTION("Copy construction") {
        Logger logger1("TestLogger");
        Logger logger2(logger1);
        // 不做具体的断言，只验证构造不会抛出异常
    }
    
    SECTION("Move construction") {
        Logger logger1("TestLogger");
        Logger logger2(std::move(logger1));
        // 不做具体的断言，只验证构造不会抛出异常
    }
    
    SECTION("Copy assignment") {
        Logger logger1("TestLogger");
        Logger logger2;
        logger2 = logger1;
        // 不做具体的断言，只验证赋值不会抛出异常
    }
    
    SECTION("Move assignment") {
        Logger logger1("TestLogger");
        Logger logger2;
        logger2 = std::move(logger1);
        // 不做具体的断言，只验证赋值不会抛出异常
    }
}

TEST_CASE("Logger basic functionality", "[logger]") {
    // 注意：这些测试主要检查方法是否能成功调用，而不是具体输出
    // 因为真正的输出取决于当前的日志配置（如是否输出到控制台、文件等）
    
    SECTION("Log with different levels") {
        Logger logger("TestLogger");
        
        // 不同级别的日志记录
        logger.Debug("Debug message");
        logger.Info("Info message");
        logger.Warn("Warning message");
        logger.Error("Error message");
        logger.Fatal("Fatal message");
        
        // 带参数的日志记录
        logger.Debug("Debug message with param: {}", 123);
        logger.Info("Info message with params: {} and {}", "param1", 42);
        logger.Warn("Warning message with params: {}, {} and {}", 1, "two", 3.0);
        logger.Error("Error message with param: {}", "error details");
        logger.Fatal("Fatal message with param: {}", "critical error");
        
        // 刷新日志
        logger.Flush();
    }
    
    SECTION("Update logger") {
        Logger logger("TestLogger");
        
        // 记录一些日志
        logger.Info("Info before update");
        
        // 更新logger（获取最新的配置）
        logger.Update();
        
        // 记录更新后的日志
        logger.Info("Info after update");
    }
}

// 测试多线程情况下的日志记录
TEST_CASE("Logger thread safety", "[logger]") {
    SECTION("Multiple threads logging") {
        Logger logger("ThreadTestLogger");
        
        // 创建多个线程，每个线程记录一些日志
        const int threadCount = 5;
        const int messagesPerThread = 100;
        
        std::vector<std::thread> threads;
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back([&logger, i, messagesPerThread]() {
                for (int j = 0; j < messagesPerThread; ++j) {
                    logger.Info("Thread {} message {}", i, j);
                    
                    // 小幅度随机延迟，增加并发可能性
                    if (j % 10 == 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }
            });
        }
        
        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }
        
        // 确保所有日志都写入
        logger.Flush();
    }
}

// 异常情况下的日志记录测试
TEST_CASE("Logger exceptional cases", "[logger]") {
    SECTION("Very long log message") {
        Logger logger("LongMessageLogger");
        
        // 创建一个非常长的消息
        std::string longMessage(10000, 'a');
        
        // 记录长消息
        logger.Info(longMessage);
        logger.Info("Long message with param: {}", longMessage);
        
        // 刷新确保写入
        logger.Flush();
    }
    
    SECTION("Special characters in log message") {
        Logger logger("SpecialCharLogger");
        
        // 包含特殊字符的消息
        logger.Info("Message with new lines\nand tabs\t and quotes \" ' ");
        logger.Info("Message with format placeholders {{ and }}");
        logger.Info("Message with non-ASCII characters: 中文, Русский, 日本語");
        
        // 刷新确保写入
        logger.Flush();
    }
} 