// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SOCKET_H
#define SOCKET_H

#include "io_context.h"
#include "error.h"
#include <string>
#include <functional>
#include <memory>

namespace uasio {

/**
 * @brief IP 地址族
 */
enum class address_family {
    ipv4,   ///< IPv4 地址族
    ipv6    ///< IPv6 地址族
};

/**
 * @brief 套接字类型
 */
enum class socket_type {
    stream, ///< 流式套接字 (TCP)
    dgram   ///< 数据报套接字 (UDP)
};

// 前向声明
class socket_service;
class socket;
class tcp_acceptor;

/**
 * @brief IP 地址类
 */
class ip_address {
public:
    /**
     * @brief 默认构造函数，创建空地址
     */
    ip_address();
    
    /**
     * @brief 从字符串构造 IP 地址
     * @param address IP 地址字符串
     * @param ec 错误代码
     * @return IP 地址对象
     */
    static ip_address from_string(const std::string& address, error_code& ec);
    
    /**
     * @brief 从主机名解析 IP 地址
     * @param hostname 主机名
     * @param ec 错误代码
     * @return IP 地址对象
     */
    static ip_address from_hostname(const std::string& hostname, error_code& ec);
    
    /**
     * @brief 获取本地主机的 IP 地址
     * @param ec 错误代码
     * @return IP 地址对象
     */
    static ip_address local_host(error_code& ec);
    
    /**
     * @brief 获取任意地址
     * @param family 地址族
     * @return IP 地址对象
     */
    static ip_address any(address_family family = address_family::ipv4);
    
    /**
     * @brief 获取环回地址
     * @param family 地址族
     * @return IP 地址对象
     */
    static ip_address loopback(address_family family = address_family::ipv4);
    
    /**
     * @brief 转换为字符串
     * @return IP 地址字符串
     */
    std::string to_string() const;
    
    /**
     * @brief 获取地址族
     * @return 地址族
     */
    address_family family() const;
    
    /**
     * @brief 判断是否为 IPv4 地址
     * @return 是否为 IPv4 地址
     */
    bool is_ipv4() const;
    
    /**
     * @brief 判断是否为 IPv6 地址
     * @return 是否为 IPv6 地址
     */
    bool is_ipv6() const;
    
    /**
     * @brief 判断是否为环回地址
     * @return 是否为环回地址
     */
    bool is_loopback() const;
    
    /**
     * @brief 判断是否为任意地址
     * @return 是否为任意地址
     */
    bool is_any() const;
    
    /**
     * @brief 比较两个IP地址是否相等
     * @param other 另一个IP地址
     * @return 是否相等
     */
    bool operator==(const ip_address& other) const;
    
    /**
     * @brief 比较两个IP地址是否不相等
     * @param other 另一个IP地址
     * @return 是否不相等
     */
    bool operator!=(const ip_address& other) const;

private:
    // 内部实现
    address_family family_;
    union {
        uint8_t ipv4_[4];
        uint8_t ipv6_[16];
    } addr_;
    
    // 私有构造函数
    ip_address(address_family family);
};

/**
 * @brief 端点类，表示 IP 地址和端口
 */
class endpoint {
public:
    /**
     * @brief 默认构造函数，创建空端点
     */
    endpoint();
    
    /**
     * @brief 构造端点
     * @param address IP 地址
     * @param port 端口号
     */
    endpoint(const ip_address& address, uint16_t port);
    
    /**
     * @brief 获取 IP 地址
     * @return IP 地址
     */
    const ip_address& address() const;
    
    /**
     * @brief 获取端口号
     * @return 端口号
     */
    uint16_t port() const;
    
    /**
     * @brief 设置 IP 地址
     * @param address IP 地址
     */
    void set_address(const ip_address& address);
    
    /**
     * @brief 设置端口号
     * @param port 端口号
     */
    void set_port(uint16_t port);
    
    /**
     * @brief 转换为字符串
     * @return 端点字符串，格式为 "地址:端口"
     */
    std::string to_string() const;

private:
    ip_address address_;
    uint16_t port_;
};

/**
 * @brief 套接字类
 */
class socket {
public:
    /**
     * @brief 构造函数
     * @param io_context IO 上下文
     */
    explicit socket(io_context& io_context);
    
    /**
     * @brief 构造函数
     * @param io_context IO 上下文
     * @param family 地址族
     * @param type 套接字类型
     */
    socket(io_context& io_context, address_family family, socket_type type = socket_type::stream);
    
    /**
     * @brief 析构函数
     */
    ~socket();
    
    /**
     * @brief 检查套接字是否已打开
     * @return 是否已打开
     */
    bool is_open() const;
    
