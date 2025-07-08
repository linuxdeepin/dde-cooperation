#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <vector>
#include "../src/asio.h"
#include "../src/ssl.h"
#include "../src/thread_pool.h"
#include "../src/error.h"
#include "../src/socket.h"
#include "../src/io_context.h"
#include "../src/signal_set.h"

// HTTP 响应内容构建器
std::string build_http_response(const std::string& content, const std::string& content_type = "text/html") {
    std::string response = 
        "HTTP/1.1 200 OK\r\n"
        "Server: uasio-https-server\r\n"
        "Content-Type: " + content_type + "\r\n"
        "Content-Length: " + std::to_string(content.length()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" + content;
    return response;
}

// 解析HTTP请求，返回请求的路径
std::string parse_http_request(const std::string& request) {
    // 简单解析HTTP请求的第一行，获取请求路径
    size_t pos = request.find(' ');
    if (pos == std::string::npos) {
        return "/";
    }
    
    size_t end_pos = request.find(' ', pos + 1);
    if (end_pos == std::string::npos) {
        return "/";
    }
    
    return request.substr(pos + 1, end_pos - pos - 1);
}

// SSL Socket连接处理类
class https_connection : public std::enable_shared_from_this<https_connection> {
public:
    https_connection(uasio::ssl::context& ssl_ctx, uasio::io_context& io_ctx)
        : socket_(io_ctx, ssl_ctx), data_() {
    }
    
    // 获取SSL Socket
    uasio::ssl::socket& socket() {
        return socket_;
    }
    
    // 开始处理连接
    void start() {
        auto self = shared_from_this();
        
        // 执行SSL握手
        socket_.async_handshake(uasio::ssl::socket::server, 
            [self](const uasio::error_code& ec) {
                if (!ec) {
                    self->read();
                } else {
                    std::cerr << "握手失败: " << ec.message() << std::endl;
                }
            });
    }
    
private:
    // 读取请求数据
    void read() {
        auto self = shared_from_this();
        
        socket_.async_read_some(data_, 1024, 
            [self](const uasio::error_code& ec, std::size_t bytes_transferred) {
                if (!ec) {
                    // 成功读取数据
                    std::string request(self->data_.data(), bytes_transferred);
                    std::cout << "收到请求: " << bytes_transferred << " 字节" << std::endl;
                    
                    // 解析HTTP请求
                    std::string path = parse_http_request(request);
                    std::cout << "请求路径: " << path << std::endl;
                    
                    // 构建响应
                    self->write(path);
                }
                // 如果有错误或连接关闭，不做任何处理，连接会自动释放
            });
    }
    
    // 发送响应数据
    void write(const std::string& path) {
        auto self = shared_from_this();
        
        // 根据路径构建不同的响应
        std::string content;
        if (path == "/" || path == "/index.html") {
            content = "<html><body><h1>HTTPS服务器示例</h1>"
                      "<p>这是使用uasio库构建的HTTPS服务器示例</p>"
                      "<p><a href=\"/about\">关于</a></p></body></html>";
        } else if (path == "/about") {
            content = "<html><body><h1>关于本示例</h1>"
                      "<p>这个示例展示了如何使用uasio库实现一个基本的HTTPS服务器</p>"
                      "<p><a href=\"/\">返回首页</a></p></body></html>";
        } else {
            content = "<html><body><h1>404 - 页面未找到</h1>"
                      "<p>请求的页面不存在</p>"
                      "<p><a href=\"/\">返回首页</a></p></body></html>";
        }
        
        // 构建完整HTTP响应
        std::string response = build_http_response(content);
        
        // 发送响应数据
        socket_.async_write(response.c_str(), response.length(),
            [self](const uasio::error_code& ec, std::size_t) {
                if (!ec) {
                    // 发送完成后，执行优雅关闭
                    self->shutdown();
                }
            });
    }
    
    // 优雅关闭SSL连接
    void shutdown() {
        auto self = shared_from_this();
        
        socket_.async_shutdown(
            [self](const uasio::error_code& ec) {
                // 即使关闭出错，这里也不再处理，连接会自动释放
                if (ec) {
                    std::cerr << "关闭连接时发生错误: " << ec.message() << std::endl;
                }
            });
    }
    
    uasio::ssl::socket socket_;
    uasio::streambuf data_;
};

// HTTPS服务器类
class https_server {
public:
    // 公开IO上下文，以便外部使用
    uasio::io_context io_ctx_;
    
    https_server(const std::string& address, int port, 
                 const std::string& cert_file, const std::string& key_file)
        : io_ctx_(),
          thread_pool_(4),  // 使用4线程的线程池
          acceptor_(io_ctx_, address, port),
          ssl_ctx_(uasio::ssl::context::sslv23) {
        
        // 配置SSL上下文
        ssl_ctx_.set_options(
            uasio::ssl::context::default_workarounds |
            uasio::ssl::context::no_sslv2 |
            uasio::ssl::context::single_dh_use);
        
        // 加载证书和私钥
        uasio::error_code ec;
        ssl_ctx_.use_certificate_file(cert_file, uasio::ssl::context::pem, ec);
        if (ec) {
            throw uasio::system_error(ec, "加载SSL证书失败");
        }
        
        ssl_ctx_.use_private_key_file(key_file, uasio::ssl::context::pem, ec);
        if (ec) {
            throw uasio::system_error(ec, "加载SSL私钥失败");
        }
        
        // 启动接受连接
        start_accept();
        
        // 启动线程池
        thread_pool_.run();
    }
    
    // 运行服务器
    void run() {
        // 在主线程中运行IO上下文
        io_ctx_.run();
    }
    
    // 停止服务器
    void stop() {
        io_ctx_.stop();
        thread_pool_.stop();
        thread_pool_.join();
    }
    
private:
    // 开始接受新连接
    void start_accept() {
        // 创建新的连接对象
        auto new_connection = std::make_shared<https_connection>(ssl_ctx_, io_ctx_);
        
        // 异步接受连接
        acceptor_.async_accept(new_connection->socket().lowest_layer(),
            [this, new_connection](const uasio::error_code& ec) {
                if (!ec) {
                    std::cout << "接受新连接" << std::endl;
                    
                    // 在线程池中处理连接
                    thread_pool_.post([new_connection]() {
                        new_connection->start();
                    });
                } else {
                    std::cerr << "接受连接时发生错误: " << ec.message() << std::endl;
                }
                
                // 继续接受新连接
                start_accept();
            });
    }
    
    uasio::thread_pool thread_pool_;
    uasio::socket_service acceptor_;
    uasio::ssl::context ssl_ctx_;
};

// 生成自签名证书的辅助函数（仅用于示例）
bool generate_self_signed_cert(const std::string& cert_file, const std::string& key_file) {
    std::cout << "生成自签名证书..." << std::endl;
    
    // 检查文件是否已存在
    FILE* cert_check = fopen(cert_file.c_str(), "r");
    FILE* key_check = fopen(key_file.c_str(), "r");
    
    if (cert_check && key_check) {
        fclose(cert_check);
        fclose(key_check);
        std::cout << "证书和密钥文件已存在，跳过生成" << std::endl;
        return true;
    }
    
    if (cert_check) fclose(cert_check);
    if (key_check) fclose(key_check);
    
    // 使用OpenSSL命令行工具生成自签名证书
    std::string cmd = "openssl req -x509 -newkey rsa:2048 -keyout " + key_file + 
                      " -out " + cert_file + 
                      " -days 365 -nodes -subj '/CN=localhost' 2>/dev/null";
    
    int result = system(cmd.c_str());
    if (result != 0) {
        std::cerr << "生成自签名证书失败，请确保已安装OpenSSL" << std::endl;
        return false;
    }
    
    std::cout << "成功生成自签名证书" << std::endl;
    return true;
}

int main() {
    try {
        std::cout << "=== HTTPS服务器示例 ===" << std::endl;
        
        // 证书文件路径
        std::string cert_file = "server.crt";
        std::string key_file = "server.key";
        
        // 生成自签名证书（仅用于示例）
        if (!generate_self_signed_cert(cert_file, key_file)) {
            std::cerr << "无法生成自签名证书，退出程序" << std::endl;
            return 1;
        }
        
        // 创建HTTPS服务器，监听8443端口
        https_server server("0.0.0.0", 8443, cert_file, key_file);
        
        std::cout << "HTTPS服务器已启动，监听端口8443" << std::endl;
        std::cout << "可以通过浏览器访问 localhost:8443" << std::endl;
        std::cout << "（注意：由于使用自签名证书，浏览器可能会显示安全警告）" << std::endl;
        std::cout << "按Ctrl+C终止服务器..." << std::endl;
        
        // 设置信号处理，优雅地关闭服务器
        uasio::signal_set signals(server.io_ctx_, SIGINT, SIGTERM);
        signals.async_wait([&server](uasio::error_code, int) {
            std::cout << "收到终止信号，正在停止服务器..." << std::endl;
            server.stop();
        });
        
        // 运行服务器
        server.run();
        
    } catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "HTTPS服务器已关闭" << std::endl;
    return 0;
} 