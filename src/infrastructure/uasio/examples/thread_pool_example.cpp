#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include "../src/thread_pool.h"

// 原子计数器，用于跟踪任务完成情况
std::atomic<int> task_counter(0);
std::atomic<int> completed_counter(0);

// 模拟耗时任务
void task_function(int id) {
    std::cout << "任务 " << id << " 开始执行（线程ID: " 
              << std::this_thread::get_id() << "）" << std::endl;
    
    // 模拟处理时间（随机1-3秒）
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 + (id % 3) * 1000));
    
    std::cout << "任务 " << id << " 执行完成" << std::endl;
    ++completed_counter;
}

int main() {
    const int num_tasks = 20;
    
    std::cout << "=== 线程池示例 ===" << std::endl;
    std::cout << "硬件支持的线程数: " << std::thread::hardware_concurrency() << std::endl;
    
    // 创建具有4个工作线程的线程池
    uasio::thread_pool pool(4);
    std::cout << "创建线程池，线程数: 4" << std::endl;
    
    // 启动线程池
    pool.run();
    std::cout << "启动线程池" << std::endl;
    
    // 提交任务到线程池
    std::cout << "提交 " << num_tasks << " 个任务到线程池..." << std::endl;
    task_counter = num_tasks;
    
    for (int i = 0; i < num_tasks; ++i) {
        pool.post([i]() {
            task_function(i);
        });
    }
    
    // 等待所有任务完成
    std::cout << "等待所有任务完成..." << std::endl;
    while (completed_counter < task_counter) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "已完成: " << completed_counter << "/" << task_counter << std::endl;
    }
    
    std::cout << "所有任务已完成！" << std::endl;
    
    // 停止线程池
    std::cout << "停止线程池..." << std::endl;
    pool.stop();
    pool.join();
    
    std::cout << "线程池已停止和清理" << std::endl;
    return 0;
} 