// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../src/asio.h"
#include "../src/io_context.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <functional>

int main() {
    try {
        // 创建 IO 上下文
        asio::io_context io;
        
        std::cout << "创建 IO 上下文完成" << std::endl;
        
        // 创建一个工作守卫，防止 IO 上下文过早退出
        auto work = io.make_work_guard();
        
        // 发布一个任务到 IO 上下文
        std::function<void()> task = []() {
            std::cout << "定时任务已执行！线程ID: " 
                      << std::this_thread::get_id() << std::endl;
        };
        
        io.post(task);
        
        // 在后台线程中运行 IO 上下文
        std::thread io_thread([&io]() {
            std::cout << "IO线程开始运行，线程ID: "
                      << std::this_thread::get_id() << std::endl;
            io.run();
            std::cout << "IO线程结束运行" << std::endl;
        });
        
        // 主线程休眠一小段时间
        std::cout << "主线程休眠2秒，线程ID: "
                  << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // 发布另一个任务
        io.post([]() {
            std::cout << "第二个任务已执行！线程ID: "
                      << std::this_thread::get_id() << std::endl;
        });
        
        // 再次休眠
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // 重置工作守卫，允许 IO 上下文退出
        std::cout << "重置工作守卫，允许IO上下文退出" << std::endl;
        work.reset();
        
        // 为了确保最后一个任务被处理，我们发送一个额外的停止任务
        io.post([]() {
            std::cout << "最后一个任务已执行，准备退出" << std::endl;
        });
        
        std::cout << "等待IO线程完成..." << std::endl;
        
        // 最后显式停止io_context
        io.stop();
        
        // 等待IO线程完成
        if (io_thread.joinable()) {
            io_thread.join();
        }
        
        std::cout << "程序正常退出" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 