    /**
     * @brief 打开套接字
     * @param family 地址族
     * @param type 套接字类型
     * @param ec 错误代码
     * @return 是否成功
     */
    bool open(address_family family, socket_type type, error_code& ec);
    
    /**
     * @brief 关闭套接字
     * @param ec 错误代码
     */
    void close(error_code& ec);
    
    /**
     * @brief 绑定端点
     * @param endpoint 端点
     * @param ec 错误代码
     * @return 是否成功
     */
    bool bind(const endpoint& endpoint, error_code& ec);
    
    /**
     * @brief 连接到端点
     * @param endpoint 端点
     * @param ec 错误代码
     * @return 是否成功
     */
    bool connect(const endpoint& endpoint, error_code& ec);
    
    /**
     * @brief 异步连接到端点
     * @param endpoint 端点
     * @param handler 完成处理器
     */
    void async_connect(const endpoint& endpoint, std::function<void(const error_code&)>&& handler);
    
    /**
     * @brief 发送数据
     * @param data 数据指针
     * @param size 数据大小
     * @param ec 错误代码
     * @return 发送的字节数
     */
    std::size_t send(const void* data, std::size_t size, error_code& ec);
    
    /**
     * @brief 接收数据
     * @param data 数据指针
     * @param size 数据大小
     * @param ec 错误代码
     * @return 接收的字节数
     */
    std::size_t receive(void* data, std::size_t size, error_code& ec);
    
    /**
     * @brief 异步发送数据
     * @param data 数据指针
     * @param size 数据大小
     * @param handler 完成处理器
     */
    void async_send(const void* data, std::size_t size,
                   std::function<void(const error_code&, std::size_t)>&& handler);
    
    /**
     * @brief 异步接收数据
     * @param data 数据指针
     * @param size 数据大小
     * @param handler 完成处理器
     */
    void async_receive(void* data, std::size_t size,
                      std::function<void(const error_code&, std::size_t)>&& handler);
    
    /**
     * @brief 发送数据到指定端点
     * @param data 数据指针
     * @param size 数据大小
     * @param endpoint 目标端点
     * @param ec 错误代码
     * @return 发送的字节数
     */
    std::size_t send_to(const void* data, std::size_t size,
                       const endpoint& endpoint, error_code& ec);
    
    /**
     * @brief 从指定端点接收数据
     * @param data 数据指针
     * @param size 数据大小
     * @param endpoint 源端点
     * @param ec 错误代码
     * @return 接收的字节数
     */
    std::size_t receive_from(void* data, std::size_t size,
                           endpoint& endpoint, error_code& ec);
    
    /**
     * @brief 异步发送数据到指定端点
     * @param data 数据指针
     * @param size 数据大小
     * @param endpoint 目标端点
     * @param handler 完成处理器
     */
    void async_send_to(const void* data, std::size_t size,
                      const endpoint& endpoint,
                      std::function<void(const error_code&, std::size_t)>&& handler);
    
    /**
     * @brief 异步从指定端点接收数据
     * @param data 数据指针
     * @param size 数据大小
     * @param endpoint 源端点
     * @param handler 完成处理器
     */
    void async_receive_from(void* data, std::size_t size,
                          endpoint& endpoint,
                          std::function<void(const error_code&, std::size_t)>&& handler);
    
    /**
     * @brief 设置套接字选项
     * @param level 选项级别
     * @param option_name 选项名称
     * @param option_value 选项值
     * @param option_size 选项值大小
     * @param ec 错误代码
     * @return 是否成功
     */
    bool setsockopt(int level, int option_name,
                   const void* option_value, std::size_t option_size,
                   error_code& ec);

    /**
     * @brief 获取本地端点
     * @param ec 错误代码
     * @return 本地端点
     */
    endpoint local_endpoint(error_code& ec) const;

    /**
     * @brief 获取远程端点
     * @param ec 错误代码
     * @return 远程端点
     */
    endpoint remote_endpoint(error_code& ec) const;
    
    /**
     * @brief 获取套接字文件描述符
     * @return 文件描述符
     */
    int fd() const { return socket_fd_; }

private:
    friend class tcp_acceptor;
    
    // 服务类型
    std::shared_ptr<socket_service> service_;
    // IO 上下文
    io_context& io_context_;
    // 套接字描述符
    int socket_fd_;
    // 是否已连接
    bool connected_;
};

/**
 * @brief TCP 接收器类
 */
class tcp_acceptor {
public:
    /**
     * @brief 构造函数
     * @param io_context IO 上下文
     */
    explicit tcp_acceptor(io_context& io_context);
    
