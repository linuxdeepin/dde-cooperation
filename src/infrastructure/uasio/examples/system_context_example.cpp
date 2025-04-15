#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include "../src/system_context.h"

// 原子计数器，用于跟踪任务完成情况
std::atomic<int> global_counter(0);

// 简单任务函数
void task_function(int id) {
    std::cout << "系统上下文任务 " << id << " 开始执行" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "系统上下文任务 " << id << " 执行完成" << std::endl;
    ++global_counter;
}

// 在另一个线程中运行系统上下文
void run_system_context() {
    std::cout << "启动系统上下文线程..." << std::endl;
    uasio::system_context::get().context().run();
    std::cout << "系统上下文线程退出" << std::endl;
}

int main() {
    std::cout << "=== 系统上下文示例 ===" << std::endl;
    
    // 获取系统上下文
    uasio::system_context& system_ctx = uasio::system_context::get();
    uasio::io_context& ctx = system_ctx.context();
    
    // 在单独的线程中运行系统上下文
    std::thread ctx_thread(run_system_context);
    
    // 给系统上下文一些时间启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 在系统上下文中调度任务
    std::cout << "提交任务到系统上下文..." << std::endl;
    
    for (int i = 0; i < 5; ++i) {
        ctx.post([i]() {
            task_function(i);
        });
    }
    
    // 等待所有任务完成
    std::cout << "等待所有任务完成..." << std::endl;
    while (global_counter < 5) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "已完成: " << global_counter << "/5" << std::endl;
    }
    
    std::cout << "所有任务已完成" << std::endl;
    
    // 停止系统上下文
    std::cout << "停止系统上下文..." << std::endl;
    system_ctx.stop();
    
    // 等待系统上下文线程退出
    if (ctx_thread.joinable()) {
        ctx_thread.join();
    }
    
    std::cout << "系统上下文示例完成" << std::endl;
    
    // 重启系统上下文
    std::cout << "\n重启系统上下文..." << std::endl;
    system_ctx.restart();
    
    // 在重启的上下文中提交任务
    global_counter = 0;
    ctx_thread = std::thread(run_system_context);
    
    // 给系统上下文一些时间启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "在重启的系统上下文中提交任务..." << std::endl;
    for (int i = 5; i < 8; ++i) {
        ctx.post([i]() {
            task_function(i);
        });
    }
    
    // 等待所有任务完成
    std::cout << "等待所有任务完成..." << std::endl;
    while (global_counter < 3) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "已完成: " << global_counter << "/3" << std::endl;
    }
    
    // 最终停止系统上下文
    system_ctx.stop();
    if (ctx_thread.joinable()) {
        ctx_thread.join();
    }
    
    std::cout << "示例结束" << std::endl;
    return 0;
} 