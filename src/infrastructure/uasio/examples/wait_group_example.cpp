#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <random>
#include "../src/wait_group.h"
#include "../src/thread_pool.h"

// 模拟工作任务
void work_task(int id, int duration_ms) {
    std::cout << "任务 " << id << " 开始执行（延迟: " << duration_ms << " ms）" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    std::cout << "任务 " << id << " 完成" << std::endl;
}

int main() {
    std::cout << "=== 等待组（WaitGroup）示例 ===" << std::endl;
    
    // 创建线程池
    uasio::thread_pool pool(4);
    pool.run();
    
    // 创建随机数生成器，用于生成随机延迟
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(500, 3000); // 500ms - 3000ms随机延迟
    
    std::cout << "\n== 示例1: 基本用法 ==" << std::endl;
    {
        uasio::wait_group wg;
        
        // 添加5个任务
        const int num_tasks = 5;
        wg.add(num_tasks);
        
        std::cout << "启动 " << num_tasks << " 个任务..." << std::endl;
        
        for (int i = 0; i < num_tasks; ++i) {
            // 创建随机延迟
            int delay = dist(gen);
            
            // 在线程池中执行任务
            pool.post([i, delay, &wg]() {
                work_task(i, delay);
                // 完成一个任务
                wg.done();
            });
        }
        
        std::cout << "等待所有任务完成..." << std::endl;
        
        // 等待所有任务完成
        wg.wait();
        
        std::cout << "所有任务已完成！" << std::endl;
    }
    
    std::cout << "\n== 示例2: 使用包装器 ==" << std::endl;
    {
        uasio::wait_group wg;
        
        std::cout << "使用包装器启动3个任务..." << std::endl;
        
        for (int i = 0; i < 3; ++i) {
            int delay = dist(gen);
            
            // 使用wait_group的wrap函数创建包装任务
            auto wrapped_task = wg.wrap([i, delay](){ 
                work_task(i, delay);
            });
            
            // 在线程池中执行包装任务
            pool.post(wrapped_task);
        }
        
        std::cout << "等待所有包装任务完成..." << std::endl;
        
        // 等待所有任务完成，带5秒超时
        bool completed = wg.wait_for(std::chrono::seconds(5));
        
        if (completed) {
            std::cout << "所有包装任务在超时前完成！" << std::endl;
        } else {
            std::cout << "等待超时，剩余 " << wg.count() << " 个任务未完成" << std::endl;
        }
    }
    
    std::cout << "\n== 示例3: 处理异常 ==" << std::endl;
    {
        uasio::wait_group wg;
        
        std::cout << "启动3个任务，其中一个会抛出异常..." << std::endl;
        
        for (int i = 0; i < 3; ++i) {
            int delay = dist(gen);
            
            // 使用wait_group的wrap函数创建包装任务
            auto wrapped_task = wg.wrap([i, delay]() {
                if (i == 1) {
                    std::cout << "任务 " << i << " 抛出异常" << std::endl;
                    throw std::runtime_error("模拟任务异常");
                }
                work_task(i, delay);
            });
            
            // 在线程池中执行包装任务
            pool.post([wrapped_task]() {
                try {
                    wrapped_task();
                } catch (const std::exception& e) {
                    std::cerr << "捕获到异常: " << e.what() << std::endl;
                }
            });
        }
        
        std::cout << "等待所有任务完成（即使有异常）..." << std::endl;
        
        // 等待所有任务完成
        wg.wait();
        
        std::cout << "所有任务已处理完毕！" << std::endl;
    }
    
    // 停止线程池
    pool.stop();
    pool.join();
    
    return 0;
} 