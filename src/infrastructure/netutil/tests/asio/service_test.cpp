#include <catch2/catch_all.hpp>
#include <asio/service.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>

using namespace NetUtil::Asio;

TEST_CASE("Service basic functionality", "[service]") {
    SECTION("Create and stop service") {
        auto service = std::make_shared<Service>();
        // 启动服务
        REQUIRE(service->Start());
        REQUIRE(service->IsStarted());
        
        // 停止服务
        REQUIRE(service->Stop());
        REQUIRE_FALSE(service->IsStarted());
    }
    
    SECTION("Test service with default thread count") {
        auto service = std::make_shared<Service>();
        
        // 默认线程数应该大于0
        REQUIRE(service->threads() > 0);
        
        service->Start();
        service->Stop();
    }
    
    SECTION("Test service with custom thread count") {
        const size_t CUSTOM_THREADS = 4;
        auto service = std::make_shared<Service>(CUSTOM_THREADS);
        
        REQUIRE(service->threads() == CUSTOM_THREADS);
        
        service->Start();
        service->Stop();
    }
    
    SECTION("Test post function") {
        auto service = std::make_shared<Service>();
        service->Start();
        
        std::mutex mutex;
        std::condition_variable cv;
        bool taskExecuted = false;
        
        // 发布一个任务到IO服务
        service->Post([&]() {
            std::lock_guard<std::mutex> lock(mutex);
            taskExecuted = true;
            cv.notify_one();
        });
        
        // 等待任务执行
        {
            std::unique_lock<std::mutex> lock(mutex);
            REQUIRE(cv.wait_for(lock, std::chrono::seconds(2), [&]() { return taskExecuted; }));
        }
        
        service->Stop();
    }
    
    SECTION("Test dispatch function") {
        auto service = std::make_shared<Service>();
        service->Start();
        
        std::mutex mutex;
        std::condition_variable cv;
        bool taskExecuted = false;
        
        // 分派一个任务到IO服务
        service->Dispatch([&]() {
            std::lock_guard<std::mutex> lock(mutex);
            taskExecuted = true;
            cv.notify_one();
        });
        
        // 等待任务执行
        {
            std::unique_lock<std::mutex> lock(mutex);
            REQUIRE(cv.wait_for(lock, std::chrono::seconds(2), [&]() { return taskExecuted; }));
        }
        
        service->Stop();
    }
    
    SECTION("Test strand execution") {
        auto service = std::make_shared<Service>();
        service->Start();
        auto io_service = service->GetAsioService();
        asio::io_service::strand strand(*io_service);
        
        std::mutex mutex;
        std::vector<int> executed_tasks;
        std::condition_variable cv;
        std::atomic<int> taskCount{0};
        
        // 使用strand确保按顺序执行多个任务
        for (int i = 0; i < 10; ++i) {
            strand.post([i, &mutex, &executed_tasks, &taskCount, &cv]() {
                // 添加随机延迟来测试顺序保证
                std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 10));
                
                std::lock_guard<std::mutex> lock(mutex);
                executed_tasks.push_back(i);
                taskCount++;
                
                if (taskCount == 10) {
                    cv.notify_one();
                }
            });
        }
        
        // 等待所有任务完成
        {
            std::unique_lock<std::mutex> lock(mutex);
            REQUIRE(cv.wait_for(lock, std::chrono::seconds(5), [&]() { return taskCount == 10; }));
            
            // 验证任务按顺序执行
            for (int i = 0; i < 10; ++i) {
                REQUIRE(executed_tasks[i] == i);
            }
        }
        
        service->Stop();
    }
    
    SECTION("Test service under high load") {
        auto service = std::make_shared<Service>(4); // 使用4个线程
        service->Start();
        
        std::mutex mutex;
        std::atomic<int> completedTasks{0};
        std::condition_variable cv;
        const int TASK_COUNT = 1000;
        
        // 提交大量任务
        for (int i = 0; i < TASK_COUNT; ++i) {
            service->Post([&completedTasks, &cv]() {
                // 模拟一些工作
                std::this_thread::sleep_for(std::chrono::microseconds(rand() % 1000));
                
                if (++completedTasks == TASK_COUNT) {
                    cv.notify_one();
                }
            });
        }
        
        // 等待所有任务完成
        {
            std::unique_lock<std::mutex> lock(mutex);
            REQUIRE(cv.wait_for(lock, std::chrono::seconds(10), [&]() { return completedTasks == TASK_COUNT; }));
            REQUIRE(completedTasks == TASK_COUNT);
        }
        
        service->Stop();
    }
} 