// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../src/asio.h"
#include <iostream>
#include <thread>
#include <chrono>
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

// 打印解析结果
void print_results(const asio::resolver_results& results) {
    std::cout << "解析结果数量: " << results.size() << std::endl;
    int index = 0;
    for (const auto& entry : results) {
        std::cout << "结果 " << ++index << ":" << std::endl;
        std::cout << "  主机名: " << entry.host_name() << std::endl;
        std::cout << "  服务名: " << entry.service_name() << std::endl;
        std::cout << "  端点: " << entry.endpoint().address().to_string() 
                  << ":" << entry.endpoint().port() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "用法: " << argv[0] << " <主机名> [服务名]" << std::endl;
        std::cout << "示例: " << argv[0] << " www.example.com http" << std::endl;
        std::cout << "      " << argv[0] << " www.example.com 80" << std::endl;
        std::cout << "      " << argv[0] << " 8.8.8.8 53" << std::endl;
        return 1;
    }
    
    std::string host = argv[1];
    std::string service = argc > 2 ? argv[2] : "http";
    
    try {
        // 创建 IO 上下文
        asio::io_context io;
        
        // 创建一个工作守卫，防止 IO 上下文过早退出
        auto work = io.make_work_guard();
        
        // 创建解析器
        asio::resolver resolver(io);
        
        std::cout << "开始解析主机: " << host << ", 服务: " << service << std::endl;
        std::cout << "开始时间: " << get_time_string() << std::endl;
        
        // 创建查询
        asio::resolver_query query(host, service);
        
        // 同步解析
        {
            std::cout << "\n=== 同步解析 ===" << std::endl;
            asio::error_code ec;
            auto start_time = std::chrono::steady_clock::now();
            
            // 执行解析
            asio::resolver_results results = resolver.resolve(query, ec);
            
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            std::cout << "解析完成，耗时: " << duration.count() << "ms" << std::endl;
            
            if (ec) {
                std::cout << "解析失败: " << ec.message() << std::endl;
            } else {
                print_results(results);
            }
        }
        
        // 异步解析
        {
            std::cout << "\n=== 异步解析 ===" << std::endl;
            auto start_time = std::chrono::steady_clock::now();
            
            // 创建带有不同标志的查询
            asio::resolver_query async_query(
                host, 
                service, 
                asio::query_type::all, 
                asio::query_flags::canonical_name
            );
            
            // 执行异步解析
            resolver.async_resolve(
                async_query,
                [start_time](const asio::error_code& ec, asio::resolver_results results) {
                    auto end_time = std::chrono::steady_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                    
                    std::cout << "异步解析完成，耗时: " << duration.count() << "ms" << std::endl;
                    
                    if (ec) {
                        std::cout << "解析失败: " << ec.message() << std::endl;
                    } else {
                        print_results(results);
                    }
                }
            );
        }
        
        // 在后台线程中运行 IO 上下文
        std::thread io_thread([&io]() {
            std::cout << "IO线程开始运行，线程ID: "
                      << std::this_thread::get_id() << std::endl;
            io.run();
            std::cout << "IO线程结束运行" << std::endl;
        });
        
        // 主线程等待一段时间
        std::cout << "\n主线程等待5秒，线程ID: "
                  << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        // 取消解析器上的所有操作
        std::size_t cancelled = resolver.cancel();
        std::cout << "取消了 " << cancelled << " 个未完成的解析操作" << std::endl;
        
        // 重置工作守卫，允许 IO 上下文退出
        std::cout << "重置工作守卫，允许IO上下文退出" << std::endl;
        work.reset();
        
        // 等待 IO 线程完成
        std::cout << "等待IO线程完成..." << std::endl;
        io_thread.join();
        
        std::cout << "程序正常退出，时间: " << get_time_string() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 