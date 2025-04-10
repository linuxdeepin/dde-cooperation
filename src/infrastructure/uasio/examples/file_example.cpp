// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "asio.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <fstream>

// 获取格式化的当前时间字符串
std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time_t));
    
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::string result = buffer;
    result += "." + std::to_string(ms.count());
    
    return result;
}

// 安全打印到控制台（带时间戳）
template<typename... Args>
void log(Args&&... args) {
    std::cout << "[" << get_timestamp() << "] ";
    (std::cout << ... << args);
    std::cout << std::endl;
}

// 生成测试文件
void generate_test_file(const std::string& filename, size_t size) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error creating test file: " << filename << std::endl;
        return;
    }
    
    std::string data;
    data.reserve(size);
    
    // 填充一些随机数据
    for (size_t i = 0; i < size; ++i) {
        data.push_back(static_cast<char>('A' + (i % 26)));
    }
    
    file.write(data.data(), data.size());
    file.close();
    
    log("生成了测试文件: ", filename, " (", size, " 字节)");
}

// 异步读取文件示例
void async_read_example(asio::io_context& io_context) {
    std::string filename = "test_read.txt";
    constexpr size_t file_size = 1024;
    
    // 生成测试文件
    generate_test_file(filename, file_size);
    
    log("开始异步读取文件示例");
    
    asio::error_code ec;
    asio::file file(io_context, filename, asio::file_mode::read_only, ec);
    
    if (ec) {
        log("打开文件失败: ", ec.message());
        return;
    }
    
    log("文件已打开: ", filename);
    
    // 获取文件大小
    size_t size = file.size(ec);
    if (ec) {
        log("获取文件大小失败: ", ec.message());
        return;
    }
    
    log("文件大小: ", size, " 字节");
    
    // 分配缓冲区
    std::vector<char> buffer(size);
    
    // 异步读取整个文件
    file.async_read(buffer.data(), buffer.size(), 
        [&buffer, &file](const asio::error_code& ec, std::size_t bytes_read) {
            if (ec) {
                log("读取文件失败: ", ec.message());
                return;
            }
            
            log("异步读取完成, 读取了 ", bytes_read, " 字节");
            
            // 显示文件内容的前50个字符
            std::string content(buffer.data(), std::min(bytes_read, static_cast<std::size_t>(50)));
            log("文件内容 (前50个字符): ", content, bytes_read > 50 ? "..." : "");
            
            // 关闭文件
            asio::error_code close_ec;
            file.close(close_ec);
            if (close_ec) {
                log("关闭文件失败: ", close_ec.message());
            } else {
                log("文件已关闭");
            }
        });
    
    log("异步读取请求已提交");
}

// 异步写入文件示例
void async_write_example(asio::io_context& io_context) {
    std::string filename = "test_write.txt";
    
    log("开始异步写入文件示例");
    
    asio::error_code ec;
    asio::file file(io_context, filename, asio::file_mode::truncate, ec);
    
    if (ec) {
        log("打开/创建文件失败: ", ec.message());
        return;
    }
    
    log("文件已打开: ", filename);
    
    // 创建要写入的数据
    std::string data = "这是一个异步写入测试。这些数据将被写入到文件中。\n";
    data += "这是第二行文本，展示了多行写入功能。\n";
    data += "第三行包含一些随机字符：ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()";
    
    // 异步写入数据
    file.async_write(data.data(), data.size(), 
        [&file, &data](const asio::error_code& ec, std::size_t bytes_written) {
            if (ec) {
                log("写入文件失败: ", ec.message());
                return;
            }
            
            log("异步写入完成, 写入了 ", bytes_written, " 字节");
            
            // 异步刷新数据到磁盘
            file.async_flush([&file](const asio::error_code& ec) {
                if (ec) {
                    log("刷新文件失败: ", ec.message());
                    return;
                }
                
                log("文件已刷新到磁盘");
                
                // 关闭文件
                asio::error_code close_ec;
                file.close(close_ec);
                if (close_ec) {
                    log("关闭文件失败: ", close_ec.message());
                } else {
                    log("文件已关闭");
                }
            });
        });
    
    log("异步写入请求已提交");
}

// 文件定位和随机读写示例
void seek_and_append_example(asio::io_context& io_context) {
    std::string filename = "test_seek.txt";
    
    log("开始文件定位和随机读写示例");
    
    // 创建初始文件
    {
        std::ofstream file(filename);
        file << "这是文件的初始内容。\n";
        file.close();
    }
    
    asio::error_code ec;
    asio::file file(io_context, filename, asio::file_mode::read_write, ec);
    
    if (ec) {
        log("打开文件失败: ", ec.message());
        return;
    }
    
    log("文件已打开: ", filename);
    
    // 读取初始内容
    char initial_buffer[100];
    std::size_t bytes_read = file.read(initial_buffer, sizeof(initial_buffer), ec);
    if (ec) {
        log("读取初始内容失败: ", ec.message());
        return;
    }
    
    std::string initial_content(initial_buffer, bytes_read);
    log("初始内容: ", initial_content);
    
    // 定位到文件末尾
    std::size_t end_pos = file.seek(0, asio::seek_basis::end, ec);
    if (ec) {
        log("定位到文件末尾失败: ", ec.message());
        return;
    }
    
    log("文件指针已定位到末尾, 位置: ", end_pos);
    
    // 添加内容
    std::string append_data = "这是添加的内容，写入到文件末尾。\n";
    std::size_t bytes_written = file.write(append_data.data(), append_data.size(), ec);
    if (ec) {
        log("写入追加数据失败: ", ec.message());
        return;
    }
    
    log("已添加 ", bytes_written, " 字节到文件末尾");
    
    // 定位回文件开头
    file.seek(0, asio::seek_basis::start, ec);
    if (ec) {
        log("定位到文件开头失败: ", ec.message());
        return;
    }
    
    // 读取整个文件
    std::vector<char> buffer(end_pos + append_data.size());
    bytes_read = file.read(buffer.data(), buffer.size(), ec);
    if (ec) {
        log("读取整个文件失败: ", ec.message());
        return;
    }
    
    std::string full_content(buffer.data(), bytes_read);
    log("完整内容: ", full_content);
    
    // 关闭文件
    file.close(ec);
    if (ec) {
        log("关闭文件失败: ", ec.message());
    } else {
        log("文件已关闭");
    }
}

int main() {
    try {
        asio::io_context io_context;
        
        // 运行异步读取示例
        async_read_example(io_context);
        
        // 运行异步写入示例
        async_write_example(io_context);
        
        // 运行文件定位和随机读写示例
        seek_and_append_example(io_context);
        
        // 启动io_context
        std::thread worker([&io_context]() {
            io_context.run();
        });
        
        // 等待异步操作完成
        worker.join();
        
        log("所有示例已完成");
    }
    catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 