#include <iostream>
#include <string>
#include <memory>
#include "../src/asio.h"
#include "../src/ssl.h"
#include "../src/error.h"
#include "../src/io_context.h"
#include "../src/resolver.h"
#include "../src/multiple_exceptions.h"
#include "../src/future.h"
#include "../src/promise.h"

// HTTPS客户端类
class https_client {
public:
    https_client(uasio::io_context& io_ctx, const std::string& host, const std::string& port)
        : io_ctx_(io_ctx),
          host_(host),
          resolver_(io_ctx),
          ssl_ctx_(uasio::ssl::context::sslv23_client),
          socket_(io_ctx, ssl_ctx_) {
        
        // 配置SSL上下文
        ssl_ctx_.set_options(
            uasio::ssl::context::default_workarounds |
            uasio::ssl::context::no_sslv2 |
            uasio::ssl::context::no_sslv3);
        
        // 设置默认SSL验证
        ssl_ctx_.set_default_verify_paths();
        
        // 设置验证回调函数
        ssl_ctx_.set_verify_mode(uasio::ssl::verify_peer);
        ssl_ctx_.set_verify_callback([](bool preverified, uasio::ssl::verify_context& ctx) {
            // 这里可以添加自定义验证逻辑
            // 为了示例简单，我们接受所有证书
            return true;
        });
        
        // 解析主机名
        try {
            auto endpoints = resolver_.resolve(host, port);
            
            // 连接到第一个可用的端点
            connect(endpoints);
        } catch (const std::exception& e) {
            std::cerr << "解析主机失败: " << e.what() << std::endl;
            throw;
        }
    }
    
    // 发送HTTP请求
    uasio::future<std::string> send_request(const std::string& method, const std::string& path, 
                                           const std::string& content = "", 
                                           const std::string& content_type = "text/plain") {
        // 创建Promise-Future对
        auto promise = std::make_shared<uasio::promise<std::string>>();
        auto future = promise->get_future();
        
        // 构建HTTP请求
        std::string request = method + " " + path + " HTTP/1.1\r\n";
        request += "Host: " + host_ + "\r\n";
        request += "Accept: */*\r\n";
        
        if (!content.empty()) {
            request += "Content-Type: " + content_type + "\r\n";
            request += "Content-Length: " + std::to_string(content.length()) + "\r\n";
            request += "\r\n";
            request += content;
        } else {
            request += "Connection: close\r\n";
            request += "\r\n";
        }
        
        // 发送请求
        socket_.async_write(request.c_str(), request.length(),
            [this, promise](const uasio::error_code& ec, std::size_t) {
                if (ec) {
                    try {
                        promise->set_exception(std::make_exception_ptr(uasio::system_error(ec)));
                    } catch (...) {
                        // 忽略任何设置异常时的错误
                    }
                    return;
                }
                
                // 读取响应
                read_response(promise);
            });
        
        return future;
    }
    
    // 关闭连接
    void close() {
        uasio::error_code ec;
        socket_.shutdown(ec);
        if (ec) {
            std::cerr << "关闭SSL连接时发生错误: " << ec.message() << std::endl;
        }
    }
    
private:
    // 连接到远程服务器
    void connect(const uasio::resolver::results_type& endpoints) {
        uasio::async_connect(socket_.lowest_layer(), endpoints,
            [this](const uasio::error_code& ec, const uasio::endpoint& endpoint) {
                if (ec) {
                    std::cerr << "连接失败: " << ec.message() << std::endl;
                    return;
                }
                
                // 连接成功，执行SSL握手
                socket_.async_handshake(uasio::ssl::socket::client,
                    [this](const uasio::error_code& ec) {
                        if (ec) {
                            std::cerr << "SSL握手失败: " << ec.message() << std::endl;
                        }
                    });
            });
    }
    
