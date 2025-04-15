#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include "../src/connection_pool.h"
#include "../src/socket.h"
#include "../src/io_context.h"

// 简单的连接包装类
class tcp_connection {
public:
    tcp_connection() : socket_(nullptr) {}
    
    explicit tcp_connection(uasio::socket* socket) : socket_(socket) {}
    
    // 移动构造
    tcp_connection(tcp_connection&& other) noexcept : socket_(other.socket_) {
        other.socket_ = nullptr;
    }
    
    // 移动赋值
    tcp_connection& operator=(tcp_connection&& other) noexcept {
        if (this != &other) {
            close();
            socket_ = other.socket_;
            other.socket_ = nullptr;
        }
        return *this;
    }
    
    // 析构函数
    ~tcp_connection() {
        close();
    }
    
    // 关闭连接
    void close() {
        if (socket_) {
            socket_->close();
            delete socket_;
            socket_ = nullptr;
        }
    }
    
    // 检查连接是否打开
    bool is_open() const {
        return socket_ && socket_->is_open();
    }
    
    // 发送数据
    void send(const std::string& data) {
        if (is_open()) {
            socket_->write_some(data);
        }
    }
    
    // 获取远程地址
    std::string remote_address() const {
        if (is_open()) {
            return socket_->remote_endpoint();
        }
        return "not connected";
    }
    
private:
    uasio::socket* socket_;
    
    // 禁止复制
    tcp_connection(const tcp_connection&) = delete;
    tcp_connection& operator=(const tcp_connection&) = delete;
};

int main() {
    std::cout << "=== 连接池示例 ===" << std::endl;
    
    // 创建IO上下文
    uasio::io_context io_ctx;
    
    // 模拟连接工厂函数（实际应用中会进行真正的连接）
    auto connection_factory = [](std::function<void(tcp_connection, uasio::error_code)> callback) {
        // 模拟创建连接的延迟
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // 创建一个新的socket（这里模拟，实际应用中会进行真正的连接）
        auto* socket = new uasio::socket(nullptr);
        
        // 随机决定是否创建成功
        if (rand() % 10 < 9) {
            // 成功率90%
            callback(tcp_connection(socket), uasio::error_code());
        } else {
            // 失败率10%
            delete socket;
            callback(tcp_connection(), uasio::make_error_code(uasio::error::connection_refused));
        }
    };
    
    // 创建连接池
    uasio::connection_pool<tcp_connection> pool(io_ctx, connection_factory, 3, 10);
    
    // 设置连接验证函数
    pool.set_validator([](const tcp_connection& conn) {
        return conn.is_open();
    });
    
    // 设置连接超时
    pool.set_idle_timeout(std::chrono::seconds(5));
    
    std::cout << "初始化连接池: 初始连接数=3, 最大连接数=10" << std::endl;
    std::cout << "等待连接池初始化..." << std::endl;
    
    // 等待连接池初始化完成
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::cout << "可用连接数: " << pool.available_count() << std::endl;
    std::cout << "总连接数: " << pool.total_count() << std::endl;
    
    // 创建一些线程来模拟客户端
    std::vector<std::thread> clients;
    
    std::cout << "\n模拟5个客户端同时获取连接..." << std::endl;
    
    for (int i = 0; i < 5; ++i) {
        clients.emplace_back([i, &pool]() {
            std::cout << "客户端 " << i << " 正在获取连接..." << std::endl;
            
            pool.get_connection([i](tcp_connection conn, uasio::error_code ec) {
                if (!ec) {
                    std::cout << "客户端 " << i << " 获取连接成功: " << conn.remote_address() << std::endl;
                    
                    // 模拟使用连接
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    
                    std::cout << "客户端 " << i << " 正在释放连接" << std::endl;
                    pool.release_connection(std::move(conn));
                } else {
                    std::cout << "客户端 " << i << " 获取连接失败: " << ec.message() << std::endl;
                }
            });
            
            // 客户端操作完毕后，等待一些时间
            std::this_thread::sleep_for(std::chrono::seconds(2));
        });
    }
    
    // 等待所有客户端线程完成
    for (auto& t : clients) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    std::cout << "\n连接池状态: " << std::endl;
    std::cout << "可用连接数: " << pool.available_count() << std::endl;
    std::cout << "总连接数: " << pool.total_count() << std::endl;
    
    std::cout << "\n等待5秒，让一些连接超时..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(6));
    
    std::cout << "连接池状态（超时后）: " << std::endl;
    std::cout << "可用连接数: " << pool.available_count() << std::endl;
    std::cout << "总连接数: " << pool.total_count() << std::endl;
    
    std::cout << "\n关闭连接池..." << std::endl;
    pool.shutdown();
    
    std::cout << "示例结束" << std::endl;
    
    return 0;
} 