// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_DATAGRAM_SOCKET_H
#define UASIO_DATAGRAM_SOCKET_H

#include "io_context.h"
#include "error.h"
#include "socket.h"
#include "buffer.h"
#include <functional>
#include <memory>

namespace uasio {

/**
 * @brief 数据报套接字类，用于UDP通信
 */
class datagram_socket : public socket {
public:
    /**
     * @brief 默认构造函数
     * 
     * 创建一个未打开的数据报套接字
     * @param io_context IO上下文对象
     */
    explicit datagram_socket(io_context& io_context);
    
    /**
     * @brief 构造函数
     * 
     * 创建并打开一个数据报套接字
     * @param io_context IO上下文对象
     * @param protocol 协议族
     * @param ec 错误码
     */
    datagram_socket(io_context& io_context, address_family family, error_code& ec);
    
    /**
     * @brief 打开套接字
     * @param family 地址族
     * @param ec 错误码
     * @return 操作是否成功
     */
    bool open(address_family family, error_code& ec);
    
    /**
     * @brief 绑定套接字到本地地址
     * @param endpoint 本地端点
     * @param ec 错误码
     */
    void bind(const endpoint& endpoint, error_code& ec);
    
    /**
     * @brief 发送数据到已连接的地址
     * @param data 发送的数据
     * @param size 数据大小
     * @param ec 错误码
     * @return 发送的字节数
     */
    std::size_t send(const void* data, std::size_t size, error_code& ec);
    
    /**
     * @brief 异步发送数据到已连接的地址
     * @param data 发送的数据
     * @param size 数据大小
     * @param handler 发送完成后的回调函数，格式为 void(const error_code&, std::size_t)
     */
    void async_send(const void* data, std::size_t size, 
                   std::function<void(const error_code&, std::size_t)>&& handler);
    
    /**
     * @brief 发送数据到指定地址
     * @param data 发送的数据
     * @param size 数据大小
     * @param endpoint 目标端点
     * @param ec 错误码
     * @return 发送的字节数
     */
    std::size_t send_to(const void* data, std::size_t size, const endpoint& endpoint, error_code& ec);
    
    /**
     * @brief 异步发送数据到指定地址
     * @param data 发送的数据
     * @param size 数据大小
     * @param endpoint 目标端点
     * @param handler 发送完成后的回调函数，格式为 void(const error_code&, std::size_t)
     */
    void async_send_to(const void* data, std::size_t size, const endpoint& endpoint,
                      std::function<void(const error_code&, std::size_t)>&& handler);
    
    /**
     * @brief 从已连接的地址接收数据
     * @param data 接收数据的缓冲区
     * @param size 缓冲区最大大小
     * @param ec 错误码
     * @return 接收的字节数
     */
    std::size_t receive(void* data, std::size_t size, error_code& ec);
    
    /**
     * @brief 异步从已连接的地址接收数据
     * @param data 接收数据的缓冲区
     * @param size 缓冲区最大大小
     * @param handler 接收完成后的回调函数，格式为 void(const error_code&, std::size_t)
     */
    void async_receive(void* data, std::size_t size, 
                      std::function<void(const error_code&, std::size_t)>&& handler);
    
    /**
     * @brief 接收数据并获取发送者地址
     * @param data 接收数据的缓冲区
     * @param size 缓冲区最大大小
     * @param endpoint 发送者端点
     * @param ec 错误码
     * @return 接收的字节数
     */
    std::size_t receive_from(void* data, std::size_t size, endpoint& endpoint, error_code& ec);
    
    /**
     * @brief 异步接收数据并获取发送者地址
     * @param data 接收数据的缓冲区
     * @param size 缓冲区最大大小
     * @param endpoint 发送者端点
     * @param handler 接收完成后的回调函数，格式为 void(const error_code&, std::size_t)
     */
    void async_receive_from(void* data, std::size_t size, endpoint& endpoint,
                           std::function<void(const error_code&, std::size_t)>&& handler);
    
    /**
     * @brief 获取本地端点
     * @param ec 错误码
     * @return 本地端点
     */
    endpoint local_endpoint(error_code& ec) const;
    
    /**
     * @brief 获取远程端点
     * @param ec 错误码
     * @return 远程端点
     */
    endpoint remote_endpoint(error_code& ec) const;
    
    /**
     * @brief 设置选项
     * @param level 选项级别
     * @param option_name 选项名称
     * @param option_value 选项值
     * @param option_size 选项大小
     * @param ec 错误码
     * @return 操作是否成功
     */
    bool set_option(int level, int option_name, const void* option_value, 
                  std::size_t option_size, error_code& ec);
    
    /**
     * @brief 设置广播选项
     * @param value 是否启用广播
     * @param ec 错误码
     * @return 操作是否成功
     */
    bool set_broadcast(bool value, error_code& ec);
    
    /**
     * @brief 设置接收缓冲区大小
     * @param size 缓冲区大小
     * @param ec 错误码
     * @return 操作是否成功
     */
    bool set_receive_buffer_size(int size, error_code& ec);
    
    /**
     * @brief 设置发送缓冲区大小
     * @param size 缓冲区大小
     * @param ec 错误码
     * @return 操作是否成功
     */
    bool set_send_buffer_size(int size, error_code& ec);
};

} // namespace uasio

#endif // UASIO_DATAGRAM_SOCKET_H 