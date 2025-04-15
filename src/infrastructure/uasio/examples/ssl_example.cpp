// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ssl_example.cpp
 * @brief SSL功能示例程序
 * @details 演示如何使用uasio库进行SSL/TLS安全通信
 */

#include "../src/asio.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <sstream>

using namespace uasio;

// 简单的TCP连接测试
void tcp_client_example() {
    try {
        // 创建IO上下文
        io_context io_ctx;
        std::cout << "创建IO上下文成功" << std::endl;
        
        // 创建套接字
        socket sock(io_ctx);
        
        // 打开套接字
        error_code ec;
        sock.open(address_family::ipv4, socket_type::stream, ec);
        if (ec) {
            std::cerr << "打开套接字失败: " << ec.message() << std::endl;
            return;
        }
        std::cout << "打开套接字成功" << std::endl;
        
        // 连接到服务器
        error_code ip_ec;
        endpoint server_endpoint(ip_address::from_string("1.1.1.1", ip_ec), 80);
        if (ip_ec) {
            std::cerr << "解析IP地址失败: " << ip_ec.message() << std::endl;
            return;
        }
        
        sock.connect(server_endpoint, ec);
        if (ec) {
            std::cerr << "连接服务器失败: " << ec.message() << std::endl;
            return;
        }
        std::cout << "连接到服务器成功: " << server_endpoint.to_string() << std::endl;
        
        // 发送HTTP请求
        std::string request = "GET / HTTP/1.1\r\n"
                             "Host: 1.1.1.1\r\n"
                             "Connection: close\r\n\r\n";
        
        // 尝试发送数据，如果socket是非阻塞的，可能需要多次尝试
        std::size_t total_sent = 0;
        while (total_sent < request.size()) {
            std::size_t sent = sock.send(request.data() + total_sent, request.size() - total_sent, ec);
            if (ec && ec != std::make_error_code(std::errc::resource_unavailable_try_again) &&
                      ec != make_error_code(error::would_block) &&
                      ec != make_error_code(error::operation_in_progress)) {
                std::cerr << "发送请求失败: " << ec.message() << std::endl;
                return;
            }
            
            if (sent > 0) {
                total_sent += sent;
            } else {
                // 短暂暂停后重试
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        std::cout << "发送请求成功" << std::endl;
        
        // 接收响应
        std::string response;
        char buffer[4096];
        bool keep_reading = true;
        
        while (keep_reading) {
            std::size_t bytes = sock.receive(buffer, sizeof(buffer) - 1, ec);
            if (ec) {
                if (ec == make_error_code(error::eof) || 
                    ec == std::make_error_code(std::errc::connection_reset)) {
                    // 正常关闭或重置连接
                    keep_reading = false;
                } else if (ec == std::make_error_code(std::errc::resource_unavailable_try_again) ||
                           ec == make_error_code(error::would_block) ||
                           ec == make_error_code(error::operation_in_progress)) {
                    // 暂时无数据，稍后重试
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                } else {
                    // 其他错误
                    std::cerr << "接收响应失败: " << ec.message() << std::endl;
                    keep_reading = false;
                }
            } else if (bytes == 0) {
                // 连接关闭
                keep_reading = false;
            } else {
                // 成功接收数据
                buffer[bytes] = '\0';
                response.append(buffer, bytes);
            }
        }
        
        std::cout << "收到HTTP响应 (" << response.size() << " 字节)" << std::endl;
        if (!response.empty()) {
            // 显示响应的前几行
            std::istringstream ss(response);
            std::string line;
            int line_count = 0;
            std::cout << "响应内容预览:" << std::endl;
            while (std::getline(ss, line) && line_count < 5) {
                std::cout << line << std::endl;
                line_count++;
            }
        }
        
        // 关闭连接
        sock.close(ec);
        std::cout << "TCP连接测试完成" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "TCP客户端异常: " << e.what() << std::endl;
    }
}

// 简单的阻塞式SSL客户端示例
void blocking_ssl_client_example() {
    try {
        std::cout << "\n=== 开始阻塞式SSL客户端测试 ===" << std::endl;
        
        // 创建IO上下文
        io_context io_ctx;
        std::cout << "创建IO上下文" << std::endl;
        
        // 创建SSL上下文
        ssl::context ctx(ssl::method::tls_client);
        std::cout << "创建SSL上下文" << std::endl;
        
        // 设置验证模式
        error_code ec;
        ctx.set_verify_mode(ssl::verify_mode::peer, ec);
        if (ec) {
            std::cerr << "设置验证模式失败: " << ec.message() << std::endl;
            return;
        }
        
        // 加载CA证书
        ctx.load_verify_file("/etc/ssl/certs/ca-certificates.crt", ssl::file_format::pem, ec);
        if (ec) {
            std::cerr << "加载CA证书失败: " << ec.message() << std::endl;
            return;
        }
        
        std::cout << "SSL初始化完成" << std::endl;
        
        // 创建SSL流（包含底层socket）
        ssl::stream<socket> ssl_stream(io_ctx, ctx);
        std::cout << "创建SSL流" << std::endl;
        
        // 打开底层套接字
        error_code open_ec;
        ssl_stream.next_layer().open(address_family::ipv4, socket_type::stream, open_ec);
        if (open_ec) {
            std::cerr << "打开底层套接字失败: " << open_ec.message() << std::endl;
            return;
        }
        std::cout << "底层套接字已打开" << std::endl;
        
        // 对于阻塞模式，我们依靠同步API的实现，不显式设置阻塞/非阻塞模式
        // 库的Socket默认创建时就是阻塞模式的，除非显式设置为非阻塞
        std::cout << "使用阻塞模式" << std::endl;
        
        // 连接到服务器 - 尝试使用其他HTTPS站点
        std::string host = "www.baidu.com";
        error_code ip_ec;
        
        std::cout << "解析主机: " << host << std::endl;
        // 使用DNS解析主机名，使用正确的query_type值
        resolver resolver(io_ctx);
        resolver_query query(host, "https", query_type::ipv4_only, query_flags::canonical_name);
        resolver_results results = resolver.resolve(query, ip_ec);
        
        if (ip_ec || results.empty()) {
            std::cerr << "无法解析主机 " << host << ": " << ip_ec.message() << std::endl;
            
            // 备用方案：直接使用IP地址
            std::cout << "尝试备用方案：直接使用IP地址 220.181.38.148" << std::endl;
            endpoint server_endpoint(ip_address::from_string("220.181.38.148", ip_ec), 443);
            if (ip_ec) {
                std::cerr << "创建IP端点失败: " << ip_ec.message() << std::endl;
                return;
            }
            
            // 使用底层套接字直接连接
            std::cout << "连接到服务器: " << server_endpoint.to_string() << std::endl;
            ssl_stream.next_layer().connect(server_endpoint, ec);
        } else {
            // 使用解析的第一个地址的endpoint
            const resolver_entry& entry = *results.begin();
            endpoint server_endpoint = entry.get_endpoint();
            std::cout << "连接到服务器: " << server_endpoint.to_string() << std::endl;
            ssl_stream.next_layer().connect(server_endpoint, ec);
        }
        
        if (ec) {
            std::cerr << "连接服务器失败: " << ec.message() << std::endl;
            return;
        }
        std::cout << "TCP连接到服务器成功" << std::endl;
        
        // 执行SSL握手（阻塞模式）
        std::cout << "开始SSL握手..." << std::endl;
        ssl_stream.handshake(ssl::handshake_type::client, ec);
        
        if (ec) {
            std::cerr << "SSL握手失败: " << ec.message() << std::endl;
            return;
        }
        std::cout << "SSL握手成功" << std::endl;
        
        // 获取SSL会话信息
        std::cout << "SSL会话信息:\n" << ssl_stream.get_session_info() << std::endl;
        
        // 发送HTTP请求
        std::string request = "GET / HTTP/1.1\r\n"
                             "Host: " + host + "\r\n"
                             "Connection: close\r\n\r\n";
        
        std::cout << "发送HTTPS请求..." << std::endl;
        std::size_t bytes_written = ssl_stream.write_some(request.data(), request.size(), ec);
        
        if (ec) {
            std::cerr << "发送请求失败: " << ec.message() << std::endl;
            return;
        }
        std::cout << "成功发送了 " << bytes_written << " 字节的请求" << std::endl;
        
        // 接收响应
        std::cout << "等待HTTPS响应..." << std::endl;
        std::string response;
        char buffer[4096];
        
        while (true) {
            std::size_t bytes = ssl_stream.read_some(buffer, sizeof(buffer) - 1, ec);
            
            if (ec == make_error_code(error::eof)) {
                // 连接已关闭，正常情况
                std::cout << "服务器关闭了连接" << std::endl;
                break;
            } else if (ec) {
                std::cerr << "接收响应失败: " << ec.message() << std::endl;
                break;
            }
            
            if (bytes == 0) {
                break;  // 没有更多数据
            }
            
            buffer[bytes] = '\0';
            response.append(buffer, bytes);
        }
        
        std::cout << "收到HTTPS响应 (" << response.size() << " 字节)" << std::endl;
        if (!response.empty()) {
            // 显示响应的前几行
            std::istringstream ss(response);
            std::string line;
            int line_count = 0;
            std::cout << "响应内容预览:" << std::endl;
            while (std::getline(ss, line) && line_count < 5) {
                std::cout << line << std::endl;
                line_count++;
            }
        }
        
        // 关闭SSL连接
        std::cout << "关闭SSL连接..." << std::endl;
        
        // 尝试关闭SSL连接
        ssl_stream.shutdown(ec);
        if (ec && ec != make_error_code(error::eof)) {
            std::cout << "SSL关闭时出现非致命错误: " << ec.message() << " (可以忽略)" << std::endl;
        }
        
        // 关闭底层套接字
        ssl_stream.next_layer().close(ec);
        if (ec) {
            std::cerr << "关闭套接字失败: " << ec.message() << std::endl;
        }
        
        std::cout << "=== 阻塞式SSL客户端测试完成 ===" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "SSL客户端异常: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "示例程序开始" << std::endl;
    
    // 先测试基本的TCP连接
    tcp_client_example();
    
    // 再测试阻塞式SSL连接
    blocking_ssl_client_example();
    
    std::cout << "示例程序结束" << std::endl;
    return 0;
} 