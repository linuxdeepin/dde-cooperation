// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_datagram_socket.cpp
 * @brief 测试datagram_socket功能
 */

#include "../src/asio.h"
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include <cassert>

// 测试函数前置声明
bool test_basic_operations();
bool test_send_receive();
bool test_broadcast();
bool test_buffer_send_receive();

// 辅助函数：在单独的线程中运行IO上下文并处理UDP服务器
void run_udp_server(uasio::io_context& io_context, int port, 
                    std::vector<std::string>& received_messages,
                    std::atomic<bool>& server_ready,
                    std::atomic<bool>& stop_server);

int main() {
    std::cout << "开始测试 datagram_socket 模块...\n\n";
    
    bool all_passed = true;
    
    std::cout << "测试 1: 基本操作... ";
    bool test1_result = test_basic_operations();
    std::cout << (test1_result ? "通过" : "失败") << "\n";
    all_passed &= test1_result;
    
    std::cout << "测试 2: 发送和接收... ";
    bool test2_result = test_send_receive();
    std::cout << (test2_result ? "通过" : "失败") << "\n";
    all_passed &= test2_result;
    
    std::cout << "测试 3: 广播... ";
    bool test3_result = test_broadcast();
    std::cout << (test3_result ? "通过" : "失败") << "\n";
    all_passed &= test3_result;
    
    std::cout << "测试 4: 缓冲区发送和接收... ";
    bool test4_result = test_buffer_send_receive();
    std::cout << (test4_result ? "通过" : "失败") << "\n";
    all_passed &= test4_result;
    
    std::cout << "\n测试结果汇总: " << (all_passed ? "所有测试通过" : "部分测试失败") << "\n";
    
    return all_passed ? 0 : 1;
}

