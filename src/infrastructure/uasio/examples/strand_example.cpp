// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../src/asio.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include <iomanip>

// 用于打印的互斥锁，避免多线程输出混乱
std::mutex cout_mutex;

// 获取当前时间的字符串表示
std::string get_time_string() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch() % std::chrono::seconds(1)).count();
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%H:%M:%S") << "." 
       << std::setfill('0') << std::setw(3) << ms;
    return ss.str();
}

// 线程安全的打印函数
template <typename... Args>
void safe_print(Args&&... args) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << get_time_string() << " [" << std::this_thread::get_id() << "] ";
    (std::cout << ... << args) << std::endl;
}

// 共享计数器
struct Counter {
    int value = 0;
    
    // 在没有strand保护时，多线程访问会导致数据竞争
    void increment() {
        int old = value;
        // 模拟一些耗时操作，增加线程切换的可能性
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        value = old + 1;
    }
    
    int get() const {
        return value;
    }
};

int main() {
    try {
        // 创建 IO 上下文
        asio::io_context io;
        
        // 创建工作守卫，防止 IO 上下文过早退出
        auto work = io.make_work_guard();
        
        // 创建多个线程运行IO上下文
        std::vector<std::thread> threads;
        const int thread_count = 4;
        
        safe_print("启动 ", thread_count, " 个IO线程");
        
        for (int i = 0; i < thread_count; ++i) {
            threads.emplace_back([&io, i]() {
                safe_print("IO线程 ", i, " 开始运行");
                io.run();
                safe_print("IO线程 ", i, " 结束运行");
            });
        }
        
        // 演示没有strand保护的情况
        {
            safe_print("===== 不使用strand的情况 =====");
            Counter counter;
            const int task_count = 100;
            
            safe_print("初始计数器值: ", counter.get());
            
            // 创建多个任务同时访问计数器
            for (int i = 0; i < task_count; ++i) {
                io.post([&counter, i]() {
                    counter.increment();
                    if ((i + 1) % 20 == 0 || i == task_count - 1) {
                        safe_print("计数器当前值: ", counter.get(), " (应该是 ", i + 1, ")");
                    }
                });
            }
            
            // 等待所有任务完成
            std::this_thread::sleep_for(std::chrono::seconds(2));
            safe_print("不使用strand的最终计数器值: ", counter.get(), " (应该是 ", task_count, ")");
            safe_print("实际可能小于预期值，因为存在数据竞争!");
        }
        
        // 演示使用strand保护的情况
        {
            safe_print("\n===== 使用strand的情况 =====");
            Counter counter;
            const int task_count = 100;
            
            // 创建strand
            asio::strand strand(io);
            
            safe_print("初始计数器值: ", counter.get());
            
            // 创建多个通过strand运行的任务访问计数器
            for (int i = 0; i < task_count; ++i) {
                strand.post([&counter, i]() {
                    counter.increment();
                    if ((i + 1) % 20 == 0 || i == task_count - 1) {
                        safe_print("计数器当前值: ", counter.get(), " (应该是 ", i + 1, ")");
                    }
                });
            }
            
            // 等待所有任务完成
            std::this_thread::sleep_for(std::chrono::seconds(2));
            safe_print("使用strand的最终计数器值: ", counter.get(), " (应该是 ", task_count, ")");
        }
        
        // 演示strand中的嵌套调度
        {
            safe_print("\n===== strand中的嵌套调度 =====");
            
            // 创建strand
            asio::strand strand(io);
            
            // 在strand中使用dispatch和post的区别
            strand.post([&strand]() {
                safe_print("1. 第一个任务开始执行");
                
                // 使用dispatch，如果当前线程已在strand中，会立即执行
                strand.dispatch([]() {
                    safe_print("2. 通过dispatch调度的任务立即执行");
                });
                
                // 使用post，总是将任务加入队列末尾
                strand.post([]() {
                    safe_print("4. 通过post调度的任务稍后执行");
                });
                
                safe_print("3. 第一个任务结束执行");
            });
            
            // 等待所有任务完成
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // 停止IO上下文并等待所有线程完成
        safe_print("\n===== 完成测试，清理资源 =====");
        work.reset();
        io.stop();
        
        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
        safe_print("所有IO线程已完成");
        
    } catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 