    // 读取HTTP响应
    void read_response(std::shared_ptr<uasio::promise<std::string>> promise) {
        auto response_buf = std::make_shared<uasio::streambuf>();
        
        socket_.async_read_some(*response_buf, 1024,
            [this, response_buf, promise](const uasio::error_code& ec, std::size_t bytes_transferred) {
                if (ec) {
                    if (ec == uasio::error::eof) {
                        // 连接被服务器关闭，这是正常的
                        std::string response(response_buf->data(), response_buf->size());
                        try {
                            promise->set_value(response);
                        } catch (...) {
                            // 忽略任何设置值时的错误
                        }
                    } else {
                        // 其他错误
                        try {
                            promise->set_exception(std::make_exception_ptr(uasio::system_error(ec)));
                        } catch (...) {
                            // 忽略任何设置异常时的错误
                        }
                    }
                    return;
                }
                
                // 如果有更多数据要读取，继续读取
                if (bytes_transferred == 1024) {
                    read_response(promise);
                } else {
                    // 全部读取完成
                    std::string response(response_buf->data(), response_buf->size());
                    try {
                        promise->set_value(response);
                    } catch (...) {
                        // 忽略任何设置值时的错误
                    }
                }
            });
    }
    
    uasio::io_context& io_ctx_;
    std::string host_;
    uasio::resolver resolver_;
    uasio::ssl::context ssl_ctx_;
    uasio::ssl::socket socket_;
};

// 解析HTTP响应
std::pair<int, std::string> parse_http_response(const std::string& response) {
    // 检查是否为空响应
    if (response.empty()) {
        return {0, ""};
    }
    
    // 解析状态码
    size_t pos = response.find(' ');
    if (pos == std::string::npos) {
        return {0, ""};
    }
    
    pos++;
    size_t status_end = response.find(' ', pos);
    if (status_end == std::string::npos) {
        return {0, ""};
    }
    
    int status_code = std::stoi(response.substr(pos, status_end - pos));
    
    // 查找响应体
    pos = response.find("\r\n\r\n");
    if (pos == std::string::npos) {
        return {status_code, ""};
    }
    
    std::string body = response.substr(pos + 4);
    return {status_code, body};
}

int main() {
    try {
        std::cout << "=== HTTPS客户端示例 ===" << std::endl;
        
        // 创建IO上下文
        uasio::io_context io_ctx;
        
        // 提示用户输入主机和路径
        std::string host, path;
        std::cout << "请输入要连接的主机 (默认: example.com): ";
        std::getline(std::cin, host);
        if (host.empty()) {
            host = "example.com";
        }
        
        std::cout << "请输入要请求的路径 (默认: /): ";
        std::getline(std::cin, path);
        if (path.empty()) {
            path = "/";
        }
        
        std::cout << "连接到: " << host << path << std::endl;
        
        // 创建HTTPS客户端
        https_client client(io_ctx, host, "443");
        
        // 发送GET请求
        std::cout << "发送GET请求..." << std::endl;
        auto future = client.send_request("GET", path);
        
        // 启动IO上下文，等待响应
        std::thread io_thread([&io_ctx]() {
            io_ctx.run();
        });
        
        // 等待响应
        std::cout << "等待响应..." << std::endl;
        
        try {
            // 带超时的等待
            if (future.wait_for(std::chrono::seconds(10))) {
                // 获取响应
                std::string response = future.get();
                
                // 解析响应
                auto [status_code, body] = parse_http_response(response);
                std::cout << "收到HTTP状态码: " << status_code << std::endl;
                
                // 限制输出响应体的大小
                if (body.size() > 1000) {
                    std::cout << "响应体 (前1000字节): " << std::endl;
                    std::cout << body.substr(0, 1000) << "..." << std::endl;
                } else {
                    std::cout << "响应体: " << std::endl;
                    std::cout << body << std::endl;
                }
            } else {
                std::cout << "请求超时" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "处理响应时发生错误: " << e.what() << std::endl;
        }
        
        // 关闭连接并停止IO上下文
        client.close();
        io_ctx.stop();
        
        // 等待IO线程结束
        if (io_thread.joinable()) {
            io_thread.join();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "HTTPS客户端示例完成" << std::endl;
    return 0;
} 