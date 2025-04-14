#include <catch2/catch_all.hpp>
#include <logging/trigger.h>
#include <logging/record.h>
#include <logging/level.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

using namespace Logging;

// 创建一个简单的测试触发器
class TestTrigger : public Trigger {
public:
    TestTrigger() : Trigger(), triggered(false) {}
    
    bool OnRecord(const Record& record) override {
        // 记录调用
        recordsCalled++;
        
        // 当收到错误或更高级别的日志时触发
        if (record.level >= Level::ERROR) {
            triggered = true;
            lastTriggeredRecord = record;
            return true; // 确认触发
        }
        return false; // 不触发
    }
    
    bool triggered;
    Record lastTriggeredRecord;
    std::atomic<int> recordsCalled{0};
};

TEST_CASE("Basic Trigger functionality", "[trigger]") {
    SECTION("Trigger activation conditions") {
        // 创建测试触发器
        auto trigger = std::make_shared<TestTrigger>();
        
        // 创建各种级别的记录
        Record debugRecord;
        debugRecord.level = Level::DEBUG;
        debugRecord.message = "Debug message";
        
        Record infoRecord;
        infoRecord.level = Level::INFO;
        infoRecord.message = "Info message";
        
        Record errorRecord;
        errorRecord.level = Level::ERROR;
        errorRecord.message = "Error message";
        
        Record fatalRecord;
        fatalRecord.level = Level::FATAL;
        fatalRecord.message = "Fatal message";
        
        // 测试不应该触发的记录
        REQUIRE_FALSE(trigger->OnRecord(debugRecord));
        REQUIRE_FALSE(trigger->OnRecord(infoRecord));
        REQUIRE_FALSE(trigger->triggered);
        
        // 测试应该触发的记录
        REQUIRE(trigger->OnRecord(errorRecord));
        REQUIRE(trigger->triggered);
        REQUIRE(trigger->lastTriggeredRecord.message == "Error message");
        
        // 重置触发器
        trigger->triggered = false;
        
        // 测试另一个应该触发的记录
        REQUIRE(trigger->OnRecord(fatalRecord));
        REQUIRE(trigger->triggered);
        REQUIRE(trigger->lastTriggeredRecord.message == "Fatal message");
    }
}

// 测试时间触发器 - 在指定时间间隔后触发
class TimeTrigger : public Trigger {
public:
    explicit TimeTrigger(std::chrono::milliseconds interval)
        : Trigger(), interval(interval), lastTriggerTime(std::chrono::steady_clock::now()) {}
    
    bool OnRecord(const Record& record) override {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTriggerTime);
        
        if (elapsed >= interval) {
            lastTriggerTime = now;
            lastRecord = record;
            triggerCount++;
            return true;
        }
        return false;
    }
    
    std::chrono::milliseconds interval;
    std::chrono::steady_clock::time_point lastTriggerTime;
    Record lastRecord;
    int triggerCount = 0;
};

TEST_CASE("Time-based Trigger", "[trigger]") {
    SECTION("Trigger based on time interval") {
        // 创建一个每100毫秒触发一次的触发器
        auto trigger = std::make_shared<TimeTrigger>(std::chrono::milliseconds(100));
        
        // 创建一些测试记录
        Record record;
        record.level = Level::INFO;
        record.message = "Time trigger test";
        
        // 第一次应该触发
        REQUIRE(trigger->OnRecord(record));
        REQUIRE(trigger->triggerCount == 1);
        
        // 立即再次调用不应该触发
        REQUIRE_FALSE(trigger->OnRecord(record));
        REQUIRE(trigger->triggerCount == 1);
        
        // 等待时间间隔后再次调用应该触发
        std::this_thread::sleep_for(std::chrono::milliseconds(110));
        REQUIRE(trigger->OnRecord(record));
        REQUIRE(trigger->triggerCount == 2);
    }
}

// 计数触发器 - 在收到指定数量的记录后触发
class CountTrigger : public Trigger {
public:
    explicit CountTrigger(int threshold) : Trigger(), threshold(threshold), count(0) {}
    
    bool OnRecord(const Record& record) override {
        count++;
        if (count >= threshold) {
            count = 0; // 重置计数
            return true;
        }
        return false;
    }
    
    int threshold;
    std::atomic<int> count;
};

TEST_CASE("Count-based Trigger", "[trigger]") {
    SECTION("Trigger after receiving specific number of records") {
        // 创建一个每收到3条记录触发一次的触发器
        auto trigger = std::make_shared<CountTrigger>(3);
        
        // 创建测试记录
        Record record;
        record.level = Level::INFO;
        record.message = "Count trigger test";
        
        // 前两次调用不应该触发
        REQUIRE_FALSE(trigger->OnRecord(record));
        REQUIRE_FALSE(trigger->OnRecord(record));
        
        // 第三次调用应该触发
        REQUIRE(trigger->OnRecord(record));
        
        // 计数应该被重置，接下来两次调用不应该触发
        REQUIRE_FALSE(trigger->OnRecord(record));
        REQUIRE_FALSE(trigger->OnRecord(record));
        
        // 第三次调用应该再次触发
        REQUIRE(trigger->OnRecord(record));
    }
}

// 组合触发器 - 当任一子触发器满足条件时触发
class CompositeTrigger : public Trigger {
public:
    CompositeTrigger() : Trigger() {}
    
    void AddTrigger(std::shared_ptr<Trigger> trigger) {
        triggers.push_back(trigger);
    }
    
    bool OnRecord(const Record& record) override {
        for (auto& trigger : triggers) {
            if (trigger->OnRecord(record)) {
                return true;
            }
        }
        return false;
    }
    
private:
    std::vector<std::shared_ptr<Trigger>> triggers;
};

TEST_CASE("Composite Trigger", "[trigger]") {
    SECTION("Combine multiple triggers") {
        // 创建一个组合触发器
        auto compositeTrigger = std::make_shared<CompositeTrigger>();
        
        // 添加一个级别触发器
        auto levelTrigger = std::make_shared<TestTrigger>();
        compositeTrigger->AddTrigger(levelTrigger);
        
        // 添加一个计数触发器
        auto countTrigger = std::make_shared<CountTrigger>(2);
        compositeTrigger->AddTrigger(countTrigger);
        
        // 创建测试记录
        Record infoRecord;
        infoRecord.level = Level::INFO;
        infoRecord.message = "Info message";
        
        Record errorRecord;
        errorRecord.level = Level::ERROR;
        errorRecord.message = "Error message";
        
        // 第一次INFO调用不应该触发
        REQUIRE_FALSE(compositeTrigger->OnRecord(infoRecord));
        
        // 第二次INFO调用应该通过计数触发器触发
        REQUIRE(compositeTrigger->OnRecord(infoRecord));
        
        // ERROR调用应该通过级别触发器触发
        REQUIRE(compositeTrigger->OnRecord(errorRecord));
    }
} 