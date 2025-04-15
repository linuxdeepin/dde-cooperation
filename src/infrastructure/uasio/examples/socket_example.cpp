// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "asio.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <memory>

// 服务器类
class Server {
public:
    Server(int port) 
        : io_context_(), 
          acceptor_(io_context_, asio::endpoint(asio::ip_address::any(), port)),
          stopped_(false) {
        std::cout << "服务器启动在端口: " << port << std::endl;
    }

    // 启动服务器
    void start() {
        // 开始接受连接
        accept();

        // 创建一个线程运行 io_context
        thread_ = std::thread([this]() {
            try {
                io_context_.run();
            } catch (const std::exception& e) {
                std::cerr << "服务器异常: " << e.what() << std::endl;
            }
        });
    }

    // 停止服务器
    void stop() {
        stopped_ = true;
        io_context_.stop();
        
        if (thread_.joinable()) {
            thread_.join();
        }
        
        std::cout << "服务器已停止" << std::endl;
    }

    ~Server() {
        stop();
    }

private:
    // 接受新连接
    void accept() {
        if (stopped_) return;

        std::shared_ptr<asio::socket> client_socket = 
            std::make_shared<asio::socket>(io_context_);
        
        acceptor_.async_accept(*client_socket, [this, client_socket](const asio::error_code& ec) {
            if (!ec) {
                std::cout << "新客户端连接: " 
                          << client_socket->remote_endpoint().address().to_string() 
                          << ":" << client_socket->remote_endpoint().port() << std::endl;
                
                // 处理客户端连接
                handle_client(client_socket);
            } else {
                std::cerr << "接受连接错误: " << ec.message() << std::endl;
            }
            
            // 继续接受下一个连接
            accept();
        });
    }
    
    // 处理客户端
    void handle_client(std::shared_ptr<asio::socket> client_socket) {
        auto buffer = std::make_shared<std::vector<char>>(1024);
        
        // 异步读取数据
        client_socket->async_receive(
            buffer->data(), buffer->size(),
            [this, client_socket, buffer](const asio::error_code& ec, std::size_t bytes_received) {
                if (!ec && bytes_received > 0) {
                    // 收到数据
                    std::string message(buffer->data(), bytes_received);
                    std::cout << "收到: " << message << std::endl;
                    
                    // 回复数据
                    std::string response = "服务器回复: " + message;
                    client_socket->async_send(
                        response.data(), response.size(),
                        [client_socket](const asio::error_code& ec, std::size_t) {
                            if (ec) {
                                std::cerr << "发送回复错误: " << ec.message() << std::endl;
                            }
                        }
                    );
                    
                    // 继续读取
                    handle_client(client_socket);
                } else if (ec == asio::error::connection_closed) {
                    std::cout << "客户端断开连接" << std::endl;
                } else if (ec) {
                    std::cerr << "读取错误: " << ec.message() << std::endl;
                }
            }
        );
    }

    asio::io_context io_context_;
    asio::tcp_acceptor acceptor_;
    std::thread thread_;
    std::atomic<bool> stopped_;
};

// 客户端类
class Client {
public:
    Client(const std::string& host, int port)
        : io_context_(),
          socket_(io_context_),
          stopped_(false) {
        asio::error_code ec;
        asio::ip_address addr = asio::ip_address::from_string(host, ec);
        if (ec) {
            // 如果不是 IP 地址，尝试解析主机名
            addr = asio::ip_address::from_hostname(host, ec);
            if (ec) {
                throw std::runtime_error("无法解析主机名: " + host);
            }
        }
        
        endpoint_ = asio::endpoint(addr, port);
        std::cout << "客户端连接到: " << host << ":" << port << std::endl;
    }
    
    // 连接到服务器
    bool connect() {
        asio::error_code ec;
        socket_.connect(endpoint_, ec);
        if (ec) {
            std::cerr << "连接错误: " << ec.message() << std::endl;
            return false;
        }
        
        std::cout << "已连接到服务器" << std::endl;
        
        // 启动 io_context 线程
        thread_ = std::thread([this]() {
            try {
                io_context_.run();
            } catch (const std::exception& e) {
                std::cerr << "客户端异常: " << e.what() << std::endl;
            }
        });
        
        return true;
    }
    
    // 发送消息
    bool send_message(const std::string& message) {
        if (!socket_.is_open()) {
            std::cerr << "套接字未打开" << std::endl;
            return false;
        }
        
        asio::error_code ec;
        socket_.send(message.data(), message.size(), ec);
        if (ec) {
            std::cerr << "发送错误: " << ec.message() << std::endl;
            return false;
        }
        
        // 接收回复
        std::vector<char> buffer(1024);
        std::size_t bytes_received = socket_.receive(buffer.data(), buffer.size(), ec);
        if (ec) {
            std::cerr << "接收错误: " << ec.message() << std::endl;
            return false;
        }
        
        std::string response(buffer.data(), bytes_received);
        std::cout << "收到服务器回复: " << response << std::endl;
        
        return true;
    }
    
    // 停止客户端
    void stop() {
        stopped_ = true;
        io_context_.stop();
        
        if (thread_.joinable()) {
            thread_.join();
        }
        
        if (socket_.is_open()) {
            asio::error_code ec;
            socket_.close(ec);
        }
        
        std::cout << "客户端已停止" << std::endl;
    }
    
    ~Client() {
        stop();
    }
    
private:
    asio::io_context io_context_;
    asio::socket socket_;
    asio::endpoint endpoint_;
    std::thread thread_;
    std::atomic<bool> stopped_;
};

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            std::cerr << "用法: " << argv[0] << " [server|client]" << std::endl;
            return 1;
        }
        
        std::string mode = argv[1];
        
        if (mode == "server") {
            // 服务器模式
            Server server(12345);
            server.start();
            
            std::cout << "按 Enter 键停止服务器..." << std::endl;
            std::cin.get();
            
            server.stop();
        } else if (mode == "client") {
            // 客户端模式
            Client client("127.0.0.1", 12345);
            if (!client.connect()) {
                return 1;
            }
            
            while (true) {
                std::cout << "输入消息 (q 退出): ";
                std::string message;
                std::getline(std::cin, message);
                
                if (message == "q") break;
                
                client.send_message(message);
            }
            
            client.stop();
        } else {
            std::cerr << "未知模式: " << mode << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 