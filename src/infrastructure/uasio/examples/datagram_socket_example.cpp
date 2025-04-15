// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file datagram_socket_example.cpp
 * @brief 演示datagram_socket的用法
 * 
 * 本示例创建一个UDP服务器和客户端，展示UDP通信的基本用法。
 */

#include "../src/asio.h"
#include <iostream>
#include <thread>
#include <string>
#include <atomic>
#include <chrono>
#include <vector>
#include <cstring>

// 服务器类
class UdpServer {
public:
    UdpServer(asio::io_context& io_context, int port)
        : io_context_(io_context),
          socket_(io_context),
          stopped_(false)
    {
        asio::error_code ec;
        
        // 打开套接字
        if (!socket_.open(asio::address_family::ipv4, ec)) {
            std::cerr << "打开套接字失败: " << ec.message() << std::endl;
            return;
        }
        
        // 绑定到本地地址
        asio::endpoint endpoint(asio::ip_address::any(), port);
        if (!socket_.bind(endpoint, ec)) {
            std::cerr << "绑定套接字失败: " << ec.message() << std::endl;
            return;
        }
        
        std::cout << "UDP服务器启动成功，监听端口: " << port << std::endl;
        
        // 启动消息接收
        start_receive();
    }
    
    ~UdpServer() {
        stop();
    }
    
    void stop() {
        stopped_ = true;
        
        asio::error_code ec;
        socket_.close(ec);
    }
    
private:
    void start_receive() {
        if (stopped_) {
            return;
        }
        
        // 准备接收数据
        socket_.async_receive_from(
            receive_buffer_, sizeof(receive_buffer_),
            sender_endpoint_,
            [this](const asio::error_code& ec, std::size_t bytes_received) {
                if (stopped_) {
                    return;
                }
                
                if (!ec && bytes_received > 0) {
                    // 处理收到的消息
                    receive_buffer_[bytes_received] = '\0'; // 确保以null结尾
                    
                    std::cout << "服务器收到来自 " 
                              << sender_endpoint_.address().to_string() 
                              << ":" << sender_endpoint_.port()
                              << " 的消息: " << receive_buffer_ << std::endl;
                    
                    // 回显消息
                    std::string response = "服务器已收到: ";
                    response += receive_buffer_;
                    
                    socket_.async_send_to(
                        response.c_str(), response.size(),
                        sender_endpoint_,
                        [this](const asio::error_code& ec, std::size_t /*bytes_sent*/) {
                            if (ec) {
                                std::cerr << "发送回复失败: " << ec.message() << std::endl;
                            }
                            
                            // 继续接收下一个消息
                            start_receive();
                        });
                }
                else {
                    if (ec) {
                        std::cerr << "接收数据失败: " << ec.message() << std::endl;
                    }
                    
                    // 继续接收下一个消息
                    start_receive();
                }
            });
    }
    
    asio::io_context& io_context_;
    asio::datagram_socket socket_;
    asio::endpoint sender_endpoint_;
    std::atomic<bool> stopped_;
    char receive_buffer_[1024];
};

// 客户端类
class UdpClient {
public:
    UdpClient(asio::io_context& io_context, const std::string& host, int port)
        : io_context_(io_context),
          socket_(io_context),
          server_endpoint_(asio::ip_address::from_string(host, ec_), port),
          stopped_(false)
    {
        if (ec_) {
            std::cerr << "解析地址失败: " << ec_.message() << std::endl;
            return;
        }
        
        // 打开套接字
        if (!socket_.open(asio::address_family::ipv4, ec_)) {
            std::cerr << "打开套接字失败: " << ec_.message() << std::endl;
            return;
        }
        
        std::cout << "UDP客户端启动成功，服务器地址: " << host << ":" << port << std::endl;
        
        // 启动消息接收
        start_receive();
    }
    
    ~UdpClient() {
        stop();
    }
    
    void stop() {
        stopped_ = true;
        
        asio::error_code ec;
        socket_.close(ec);
    }
    
    void send_message(const std::string& message) {
        if (stopped_) {
            std::cerr << "客户端已停止，无法发送消息" << std::endl;
            return;
        }
        
        // 发送消息到服务器
        socket_.async_send_to(
            message.c_str(), message.size(),
            server_endpoint_,
            [this, message](const asio::error_code& ec, std::size_t /*bytes_sent*/) {
                if (ec) {
                    std::cerr << "发送消息失败: " << ec.message() << std::endl;
                    return;
                }
                
                std::cout << "客户端已发送消息: " << message << std::endl;
            });
    }
    
private:
    void start_receive() {
        if (stopped_) {
            return;
        }
        
        // 准备接收数据
        socket_.async_receive_from(
            receive_buffer_, sizeof(receive_buffer_),
            sender_endpoint_,
            [this](const asio::error_code& ec, std::size_t bytes_received) {
                if (stopped_) {
                    return;
                }
                
                if (!ec && bytes_received > 0) {
                    // 处理收到的消息
                    receive_buffer_[bytes_received] = '\0'; // 确保以null结尾
                    
                    std::cout << "客户端收到来自 " 
                              << sender_endpoint_.address().to_string() 
                              << ":" << sender_endpoint_.port()
                              << " 的消息: " << receive_buffer_ << std::endl;
                }
                else if (ec) {
                    std::cerr << "接收数据失败: " << ec.message() << std::endl;
                }
                
                // 继续接收下一个消息
                start_receive();
            });
    }
    
    asio::io_context& io_context_;
    asio::datagram_socket socket_;
    asio::endpoint server_endpoint_;
    asio::endpoint sender_endpoint_;
    asio::error_code ec_;
    std::atomic<bool> stopped_;
    char receive_buffer_[1024];
};

// 创建服务器并在单独的线程中运行
void run_server(asio::io_context& io_context, int port) {
    UdpServer server(io_context, port);
    
    std::cout << "服务器线程启动" << std::endl;
    io_context.run();
    std::cout << "服务器线程结束" << std::endl;
}

int main() {
    try {
        const int port = 8888;
        const std::string server_address = "127.0.0.1";
        
        // 创建IO上下文
        asio::io_context io_context;
        
        // 创建一个工作守卫，防止IO上下文过早退出
        auto work = io_context.make_work_guard();
        
        // 在单独的线程中启动服务器
        std::thread server_thread(run_server, std::ref(io_context), port);
        
        // 等待服务器启动
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // 创建客户端
        UdpClient client(io_context, server_address, port);
        
        // 创建线程运行IO上下文
        std::thread io_thread([&io_context]() {
            std::cout << "IO线程启动" << std::endl;
            io_context.run();
            std::cout << "IO线程结束" << std::endl;
        });
        
        // 发送一些测试消息
        std::vector<std::string> test_messages = {
            "你好，服务器！",
            "这是一条UDP测试消息",
            "UASIO库的UDP示例程序",
            "最后一条消息"
        };
        
        for (const auto& message : test_messages) {
            client.send_message(message);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // 等待一会儿让所有消息处理完成
        std::cout << "等待所有消息处理完成..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // 停止IO上下文
        std::cout << "停止IO上下文..." << std::endl;
        work.reset();
        
        // 等待线程结束
        if (io_thread.joinable()) {
            io_thread.join();
        }
        
        if (server_thread.joinable()) {
            server_thread.join();
        }
        
        std::cout << "程序正常退出" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 