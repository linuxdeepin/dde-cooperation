#include <catch2/catch_all.hpp>
#include <logging/record.h>
#include <string>

using namespace Logging;

TEST_CASE("Record construction and properties", "[record]") {
    SECTION("Default construction") {
        Record record;
        
        // 验证默认值
        REQUIRE(record.timestamp > 0); // 时间戳应该被设置为当前时间
        REQUIRE(record.thread > 0);    // 线程ID应该是当前线程
        REQUIRE(record.level == Level::NONE);
        REQUIRE(record.logger.empty());
        REQUIRE(record.message.empty());
        REQUIRE(record.buffer.empty());
        REQUIRE(record.raw.empty());
    }
    
    SECTION("Record copying") {
        Record record1;
        record1.timestamp = 12345;
        record1.thread = 67890;
        record1.level = Level::INFO;
        record1.logger = "TestLogger";
        record1.message = "Test message";
        
        // 测试拷贝构造
        Record record2(record1);
        REQUIRE(record2.timestamp == 12345);
        REQUIRE(record2.thread == 67890);
        REQUIRE(record2.level == Level::INFO);
        REQUIRE(record2.logger == "TestLogger");
        REQUIRE(record2.message == "Test message");
        
        // 测试拷贝赋值
        Record record3;
        record3 = record1;
        REQUIRE(record3.timestamp == 12345);
        REQUIRE(record3.thread == 67890);
        REQUIRE(record3.level == Level::INFO);
        REQUIRE(record3.logger == "TestLogger");
        REQUIRE(record3.message == "Test message");
    }
    
    SECTION("Record moving") {
        Record record1;
        record1.timestamp = 12345;
        record1.thread = 67890;
        record1.level = Level::INFO;
        record1.logger = "TestLogger";
        record1.message = "Test message";
        
        // 测试移动构造
        Record record2(std::move(record1));
        REQUIRE(record2.timestamp == 12345);
        REQUIRE(record2.thread == 67890);
        REQUIRE(record2.level == Level::INFO);
        REQUIRE(record2.logger == "TestLogger");
        REQUIRE(record2.message == "Test message");
        
        // 测试移动赋值
        Record record3;
        record3 = std::move(record2);
        REQUIRE(record3.timestamp == 12345);
        REQUIRE(record3.thread == 67890);
        REQUIRE(record3.level == Level::INFO);
        REQUIRE(record3.logger == "TestLogger");
        REQUIRE(record3.message == "Test message");
    }
}

TEST_CASE("Record formatting", "[record]") {
    SECTION("Basic formatting") {
        Record record;
        record.Format("Test {}", "message");
        REQUIRE(record.message == "Test message");
    }
    
    SECTION("Format with multiple arguments") {
        Record record;
        record.Format("Value 1: {}, Value 2: {}", 42, "hello");
        REQUIRE(record.message == "Value 1: 42, Value 2: hello");
    }
    
    SECTION("Store format") {
        Record record;
        record.StoreFormat("Stored {}", "message");
        REQUIRE(record.message == "Stored {}");
        REQUIRE(record.IsFormatStored());
        REQUIRE(record.RestoreFormat() == "Stored message");
    }
}

TEST_CASE("Record clear and swap", "[record]") {
    SECTION("Clear record") {
        Record record;
        record.timestamp = 12345;
        record.thread = 67890;
        record.level = Level::INFO;
        record.logger = "TestLogger";
        record.message = "Test message";
        
        record.Clear();
        
        // 验证清除后的状态
        REQUIRE(record.timestamp > 0); // 时间戳会被重置为当前时间
        REQUIRE(record.thread > 0);    // 线程ID应该是当前线程
        REQUIRE(record.level == Level::NONE);
        REQUIRE(record.logger.empty());
        REQUIRE(record.message.empty());
        REQUIRE(record.buffer.empty());
    }
    
    SECTION("Swap records") {
        Record record1;
        record1.timestamp = 12345;
        record1.thread = 67890;
        record1.level = Level::INFO;
        record1.logger = "TestLogger1";
        record1.message = "Test message 1";
        
        Record record2;
        record2.timestamp = 54321;
        record2.thread = 98760;
        record2.level = Level::ERROR;
        record2.logger = "TestLogger2";
        record2.message = "Test message 2";
        
        // 使用swap方法
        record1.swap(record2);
        
        REQUIRE(record1.timestamp == 54321);
        REQUIRE(record1.thread == 98760);
        REQUIRE(record1.level == Level::ERROR);
        REQUIRE(record1.logger == "TestLogger2");
        REQUIRE(record1.message == "Test message 2");
        
        REQUIRE(record2.timestamp == 12345);
        REQUIRE(record2.thread == 67890);
        REQUIRE(record2.level == Level::INFO);
        REQUIRE(record2.logger == "TestLogger1");
        REQUIRE(record2.message == "Test message 1");
        
        // 使用友元swap函数
        swap(record1, record2);
        
        REQUIRE(record1.timestamp == 12345);
        REQUIRE(record1.thread == 67890);
        REQUIRE(record1.level == Level::INFO);
        REQUIRE(record1.logger == "TestLogger1");
        REQUIRE(record1.message == "Test message 1");
        
        REQUIRE(record2.timestamp == 54321);
        REQUIRE(record2.thread == 98760);
        REQUIRE(record2.level == Level::ERROR);
        REQUIRE(record2.logger == "TestLogger2");
        REQUIRE(record2.message == "Test message 2");
    }
} 