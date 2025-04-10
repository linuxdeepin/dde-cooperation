#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include "../src/future.h"
#include "../src/promise.h"
#include "../src/packaged_task.h"
#include "../src/thread_pool.h"

// 示例1: 基本的Future-Promise使用
void basic_future_promise_example() {
    std::cout << "\n== 基本Future-Promise示例 ==" << std::endl;
    
    // 创建Promise
    uasio::promise<int> prom;
    
    // 获取关联的Future
    uasio::future<int> fut = prom.get_future();
    
    // 在单独的线程中设置Promise的值
    std::thread t([&prom]() {
        std::cout << "计算线程开始工作..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "计算完成，设置结果值为 42" << std::endl;
        prom.set_value(42);
    });
    
    // 在主线程中等待结果
    std::cout << "主线程等待结果..." << std::endl;
    int result = fut.get();
    std::cout << "得到结果: " << result << std::endl;
    
    // 等待线程结束
    t.join();
}

// 示例2: 带超时的Future
void future_with_timeout_example() {
    std::cout << "\n== 带超时的Future示例 ==" << std::endl;
    
    uasio::promise<std::string> prom;
    uasio::future<std::string> fut = prom.get_future();
    
    std::thread t([&prom]() {
        std::cout << "长时间任务开始..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "长时间任务完成，设置结果" << std::endl;
        prom.set_value("任务完成");
    });
    
    std::cout << "尝试等待结果，最多等待2秒..." << std::endl;
    
    // 带超时的等待
    if (fut.wait_for(std::chrono::seconds(2))) {
        std::cout << "在超时前得到结果: " << fut.get() << std::endl;
    } else {
        std::cout << "等待超时！" << std::endl;
        
        // 继续等待结果
        std::cout << "继续等待直到结果可用..." << std::endl;
        std::string result = fut.get();
        std::cout << "最终得到结果: " << result << std::endl;
    }
    
    t.join();
}

// 示例3: 异常处理
void future_exception_example() {
    std::cout << "\n== Future异常处理示例 ==" << std::endl;
    
    uasio::promise<int> prom;
    uasio::future<int> fut = prom.get_future();
    
    std::thread t([&prom]() {
        std::cout << "执行可能抛出异常的任务..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        try {
            // 模拟抛出异常
            throw std::runtime_error("模拟错误");
        } catch (...) {
            std::cout << "捕获到异常，传递给Promise" << std::endl;
            prom.set_exception(std::current_exception());
        }
    });
    
    std::cout << "等待任务结果..." << std::endl;
    try {
        int result = fut.get();
        std::cout << "得到结果: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cout << "捕获到异常: " << e.what() << std::endl;
    }
    
    t.join();
}

// 示例4: PackagedTask
void packaged_task_example() {
    std::cout << "\n== PackagedTask示例 ==" << std::endl;
    
    // 创建PackagedTask，包装一个计算平方的函数
    uasio::packaged_task<int(int)> task([](int x) {
        std::cout << "计算 " << x << " 的平方..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return x * x;
    });
    
    // 获取Future
    uasio::future<int> fut = task.get_future();
    
    // 在另一个线程中执行任务
    std::thread t(std::move(task), 8);
    
    std::cout << "等待计算结果..." << std::endl;
    int result = fut.get();
    std::cout << "8的平方 = " << result << std::endl;
    
    t.join();
}

// 示例5: 回调函数
void future_callback_example() {
    std::cout << "\n== Future回调示例 ==" << std::endl;
    
    uasio::promise<int> prom;
    uasio::future<int> fut = prom.get_future();
    
    // 添加回调
    fut.then([](int value) {
        std::cout << "回调1: 值的平方 = " << (value * value) << std::endl;
    });
    
    fut.then([](int value) {
        std::cout << "回调2: 值的立方 = " << (value * value * value) << std::endl;
    });
    
    std::cout << "设置Promise的值..." << std::endl;
    prom.set_value(5);
    
    // 等待所有回调执行完成
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// 示例6: when_all和when_any
void when_all_any_example() {
    std::cout << "\n== when_all和when_any示例 ==" << std::endl;
    
    // 创建线程池
    uasio::thread_pool pool(4);
    pool.run();
    
    // 创建多个Promise
    std::vector<uasio::promise<int>> promises(3);
    std::vector<uasio::future<int>> futures;
    
    // 获取Futures
    for (auto& prom : promises) {
        futures.push_back(prom.get_future());
    }
    
    // 设置一些任务来完成Promise
    pool.post([&promises]() {
        std::cout << "任务1开始..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "任务1完成，设置值为10" << std::endl;
        promises[0].set_value(10);
    });
    
    pool.post([&promises]() {
        std::cout << "任务2开始..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "任务2完成，设置值为20" << std::endl;
        promises[1].set_value(20);
    });
    
    pool.post([&promises]() {
        std::cout << "任务3开始..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout << "任务3完成，设置值为30" << std::endl;
        promises[2].set_value(30);
    });
    
    // 使用when_any等待任何一个完成
    std::cout << "等待任何一个任务完成..." << std::endl;
    uasio::future<std::size_t> any_future = uasio::when_any(futures.begin(), futures.end());
    std::size_t index = any_future.get();
    std::cout << "任务 " << (index + 1) << " 首先完成，值为: " 
              << futures[index].get() << std::endl;
    
    // 使用when_all等待所有完成
    std::cout << "等待所有任务完成..." << std::endl;
    auto all_future = uasio::when_all(futures.begin(), futures.end());
    auto results = all_future.get();
    
    std::cout << "所有任务完成，结果: ";
    int sum = 0;
    for (auto& f : results) {
        int val = f.get();
        sum += val;
        std::cout << val << " ";
    }
    std::cout << "\n总和: " << sum << std::endl;
    
    // 停止线程池
    pool.stop();
    pool.join();
}

// 示例7: make_ready_future
void make_ready_future_example() {
    std::cout << "\n== make_ready_future示例 ==" << std::endl;
    
    // 创建已就绪的future
    auto ready_fut = uasio::make_ready_future(100);
    std::cout << "已就绪future的值: " << ready_fut.get() << std::endl;
    
    // 创建带异常的future
    auto exceptional_fut = uasio::make_exceptional_future<int>(
        std::runtime_error("预设异常"));
    
    try {
        int val = exceptional_fut.get();
        std::cout << "值: " << val << std::endl;
    } catch (const std::exception& e) {
        std::cout << "捕获到预设异常: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "=== Future和Promise示例 ===" << std::endl;
    
    // 运行各个示例
    basic_future_promise_example();
    future_with_timeout_example();
    future_exception_example();
    packaged_task_example();
    future_callback_example();
    when_all_any_example();
    make_ready_future_example();
    
    std::cout << "\n所有示例完成！" << std::endl;
    return 0;
} 