    /**
     * @brief 构造函数
     * @param io_context IO 上下文
     * @param endpoint 本地端点
     */
    tcp_acceptor(io_context& io_context, const endpoint& endpoint);
    
    /**
     * @brief 析构函数
     */
    ~tcp_acceptor();
    
    /**
     * @brief 打开接收器
     * @param family 地址族
     * @param ec 错误代码
     * @return 是否成功
     */
    bool open(address_family family, error_code& ec);
    
    /**
     * @brief 关闭接收器
     * @param ec 错误代码
     */
    void close(error_code& ec);
    
    /**
     * @brief 绑定端点
     * @param endpoint 端点
     * @param ec 错误代码
     * @return 是否成功
     */
    bool bind(const endpoint& endpoint, error_code& ec);
    
    /**
     * @brief 开始监听
     * @param backlog 队列大小
     * @param ec 错误代码
     * @return 是否成功
     */
    bool listen(int backlog, error_code& ec);
    
    /**
     * @brief 接受连接
     * @param socket 接受连接的套接字
     * @param ec 错误代码
     * @return 是否成功
     */
    bool accept(socket& socket, error_code& ec);
    
    /**
     * @brief 异步接受连接
     * @param socket 接受连接的套接字
     * @param handler 处理器，格式为 void(const error_code&)
     */
    void async_accept(socket& socket, std::function<void(const error_code&)>&& handler);
    
    /**
     * @brief 获取本地端点
     * @param ec 错误代码
     * @return 本地端点
     */
    endpoint local_endpoint(error_code& ec) const;
    
    /**
     * @brief 检查接收器是否已打开
     * @return 是否已打开
     */
    bool is_open() const;

    /**
     * @brief 接收器服务类
     */
    class acceptor_service {
    public:
        /**
         * @brief 构造函数
         * @param io_context IO 上下文
         */
        explicit acceptor_service(io_context& io_context);
        
        /**
         * @brief 析构函数
         */
        ~acceptor_service();
        
        /**
         * @brief 打开接收器
         * @param family 地址族
         * @param acceptor_fd 接收器句柄
         * @param ec 错误代码
         * @return 是否成功
         */
        bool open(address_family family, int& acceptor_fd, error_code& ec);
        
        /**
         * @brief 关闭接收器
         * @param acceptor_fd 接收器句柄
         * @param ec 错误代码
         */
        void close(int acceptor_fd, error_code& ec);
        
        /**
         * @brief 绑定端点
         * @param acceptor_fd 接收器句柄
         * @param endpoint 端点
         * @param ec 错误代码
         * @return 是否成功
         */
        bool bind(int acceptor_fd, const endpoint& endpoint, error_code& ec);
        
        /**
         * @brief 开始监听
         * @param acceptor_fd 接收器句柄
         * @param backlog 队列大小
         * @param ec 错误代码
         * @return 是否成功
         */
        bool listen(int acceptor_fd, int backlog, error_code& ec);
        
        /**
         * @brief 接受连接
         * @param acceptor_fd 接收器句柄
         * @param socket 接受连接的套接字
         * @param ec 错误代码
         * @return 是否成功
         */
        bool accept(int acceptor_fd, socket& socket, error_code& ec);
        
        /**
         * @brief 异步接受连接
         * @param acceptor_fd 接收器句柄
         * @param socket 接受连接的套接字
         * @param handler 处理器，格式为 void(const error_code&)
         */
        void async_accept(int acceptor_fd, socket& socket, 
                        std::function<void(const error_code&)>&& handler);
        
        /**
         * @brief 获取本地端点
         * @param acceptor_fd 接收器句柄
         * @param ec 错误代码
         * @return 本地端点
         */
        endpoint local_endpoint(int acceptor_fd, error_code& ec) const;
        
        /**
         * @brief 设置非阻塞模式
         * @param acceptor_fd 接收器句柄
         * @param ec 错误代码
         * @return 是否成功
         */
        bool set_non_blocking(int acceptor_fd, error_code& ec);
        
        /**
         * @brief 启动异步操作
         * @param acceptor_fd 接收器句柄
         * @param socket 接受连接的套接字
         * @param handler 处理器
         */
        void start_async_operation(int acceptor_fd, socket& socket, 
                                std::function<void(const error_code&)>&& handler);
        
        /**
         * @brief 检查接收器是否有效
         * @param acceptor_fd 接收器句柄
         * @return 是否有效
         */
        bool is_acceptor_valid(int acceptor_fd) const;
        
    private:
        io_context& io_context_;
    };

private:
    // 服务类型
    std::shared_ptr<acceptor_service> service_;
    // 接收器句柄
    int acceptor_fd_;
};

} // namespace uasio

#endif // SOCKET_H 