// 测试基本操作
bool test_basic_operations() {
    try {
        uasio::io_context io;
        uasio::datagram_socket socket(io);
        uasio::error_code ec;
        
        // 测试打开套接字
        socket.open(uasio::address_family::ipv4, ec);
        if (ec) {
            std::cerr << "打开套接字失败: " << ec.message() << "\n";
            return false;
        }
        
        // 测试绑定套接字到地址
        uasio::endpoint endpoint(uasio::ip_address::any(), 0);
        socket.bind(endpoint, ec);
        if (ec) {
            std::cerr << "绑定套接字失败: " << ec.message() << "\n";
            return false;
        }
        
        // 测试获取本地端点
        uasio::endpoint local_endpoint = socket.local_endpoint(ec);
        if (ec) {
            std::cerr << "获取本地端点失败: " << ec.message() << "\n";
            return false;
        }
        
        // 检查绑定的端口不为0（系统分配了端口）
        if (local_endpoint.port() == 0) {
            std::cerr << "本地端点端口为0，应为系统分配的端口\n";
            return false;
        }
        
        // 测试设置选项
        socket.set_receive_buffer_size(4096, ec);
        if (ec) {
            std::cerr << "设置接收缓冲区大小失败: " << ec.message() << "\n";
            return false;
        }
        
        // 测试关闭套接字
        socket.close(ec);
        if (ec) {
            std::cerr << "关闭套接字失败: " << ec.message() << "\n";
            return false;
        }
        
        // 验证套接字已关闭
        if (socket.is_open()) {
            std::cerr << "套接字应该已关闭，但is_open()返回true\n";
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "测试异常: " << e.what() << "\n";
        return false;
    }
}

// 测试发送和接收
bool test_send_receive() {
    try {
        uasio::io_context io;
        std::vector<std::string> received_messages;
        std::atomic<bool> server_ready(false);
        std::atomic<bool> stop_server(false);
        
        // 启动UDP服务器
        int server_port = 12345;
        std::thread server_thread(run_udp_server, std::ref(io), server_port, 
                                 std::ref(received_messages), std::ref(server_ready), 
                                 std::ref(stop_server));
        
        // 等待服务器启动
        while (!server_ready) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // 创建客户端套接字
        uasio::datagram_socket client(io);
        uasio::error_code ec;
        
        // 打开套接字
        client.open(uasio::address_family::ipv4, ec);
        if (ec) {
            std::cerr << "打开客户端套接字失败: " << ec.message() << "\n";
            stop_server = true;
            server_thread.join();
            return false;
        }
        
        // 创建服务器端点
        uasio::endpoint server_endpoint(uasio::ip_address::from_string("127.0.0.1", ec), server_port);
        if (ec) {
            std::cerr << "创建服务器端点失败: " << ec.message() << "\n";
            stop_server = true;
            server_thread.join();
            return false;
        }
        
        // 发送消息
        std::string message = "Hello, UDP Server!";
        std::size_t bytes_sent = client.send_to(message.c_str(), message.size(), server_endpoint, ec);
        if (ec || bytes_sent != message.size()) {
            std::cerr << "发送消息失败: " << ec.message() << "\n";
            stop_server = true;
            server_thread.join();
            return false;
        }
        
        // 等待服务器接收消息
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // 检查服务器是否收到消息
        if (received_messages.empty() || received_messages[0] != message) {
            std::cerr << "服务器未收到正确的消息\n";
            stop_server = true;
            server_thread.join();
            return false;
        }
        
        // 停止服务器
        stop_server = true;
        server_thread.join();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "测试异常: " << e.what() << "\n";
        return false;
    }
}

// 测试广播
bool test_broadcast() {
    try {
        uasio::io_context io;
        uasio::datagram_socket socket(io);
        uasio::error_code ec;
        
        // 打开套接字
        socket.open(uasio::address_family::ipv4, ec);
        if (ec) {
            std::cerr << "打开套接字失败: " << ec.message() << "\n";
            return false;
        }
        
        // 设置广播选项
        socket.set_broadcast(true, ec);
        if (ec) {
            std::cerr << "设置广播选项失败: " << ec.message() << "\n";
            return false;
        }
        
        // 绑定套接字到地址
        uasio::endpoint endpoint(uasio::ip_address::any(), 0);
        socket.bind(endpoint, ec);
        if (ec) {
            std::cerr << "绑定套接字失败: " << ec.message() << "\n";
            return false;
        }
        
        // 创建广播端点
        uasio::endpoint broadcast_endpoint(uasio::ip_address::from_string("255.255.255.255", ec), 12345);
        if (ec) {
            std::cerr << "创建广播端点失败: " << ec.message() << "\n";
            return false;
        }
        
        // 发送广播消息（仅测试是否可以发送，不检查接收）
        std::string message = "Broadcast test message";
        std::size_t bytes_sent = socket.send_to(message.c_str(), message.size(), broadcast_endpoint, ec);
        
        // 在某些系统上可能会失败，所以这里只检查但不返回false
        if (ec) {
            std::cerr << "发送广播消息失败: " << ec.message() << "\n";
            std::cerr << "这可能是由于网络配置或防火墙设置导致的，不认为是测试失败\n";
        } else if (bytes_sent != message.size()) {
            std::cerr << "发送的字节数不匹配，期望: " << message.size() << "，实际: " << bytes_sent << "\n";
            return false;
        }
        
        // 关闭套接字
        socket.close(ec);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "测试异常: " << e.what() << "\n";
        return false;
    }
}

// 测试缓冲区发送和接收
bool test_buffer_send_receive() {
    try {
        uasio::io_context io;
        std::vector<std::string> received_messages;
        std::atomic<bool> server_ready(false);
        std::atomic<bool> stop_server(false);
        
        // 启动UDP服务器
        int server_port = 12346;
        std::thread server_thread(run_udp_server, std::ref(io), server_port, 
                                 std::ref(received_messages), std::ref(server_ready), 
                                 std::ref(stop_server));
        
        // 等待服务器启动
        while (!server_ready) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // 创建客户端套接字
        uasio::datagram_socket client(io);
        uasio::error_code ec;
        
        // 打开套接字
        client.open(uasio::address_family::ipv4, ec);
        if (ec) {
            std::cerr << "打开客户端套接字失败: " << ec.message() << "\n";
            stop_server = true;
            server_thread.join();
            return false;
        }
        
        // 创建服务器端点
        uasio::endpoint server_endpoint(uasio::ip_address::from_string("127.0.0.1", ec), server_port);
        if (ec) {
            std::cerr << "创建服务器端点失败: " << ec.message() << "\n";
            stop_server = true;
            server_thread.join();
            return false;
        }
        
        // 使用缓冲区发送消息
        std::string message = "Buffer test message";
        uasio::const_buffer buffer(message.data(), message.size());
        std::size_t bytes_sent = client.send_to(buffer.data(), buffer.size(), server_endpoint, ec);
        if (ec || bytes_sent != message.size()) {
            std::cerr << "发送缓冲区消息失败: " << ec.message() << "\n";
            stop_server = true;
            server_thread.join();
            return false;
        }
        
        // 等待服务器接收消息
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // 检查服务器是否收到消息
        if (received_messages.empty() || received_messages[0] != message) {
            std::cerr << "服务器未收到正确的缓冲区消息\n";
            stop_server = true;
            server_thread.join();
            return false;
        }
        
        // 停止服务器
        stop_server = true;
        server_thread.join();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "测试异常: " << e.what() << "\n";
        return false;
    }
}

// 在单独的线程中运行UDP服务器
void run_udp_server(uasio::io_context& io_context, int port, 
                    std::vector<std::string>& received_messages,
                    std::atomic<bool>& server_ready,
                    std::atomic<bool>& stop_server) {
    try {
        uasio::datagram_socket socket(io_context); 
        uasio::error_code ec;

        // 打开套接字
        socket.open(uasio::address_family::ipv4, ec);
        if (ec) {
            std::cerr << "打开服务器套接字失败: " << ec.message() << "\n";
            return;
        }
        
        // 绑定套接字到地址
        uasio::endpoint endpoint(uasio::ip_address::any(), port);
        socket.bind(endpoint, ec);
        if (ec) {
            std::cerr << "绑定服务器套接字失败: " << ec.message() << "\n";
            return;
        }
        
        // 服务器已准备好
        server_ready = true;
        
        // 接收缓冲区
        char buffer[1024];
        uasio::endpoint sender_endpoint;
        
        auto start_time = std::chrono::steady_clock::now();
        // 持续接收消息直到停止信号
        while (!stop_server) {
            // 添加超时检查
            if (std::chrono::steady_clock::now() - start_time > std::chrono::seconds(5)) {
                break;
            }
            
            // 尝试接收数据
            std::size_t bytes_received = socket.receive_from(buffer, sizeof(buffer), sender_endpoint, ec);
            if (!ec && bytes_received > 0) {
                // 如果接收成功，添加到消息列表
                buffer[bytes_received] = '\0';
                received_messages.push_back(std::string(buffer, bytes_received));
            }
            else if (ec) {
                // 如果出错，报告并退出
                std::cerr << "接收数据失败: " << ec.message() << "\n";
                break;
            }
            
            // 短暂休眠，减少CPU使用
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // 关闭套接字
        socket.close(ec);
    }
    catch (const std::exception& e) {
        std::cerr << "服务器异常: " << e.what() << "\n";
    }
}
