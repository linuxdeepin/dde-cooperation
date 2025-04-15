// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file steady_timer_example.cpp
 * @brief 演示steady_timer的用法
 */

#include "../src/asio.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <atomic>

// 获取当前时间的字符串表示
std::string get_time_string() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_tm = std::localtime(&now_time_t);
    
    std::ostringstream oss;
    oss << std::put_time(now_tm, "%H:%M:%S");
    
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - std::chrono::system_clock::from_time_t(now_time_t)).count();
    
    oss << "." << std::setfill('0') << std::setw(3) << ms;
    return oss.str();
}

class timer_chain {
public:
    timer_chain(asio::io_context& io_context, int count, int interval_ms)
        : io_context_(io_context),
          timer_(io_context),
          count_(count),
          interval_ms_(interval_ms),
          current_(0),
          stopped_(false) {
    }
    
    // 启动定时器链
    void start() {
        std::cout << "[" << get_time_string() << "] 启动定时器链，计划执行 " 
                  << count_ << " 次，间隔 " << interval_ms_ << " 毫秒\n";
        
        current_ = 0;
        stopped_ = false;
        
        // 设置第一个定时器
        schedule_timer();
    }
    
    // 停止定时器链
    void stop() {
        std::cout << "[" << get_time_string() << "] 停止定时器链\n";
        stopped_ = true;
        timer_.cancel();
    }
    
private:
    // 调度下一个定时器
    void schedule_timer() {
        if (stopped_ || current_ >= count_) {
            return;
        }
        
        // 设置定时器
        timer_.expires_from_now(std::chrono::milliseconds(interval_ms_));
        
        // 设置异步等待回调
        timer_.async_wait([this](const asio::error_code& ec) {
            if (ec) {
                if (ec != asio::error::operation_aborted) {
                    std::cout << "[" << get_time_string() << "] 定时器错误: " 
                              << ec.message() << "\n";
                }
                return;
            }
            
            // 定时器触发
            current_++;
            std::cout << "[" << get_time_string() << "] 定时器触发 #" 
                      << current_ << " / " << count_ << "\n";
            
            // 调度下一个定时器
            schedule_timer();
        });
    }
    
    asio::io_context& io_context_;
    asio::steady_timer timer_;
    int count_;
    int interval_ms_;
    int current_;
    std::atomic<bool> stopped_;
};

int main() {
    try {
        asio::io_context io_context;
        
        // 创建一个工作守卫，防止IO上下文过早退出
        auto work = io_context.make_work_guard();
        
        // 创建定时器链
        timer_chain chain1(io_context, 5, 500);  // 5次，每次间隔500毫秒
        timer_chain chain2(io_context, 3, 800);  // 3次，每次间隔800毫秒
        
        // 启动IO上下文线程
        std::thread io_thread([&io_context]() {
            std::cout << "[" << get_time_string() << "] IO线程启动\n";
            io_context.run();
            std::cout << "[" << get_time_string() << "] IO线程结束\n";
        });
        
        // 启动定时器链
        chain1.start();
        
        // 等待1秒后启动第二个定时器链
        std::this_thread::sleep_for(std::chrono::seconds(1));
        chain2.start();
        
        // 等待5秒后停止第一个定时器链
        std::this_thread::sleep_for(std::chrono::seconds(5));
        chain1.stop();
        
        // 等待所有定时器完成
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        // 清理工作
        std::cout << "[" << get_time_string() << "] 清理资源\n";
        work.reset();
        
        // 等待IO线程结束
        if (io_thread.joinable()) {
            io_thread.join();
        }
        
        std::cout << "[" << get_time_string() << "] 程序正常退出\n";
    }
    catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
} 