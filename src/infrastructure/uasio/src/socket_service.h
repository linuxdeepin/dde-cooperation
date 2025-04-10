// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_SOCKET_SERVICE_H
#define UASIO_SOCKET_SERVICE_H

#include "io_context.h"
#include "error.h"
#include "socket.h"
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace uasio {

// 常量定义
constexpr int FD_READ = 1;
constexpr int FD_WRITE = 2;

// Socket 服务类，用于处理 socket 底层操作
class socket_service {
public:
    /**
     * @brief 构造函数
     * @param io_context IO上下文
     */
    explicit socket_service(io_context& io_context);
    
    /**
     * @brief 析构函数
     */
    ~socket_service();
    
    /**
     * @brief 打开 socket
     * @param family 地址族
     * @param type socket 类型
     * @param socket_fd 要设置的 socket 描述符
     * @param ec 错误码
     * @return 成功返回 true，失败返回 false
     */
    bool open(address_family family, socket_type type, int& socket_fd, error_code& ec);
    
    /**
     * @brief 关闭 socket
     * @param socket_fd socket 描述符
     * @param ec 错误码
     */
    void close(int socket_fd, error_code& ec);
    
    /**
     * @brief 绑定本地地址
     * @param socket_fd socket 描述符
     * @param endpoint 本地端点
     * @param ec 错误码
     * @return 成功返回 true，失败返回 false
     */
    bool bind(int socket_fd, const endpoint& endpoint, error_code& ec);
    
    /**
     * @brief 连接到远程地址
     * @param socket_fd socket 描述符
     * @param endpoint 远程端点
     * @param ec 错误码
     * @return 成功返回 true，失败返回 false
     */
    bool connect(int socket_fd, const endpoint& endpoint, error_code& ec);
    
    /**
     * @brief 异步连接到远程地址
     * @param socket_fd socket 描述符
     * @param endpoint 远程端点
     * @param handler 完成处理器
     */
    void async_connect(int socket_fd, const endpoint& endpoint, 
                      std::function<void(const error_code&)>&& handler);
    
    /**
     * @brief 发送数据
     * @param socket_fd socket 描述符
     * @param data 数据指针
     * @param size 数据大小
     * @param ec 错误码
     * @return 已发送的字节数
     */
    std::size_t send(int socket_fd, const void* data, std::size_t size, error_code& ec);
    
    /**
     * @brief 异步发送数据
     * @param socket_fd socket 描述符
     * @param data 数据指针
     * @param size 数据大小
     * @param handler 完成处理器
     */
    void async_send(int socket_fd, const void* data, std::size_t size, 
                   std::function<void(const error_code&, std::size_t)>&& handler);
    
    /**
     * @brief 接收数据
     * @param socket_fd socket 描述符
     * @param data 数据缓冲区
     * @param max_size 最大接收字节数
     * @param ec 错误码
     * @return 已接收的字节数
     */
    std::size_t receive(int socket_fd, void* data, std::size_t max_size, error_code& ec);
    
    /**
     * @brief 异步接收数据
     * @param socket_fd socket 描述符
     * @param data 数据缓冲区
     * @param max_size 最大接收字节数
     * @param handler 完成处理器
     */
    void async_receive(int socket_fd, void* data, std::size_t max_size, 
                      std::function<void(const error_code&, std::size_t)>&& handler);
    
    /**
     * @brief 发送数据到指定端点
     * @param socket_fd socket 描述符
     * @param data 数据指针
     * @param size 数据大小
     * @param endpoint 目标端点
     * @param ec 错误码
     * @return 发送的字节数
     */
    std::size_t send_to(int socket_fd, const void* data, std::size_t size,
                       const endpoint& endpoint, error_code& ec);
    
    /**
     * @brief 异步发送数据到指定端点
     * @param socket_fd socket 描述符
     * @param data 数据指针
     * @param size 数据大小
     * @param endpoint 目标端点
     * @param handler 完成处理器
     */
    void async_send_to(int socket_fd, const void* data, std::size_t size,
                      const endpoint& endpoint,
                      std::function<void(const error_code&, std::size_t)>&& handler);
    
    /**
     * @brief 从指定端点接收数据
     * @param socket_fd socket 描述符
     * @param data 数据指针
     * @param max_size 最大接收大小
     * @param endpoint 源端点（输出）
     * @param ec 错误码
     * @return 接收的字节数
     */
    std::size_t receive_from(int socket_fd, void* data, std::size_t max_size,
                           endpoint& endpoint, error_code& ec);
    
    /**
     * @brief 异步从指定端点接收数据
     * @param socket_fd socket 描述符
     * @param data 数据指针
     * @param max_size 最大接收大小
     * @param endpoint 源端点（输出）
     * @param handler 完成处理器
     */
    void async_receive_from(int socket_fd, void* data, std::size_t max_size,
                          endpoint& endpoint,
                          std::function<void(const error_code&, std::size_t)>&& handler);
    
    /**
     * @brief 获取本地端点
     * @param socket_fd socket 描述符
     * @param ec 错误码
     * @return 本地端点
     */
    endpoint local_endpoint(int socket_fd, error_code& ec) const;
    
    /**
     * @brief 获取远程端点
     * @param socket_fd socket 描述符
     * @param ec 错误码
     * @return 远程端点
     */
    endpoint remote_endpoint(int socket_fd, error_code& ec) const;
    
    /**
     * @brief 设置套接字选项
     * @param socket_fd socket 描述符
     * @param level 选项级别
     * @param option_name 选项名称
     * @param option_value 选项值指针
     * @param option_size 选项值大小
     * @param ec 错误代码
     * @return 成功返回 true，失败返回 false
     */
    bool setsockopt(int socket_fd, int level, int option_name,
                   const void* option_value, std::size_t option_size,
                   error_code& ec);
    
    /**
     * @brief 设置非阻塞模式
     * @param socket_fd socket 描述符
     * @param ec 错误码
     * @return 成功返回 true，失败返回 false
     */
    bool set_non_blocking(int socket_fd, error_code& ec);
    
private:
    // IO上下文
    io_context& io_context_;
    
    // 处理异步操作
    void start_async_operation(int socket_fd, int events, 
                             std::function<void(const error_code&)>&& handler);
    
    // 检查 socket 是否有效
    bool is_socket_valid(int socket_fd) const;
    
    // 互斥锁
    mutable std::mutex mutex_;
    
    // 操作队列
    struct operation {
        int socket_fd;
        int events;
        std::function<void(const error_code&)> handler;
    };
    std::vector<operation> operations_;
};

// 注意：tcp_acceptor::acceptor_service 已经在 socket.h 中定义
// 这里不再重复定义

} // namespace uasio

#endif // UASIO_SOCKET_SERVICE_H 