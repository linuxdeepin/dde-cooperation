// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../src/asio.h"
#include "../src/io_context.h"
#include "../src/timer.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <functional>
#include <iomanip>

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

int main() {
    try {
        // 创建 IO 上下文
        asio::io_context io;
        
        // 打印开始消息
        std::cout << "定时器示例程序启动时间: " << get_time_string() << std::endl;
        
        // 创建一个工作守卫，防止 IO 上下文过早退出
        auto work = io.make_work_guard();
        
        // 创建一个定时器，设置为 2 秒后到期
        asio::deadline_timer timer1(io, std::chrono::seconds(2));
        std::cout << "定时器1创建时间: " << get_time_string() << "，定时2秒" << std::endl;
        
        // 异步等待定时器1到期
        timer1.async_wait([](const asio::error_code& ec) {
            if (!ec) {
                std::cout << "定时器1到期时间: " << get_time_string() << std::endl;
            } else {
                std::cout << "定时器1错误: " << ec.message() << std::endl;
            }
        });
        
        // 创建一个定时器，设置为 3 秒后到期
        asio::deadline_timer timer2(io, std::chrono::seconds(3));
        std::cout << "定时器2创建时间: " << get_time_string() << "，定时3秒" << std::endl;
        
        // 异步等待定时器2到期
        timer2.async_wait([](const asio::error_code& ec) {
            if (!ec) {
                std::cout << "定时器2到期时间: " << get_time_string() << std::endl;
            } else {
                std::cout << "定时器2错误: " << ec.message() << std::endl;
            }
        });
        
        // 创建一个定时器，设置为 1 秒后到期
        asio::deadline_timer timer3(io, std::chrono::seconds(1));
        std::cout << "定时器3创建时间: " << get_time_string() << "，定时1秒" << std::endl;
        
        // 异步等待定时器3到期
        timer3.async_wait([&io](const asio::error_code& ec) {
            if (!ec) {
                std::cout << "定时器3到期时间: " << get_time_string() << std::endl;
                
                // 创建一个新的定时器，设置为 2.5 秒后到期
                asio::deadline_timer timer4(io, std::chrono::milliseconds(2500));
                std::cout << "定时器4创建时间: " << get_time_string() << "，定时2.5秒" << std::endl;
                
                // 异步等待定时器4到期
                timer4.async_wait([](const asio::error_code& ec) {
                    if (!ec) {
                        std::cout << "定时器4到期时间: " << get_time_string() << std::endl;
                    } else {
                        std::cout << "定时器4错误: " << ec.message() << std::endl;
                    }
                });
            } else {
                std::cout << "定时器3错误: " << ec.message() << std::endl;
            }
        });
        
        // 在后台线程中运行 IO 上下文
        std::thread io_thread([&io]() {
            std::cout << "IO线程开始运行，线程ID: "
                      << std::this_thread::get_id() << std::endl;
            io.run();
            std::cout << "IO线程结束运行" << std::endl;
        });
        
        // 主线程休眠 5 秒
        std::cout << "主线程休眠5秒，线程ID: "
                  << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        // 重置工作守卫，允许 IO 上下文退出
        std::cout << "重置工作守卫，允许IO上下文退出" << std::endl;
        work.reset();
        
        // 显式停止 IO 上下文
        std::cout << "显式停止IO上下文" << std::endl;
        io.stop();
        
        // 等待IO线程完成
        std::cout << "等待IO线程完成..." << std::endl;
        if (io_thread.joinable()) {
            io_thread.join();
        }
        
        std::cout << "程序正常退出，时间: " << get_time_string() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 