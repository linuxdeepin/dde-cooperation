// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file signal_set_example.cpp
 * @brief 演示如何使用signal_set处理系统信号
 */

#include "../src/asio.h"
#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>

void print_time() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::cout << "[" << std::ctime(&now_time_t) << "] ";
}

int main() {
    // 创建IO上下文
    asio::io_context io_context;
    
    // 创建信号集
    asio::signal_set signals(io_context);
    
    // 添加信号
    asio::error_code ec;
    
    std::cout << "添加SIGINT信号... ";
    if (signals.add(SIGINT, ec)) {
        std::cout << "成功\n";
    } else {
        std::cout << "失败: " << ec.message() << "\n";
        return 1;
    }
    
    std::cout << "添加SIGTERM信号... ";
    if (signals.add(SIGTERM, ec)) {
        std::cout << "成功\n";
    } else {
        std::cout << "失败: " << ec.message() << "\n";
        return 1;
    }
    
    // 设置信号处理函数
    signals.async_wait([&](const asio::error_code& error, int signal_number) {
        if (!error) {
            print_time();
            std::cout << "收到信号: ";
            
            switch (signal_number) {
                case SIGINT:
                    std::cout << "SIGINT (Ctrl+C)\n";
                    break;
                case SIGTERM:
                    std::cout << "SIGTERM\n";
                    break;
                default:
                    std::cout << signal_number << "\n";
                    break;
            }
            
            // 再次等待信号
            signals.async_wait([&](const asio::error_code& error, int signal_number) {
                if (!error) {
                    print_time();
                    std::cout << "再次收到信号: " << signal_number << "\n";
                    std::cout << "停止IO上下文\n";
                    io_context.stop();
                } else {
                    std::cout << "信号等待错误: " << error.message() << "\n";
                }
            });
            
            std::cout << "已重新设置信号处理函数，再次发送信号将停止程序\n";
        } else {
            std::cout << "信号等待错误: " << error.message() << "\n";
        }
    });
    
    // 创建工作任务以保持IO上下文运行
    asio::io_context::work work(io_context);
    
    // 打印说明信息
    std::cout << "\n信号处理示例程序\n";
    std::cout << "----------------------------\n";
    std::cout << "- 按 Ctrl+C 触发 SIGINT\n";
    std::cout << "- 在另一个终端执行 'kill -TERM " << getpid() << "' 触发 SIGTERM\n";
    std::cout << "----------------------------\n";
    std::cout << "程序将在收到两次信号后退出\n\n";
    
    print_time();
    std::cout << "开始等待信号...\n";
    
    // 定时打印心跳消息
    std::thread heartbeat_thread([&]() {
        int count = 0;
        while (!io_context.stopped()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            if (!io_context.stopped()) {
                print_time();
                std::cout << "心跳 #" << ++count << " - 程序仍在运行\n";
            }
        }
    });
    
    // 运行IO上下文
    io_context.run();
    
    print_time();
    std::cout << "程序正常退出\n";
    
    // 等待心跳线程结束
    heartbeat_thread.join();
    
    return 0;
} 