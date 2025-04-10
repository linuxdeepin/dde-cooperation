// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_steady_timer.cpp
 * @brief 测试steady_timer功能
 */

#include "../src/asio.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>
#include <atomic>
#include <vector>
#include <functional>

// 测试函数前置声明
bool test_timer_basic();
bool test_timer_cancel();
bool test_timer_multiple();

int main() {
    std::cout << "开始测试 steady_timer 模块...\n\n";
    
    bool all_passed = true;
    
    std::cout << "测试 1: 基本定时器功能... ";
    bool test1_result = test_timer_basic();
    std::cout << (test1_result ? "通过" : "失败") << "\n";
    all_passed &= test1_result;
    
    std::cout << "测试 2: 定时器取消... ";
    bool test2_result = test_timer_cancel();
    std::cout << (test2_result ? "通过" : "失败") << "\n";
    all_passed &= test2_result;
    
    std::cout << "测试 3: 多个定时器... ";
    bool test3_result = test_timer_multiple();
    std::cout << (test3_result ? "通过" : "失败") << "\n";
    all_passed &= test3_result;
    
    std::cout << "\n测试结果汇总: " << (all_passed ? "所有测试通过" : "部分测试失败") << "\n";
    
    return all_passed ? 0 : 1;
}

// 测试基本定时器功能
bool test_timer_basic() {
    try {
        uasio::io_context io;
        uasio::steady_timer timer(io);
        uasio::error_code ec;
        
        // 设置定时器
        timer.expires_from_now(std::chrono::milliseconds(100));
        
        std::atomic<bool> timer_fired(false);
        
        // 异步等待
        timer.async_wait([&](const uasio::error_code& error) {
            if (!error) {
                timer_fired = true;
            }
        });
        
        // 运行IO上下文
        io.run();
        
        // 检查定时器是否触发
        if (!timer_fired) {
            std::cerr << "定时器未触发\n";
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "测试异常: " << e.what() << "\n";
        return false;
    }
}

// 测试定时器取消
bool test_timer_cancel() {
    try {
        uasio::io_context io;
        uasio::steady_timer timer(io);
        uasio::error_code ec;
        
        // 设置定时器
        timer.expires_from_now(std::chrono::milliseconds(500));
        
        std::atomic<bool> timer_cancelled(false);
        
        // 异步等待
        timer.async_wait([&](const uasio::error_code& error) {
            if (error.value() == uasio::error::operation_aborted) {
                timer_cancelled = true;
            }
        });
        
        // 在另一个线程中取消定时器
        std::thread cancel_thread([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            timer.cancel();
        });
        
        // 运行IO上下文
        io.run();
        
        // 等待取消线程完成
        cancel_thread.join();
        
        // 检查定时器是否被取消
        if (!timer_cancelled) {
            std::cerr << "定时器未被正确取消\n";
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "测试异常: " << e.what() << "\n";
        return false;
    }
}

// 测试多个定时器
bool test_timer_multiple() {
    try {
        uasio::io_context io;
        std::vector<uasio::steady_timer> timers;
        std::atomic<int> fired_count(0);
        
        // 创建5个定时器
        for (int i = 0; i < 5; ++i) {
            timers.emplace_back(io);
            auto& timer = timers.back();
            
            // 设置不同的超时时间
            timer.expires_from_now(std::chrono::milliseconds(100 * (i + 1)));
            
            // 异步等待
            timer.async_wait([&](const uasio::error_code& error) {
                if (!error) {
                    ++fired_count;
                }
            });
        }
        
        // 运行IO上下文
        io.run();
        
        // 检查所有定时器是否都触发
        if (fired_count != 5) {
            std::cerr << "定时器触发数量不正确，期望: 5，实际: " << fired_count << "\n";
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "测试异常: " << e.what() << "\n";
        return false;
    }
}