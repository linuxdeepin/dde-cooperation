#include <gtest/gtest.h>
#include "threads/thread.h"
#include "threads/mutex.h"
#include "time/timestamp.h"
#include <atomic>
#include <vector>
#include <future>

using namespace BaseKit;

// 测试线程标识符
TEST(ThreadTest, ThreadId) {
    // 获取当前线程ID
    uint64_t currentId = Thread::CurrentThreadId();
    EXPECT_NE(currentId, 0);

    // 宏应该返回相同的值
    EXPECT_EQ(__THREAD__, currentId);

    // 启动一个新线程，并检查其ID是否不同
    std::atomic<uint64_t> threadId(0);
    std::thread t([&threadId]() {
        threadId = Thread::CurrentThreadId();
    });
    t.join();

    EXPECT_NE(threadId, 0);
    EXPECT_NE(threadId, currentId);
}

// 测试线程休眠
TEST(ThreadTest, ThreadSleep) {
    // 测试休眠一小段时间
    auto start = UtcTimestamp();
    Thread::Sleep(100); // 休眠100毫秒
    auto end = UtcTimestamp();
    auto diff = end - start;
    
    // 确保至少过去了接近100毫秒（允许一定误差）
    EXPECT_GE(diff.milliseconds(), 90);
    
    // 测试使用Timespan休眠
    start = UtcTimestamp();
    Thread::SleepFor(Timespan::milliseconds(100));
    end = UtcTimestamp();
    diff = end - start;
    
    EXPECT_GE(diff.milliseconds(), 90);
    
    // 测试休眠到特定时间点
    auto wakeupTime = UtcTimestamp() + Timespan::milliseconds(100);
    Thread::SleepUntil(wakeupTime);
    end = UtcTimestamp();
    
    // 确保现在的时间至少达到了指定的唤醒时间
    EXPECT_GE(end, wakeupTime);
}

// 测试线程启动
TEST(ThreadTest, ThreadStart) {
    // 测试启动一个执行简单任务的线程
    std::atomic<bool> threadRun(false);
    std::thread t = Thread::Start([&threadRun]() {
        threadRun = true;
    });
    
    t.join();
    EXPECT_TRUE(threadRun);
    
    // 测试启动带参数的线程
    std::atomic<int> result(0);
    std::thread t2 = Thread::Start([](std::atomic<int>& res, int value) {
        res = value;
    }, std::ref(result), 42);
    
    t2.join();
    EXPECT_EQ(result, 42);
}

// 测试线程优先级
TEST(ThreadTest, ThreadPriority) {
    // 测试设置和获取当前线程优先级
    // 注意：更改线程优先级可能需要特殊权限，因此这个测试可能会在某些系统上失败
    
    try {
        ThreadPriority originalPriority = Thread::GetPriority();
        
        // 尝试设置为正常优先级
        Thread::SetPriority(ThreadPriority::NORMAL);
        EXPECT_EQ(Thread::GetPriority(), ThreadPriority::NORMAL);
        
        // 尝试设置为较高优先级
        Thread::SetPriority(ThreadPriority::HIGH);
        
        // 恢复原始优先级
        Thread::SetPriority(originalPriority);
    } catch (const Exception& ex) {
        // 在没有权限的情况下可能会失败，但不应该中断测试
        std::cout << "线程优先级测试跳过: " << ex.message() << std::endl;
    }
}

// 测试线程 CPU 亲和性
TEST(ThreadTest, ThreadAffinity) {
    // 测试获取当前线程的 CPU 亲和性
    try {
        std::bitset<64> originalAffinity = Thread::GetAffinity();
        EXPECT_NE(originalAffinity.count(), 0);
        
        // 尝试设置为单核心亲和性（使用第一个可用核心）
        if (originalAffinity.count() > 1) {
            std::bitset<64> singleCore;
            for (size_t i = 0; i < 64; ++i) {
                if (originalAffinity.test(i)) {
                    singleCore.set(i);
                    break;
                }
            }
            
            if (singleCore.any()) {
                Thread::SetAffinity(singleCore);
                std::bitset<64> newAffinity = Thread::GetAffinity();
                EXPECT_EQ(newAffinity, singleCore);
            }
        }
        
        // 恢复原始亲和性
        Thread::SetAffinity(originalAffinity);
    } catch (const Exception& ex) {
        // 在没有权限的情况下可能会失败，但不应该中断测试
        std::cout << "线程亲和性测试跳过: " << ex.message() << std::endl;
    }
}

// 测试线程让出（Yield）
TEST(ThreadTest, ThreadYield) {
    // 这个测试主要是确保 Yield 方法不会导致崩溃
    // 实际上很难测试 Yield 的效果
    
    // 简单测试调用不会崩溃
    Thread::Yield();
    
    // 尝试在多线程环境中测试 Yield
    constexpr int ThreadCount = 4;
    constexpr int YieldCount = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> counter(0);
    Mutex mutex;
    
    for (int i = 0; i < ThreadCount; ++i) {
        threads.emplace_back(Thread::Start([&]() {
            for (int j = 0; j < YieldCount; ++j) {
                {
                    BaseKit::Locker<BaseKit::Mutex> lock(mutex);
                    counter++;
                }
                Thread::Yield();
            }
        }));
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(counter, ThreadCount * YieldCount);
}

// 测试线程与异常处理集成
TEST(ThreadTest, ThreadWithExceptions) {
    // 测试在线程中抛出异常能够被正确捕获
    
    std::promise<std::string> exceptionMessage;
    std::future<std::string> futureMessage = exceptionMessage.get_future();
    
    std::thread t = Thread::Start([&exceptionMessage]() {
        try {
            throw std::runtime_error("测试线程异常");
        } catch (const std::exception& ex) {
            exceptionMessage.set_value(ex.what());
        }
    });
    
    t.join();
    std::string message = futureMessage.get();
    EXPECT_EQ(message, "测试线程异常");
} 