// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_SSL_STREAM_H
#define UASIO_SSL_STREAM_H

#include "../error.h"
#include "../socket.h"
#include "../io_context.h"
#include "../buffer.h"
#include "context.h"
#include "error.h"
#include <string>
#include <memory>
#include <functional>

namespace uasio {
namespace ssl {

/**
 * @brief 握手类型枚举
 */
enum class handshake_type {
    client,  // 客户端握手
    server   // 服务器握手
};

// 前向声明
class stream_impl;

/**
 * @brief SSL流类
 * 
 * 提供SSL/TLS加密通信功能的流类，可以包装普通Socket以提供安全传输。
 * 
 * @tparam Stream 底层流类型，通常是Socket
 */
template <typename Stream>
class stream {
public:
    /// 底层流类型
    using next_layer_type = Stream;
    
    /**
     * @brief 构造函数
     * @param io_context IO上下文
     * @param ctx SSL上下文
     */
    explicit stream(io_context& io_context, context& ctx);
    
    /**
     * @brief 析构函数
     */
    ~stream();
    
    /**
     * @brief 获取底层流对象
     * @return 底层流对象的引用
     */
    next_layer_type& next_layer();
    
    /**
     * @brief 获取底层流对象（const版本）
     * @return 底层流对象的常量引用
     */
    const next_layer_type& next_layer() const;
    
    /**
     * @brief 执行SSL握手
     * @param type 握手类型（客户端或服务器）
     * @param ec 错误代码
     */
    void handshake(handshake_type type, error_code& ec);
    
    /**
     * @brief 执行异步SSL握手
     * @param type 握手类型（客户端或服务器）
     * @param handler 握手完成回调函数，格式为void(error_code)
     */
    template <typename HandshakeHandler>
    void async_handshake(handshake_type type, HandshakeHandler&& handler);
    
    /**
     * @brief 优雅地关闭SSL连接
     * @param ec 错误代码
     */
    void shutdown(error_code& ec);
    
    /**
     * @brief 异步优雅地关闭SSL连接
     * @param handler 关闭完成回调函数，格式为void(error_code)
     */
    template <typename ShutdownHandler>
    void async_shutdown(ShutdownHandler&& handler);
    
    /**
     * @brief 发送数据
     * @param data 要发送的数据指针
     * @param size 数据大小
     * @param ec 错误代码
     * @return 实际发送的字节数
     */
    std::size_t write_some(const void* data, std::size_t size, error_code& ec);
    
    /**
     * @brief 发送缓冲区数据
     * @param buffer 要发送的缓冲区
     * @param ec 错误代码
     * @return 实际发送的字节数
     */
    std::size_t write_some(const const_buffer& buffer, error_code& ec);
    
    /**
     * @brief 异步发送数据
     * @param data 要发送的数据指针
     * @param size 数据大小
     * @param handler 发送完成回调函数，格式为void(error_code, std::size_t)
     */
    template <typename WriteHandler>
    void async_write_some(const void* data, std::size_t size, WriteHandler&& handler);
    
    /**
     * @brief 异步发送缓冲区数据
     * @param buffer 要发送的缓冲区
     * @param handler 发送完成回调函数，格式为void(error_code, std::size_t)
     */
    template <typename WriteHandler>
    void async_write_some(const const_buffer& buffer, WriteHandler&& handler);
    
    /**
     * @brief 接收数据
     * @param data 接收数据的缓冲区指针
     * @param max_size 缓冲区最大大小
     * @param ec 错误代码
     * @return 实际接收的字节数
     */
    std::size_t read_some(void* data, std::size_t max_size, error_code& ec);
    
    /**
     * @brief 接收数据到缓冲区
     * @param buffer 接收数据的缓冲区
     * @param ec 错误代码
     * @return 实际接收的字节数
     */
    std::size_t read_some(const mutable_buffer& buffer, error_code& ec);
    
    /**
     * @brief 异步接收数据
     * @param data 接收数据的缓冲区指针
     * @param max_size 缓冲区最大大小
     * @param handler 接收完成回调函数，格式为void(error_code, std::size_t)
     */
    template <typename ReadHandler>
    void async_read_some(void* data, std::size_t max_size, ReadHandler&& handler);
    
    /**
     * @brief 异步接收数据到缓冲区
     * @param buffer 接收数据的缓冲区
     * @param handler 接收完成回调函数，格式为void(error_code, std::size_t)
     */
    template <typename ReadHandler>
    void async_read_some(const mutable_buffer& buffer, ReadHandler&& handler);
    
    /**
     * @brief 获取SSL会话信息
     * @return SSL会话信息字符串
     */
    std::string get_session_info() const;
    
    /**
     * @brief 获取对端证书信息
     * @return 对端证书信息字符串
     */
    std::string get_peer_certificate_info() const;
    
private:
    // 底层流对象
    next_layer_type next_layer_;
    
    // SSL实现
    std::shared_ptr<stream_impl> impl_;
};

// 模板类外的标准实现

/**
 * @brief 完整发送操作，直到所有数据都被发送
 * @param s SSL流对象
 * @param buffer 要发送的缓冲区
 * @param ec 错误代码
 * @return 实际发送的字节数
 */
template <typename Stream>
std::size_t write(stream<Stream>& s, const const_buffer& buffer, error_code& ec);

/**
 * @brief 异步完整发送操作，直到所有数据都被发送
 * @param s SSL流对象
 * @param buffer 要发送的缓冲区
 * @param handler 发送完成回调函数，格式为void(error_code, std::size_t)
 */
template <typename Stream, typename WriteHandler>
void async_write(stream<Stream>& s, const const_buffer& buffer, WriteHandler&& handler);

/**
 * @brief 完整接收操作，直到接收指定大小的数据
 * @param s SSL流对象
 * @param buffer 接收数据的缓冲区
 * @param ec 错误代码
 * @return 实际接收的字节数
 */
template <typename Stream>
std::size_t read(stream<Stream>& s, const mutable_buffer& buffer, error_code& ec);

/**
 * @brief 异步完整接收操作，直到接收指定大小的数据
 * @param s SSL流对象
 * @param buffer 接收数据的缓冲区
 * @param handler 接收完成回调函数，格式为void(error_code, std::size_t)
 */
template <typename Stream, typename ReadHandler>
void async_read(stream<Stream>& s, const mutable_buffer& buffer, ReadHandler&& handler);

} // namespace ssl
} // namespace uasio

// 模板实现部分
#include "stream_impl.h"

#endif // UASIO_SSL_STREAM_H 