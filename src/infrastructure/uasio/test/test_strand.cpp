#include "../src/asio.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <cassert>

int main() {
    try {
        std::cout << "Strand 测试程序" << std::endl;
        std::cout << "==============" << std::endl;
        
        // 创建 IO 上下文
        uasio::io_context io;
        
        // 创建工作守卫，防止 IO 上下文过早退出
        uasio::io_context::work work(io);
        
        // 创建多个线程运行IO上下文
        std::vector<std::thread> threads;
        const int thread_count = 4;
        
        std::cout << "启动 " << thread_count << " 个IO线程" << std::endl;
        
        for (int i = 0; i < thread_count; ++i) {
            threads.emplace_back([&io]() {
                io.run();
            });
        }
        
        // 测试1: 基本功能 - 顺序执行
        {
            std::cout << "\n测试1: 基本功能 - 顺序执行" << std::endl;
            
            uasio::strand strand(io);
            std::vector<int> results;
            
            for (int i = 0; i < 10; ++i) {
                strand.post([i, &results]() {
                    results.push_back(i);
                });
            }
            
            // 等待所有任务完成
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            // 验证结果按照预期顺序
            bool is_ordered = true;
            for (size_t i = 0; i < results.size(); ++i) {
                if (static_cast<int>(i) != results[i]) {
                    is_ordered = false;
                    break;
                }
            }
            
            std::cout << "顺序执行测试: " << (is_ordered ? "通过" : "失败") << std::endl;
            assert(is_ordered && "顺序执行测试失败");
        }
        
        // 测试2: 嵌套调度 - dispatch vs post
        {
            std::cout << "\n测试2: 嵌套调度 - dispatch vs post" << std::endl;
            
            uasio::strand strand(io);
            std::vector<int> execution_order;
            
            strand.post([&]() {
                execution_order.push_back(1);
                
                // dispatch应该在当前任务结束前执行
                strand.dispatch([&]() {
                    execution_order.push_back(2);
                });
                
                // post应该在当前任务结束后执行
                strand.post([&]() {
                    execution_order.push_back(4);
                });
                
                execution_order.push_back(3);
            });
            
            // 等待所有任务完成
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            // 验证执行顺序是否正确: 1, 2, 3, 4
            bool is_correct_order = true;
            if (execution_order.size() != 4) {
                is_correct_order = false;
            } else {
                for (size_t i = 0; i < execution_order.size(); ++i) {
                    if (static_cast<int>(i + 1) != execution_order[i]) {
                        is_correct_order = false;
                        break;
                    }
                }
            }
            
            std::cout << "嵌套调度测试: " << (is_correct_order ? "通过" : "失败") << std::endl;
            assert(is_correct_order && "嵌套调度测试失败");
        }
        
        // 测试3: 线程安全性
        {
            std::cout << "\n测试3: 线程安全性" << std::endl;
            
            const int task_count = 1000;
            
            // 使用strand
            {
                uasio::strand strand(io);
                int counter = 0;
                
                for (int i = 0; i < task_count; ++i) {
                    strand.post([&counter]() {
                        int old = counter;
                        // 模拟一些耗时操作，增加线程切换的可能性
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                        counter = old + 1;
                    });
                }
                
                // 等待所有任务完成
                std::this_thread::sleep_for(std::chrono::seconds(2));
                
                std::cout << "使用strand的计数器值: " << counter << " (预期: " << task_count << ")" << std::endl;
                assert(counter == task_count && "线程安全性测试失败");
            }
            
            // 不使用strand（仅做参考，不保证正确性）
            {
                int counter = 0;
                std::atomic<int> atomic_counter(0);
                
                for (int i = 0; i < task_count; ++i) {
                    io.post([&counter, &atomic_counter]() {
                        int old = counter;
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                        counter = old + 1;
                        atomic_counter.fetch_add(1);
                    });
                }
                
                // 等待所有任务完成
                while (atomic_counter.load() < task_count) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                
                std::cout << "不使用strand的计数器值: " << counter << " (预期: " << task_count << ")" << std::endl;
                std::cout << "注意：由于数据竞争，不使用strand的值可能小于预期" << std::endl;
            }
        }
        
        // 测试4: running_in_this_thread
        {
            std::cout << "\n测试4: running_in_this_thread" << std::endl;
            
            uasio::strand strand(io);
            bool outside_value = strand.running_in_this_thread();
            bool inside_value = false;
            
            strand.post([&]() {
                inside_value = strand.running_in_this_thread();
            });
            
            // 等待任务完成
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            std::cout << "strand外部running_in_this_thread(): " << (outside_value ? "true" : "false") << std::endl;
            std::cout << "strand内部running_in_this_thread(): " << (inside_value ? "true" : "false") << std::endl;
            
            assert(!outside_value && "strand外部running_in_this_thread()应为false");
            assert(inside_value && "strand内部running_in_this_thread()应为true");
        }
        
        // 停止IO上下文并等待所有线程完成
        std::cout << "\n所有测试完成，清理资源" << std::endl;
        work.reset();
        io.stop();
        
        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
        std::cout << "Strand 测试通过!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}