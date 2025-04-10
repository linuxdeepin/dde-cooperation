// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_SERIAL_PORT_H
#define UASIO_SERIAL_PORT_H

#include "io_context.h"
#include "error.h"
#include "buffer.h"
#include <string>
#include <memory>
#include <functional>

namespace asio {

/**
 * @brief 串行端口波特率枚举
 */
enum class baud_rate {
    baud_1200 = 1200,
    baud_2400 = 2400,
    baud_4800 = 4800,
    baud_9600 = 9600,
    baud_19200 = 19200,
    baud_38400 = 38400,
    baud_57600 = 57600,
    baud_115200 = 115200
};

/**
 * @brief 串行端口数据位枚举
 */
enum class character_size {
    size_5 = 5,
    size_6 = 6,
    size_7 = 7,
    size_8 = 8
};

/**
 * @brief 串行端口停止位枚举
 */
enum class stop_bits {
    one,
    onepointfive,
    two
};

/**
 * @brief 串行端口奇偶校验枚举
 */
enum class parity {
    none,
    odd,
    even
};

/**
 * @brief 串行端口流控制枚举
 */
enum class flow_control {
    none,
    software,
    hardware
};

/**
 * @brief 串行端口类
 */
class serial_port {
public:
    /**
     * @brief 构造函数
     * @param io_context IO上下文
     */
    explicit serial_port(io_context& io_context);

    /**
     * @brief 构造函数，打开指定的串行设备
     * @param io_context IO上下文
     * @param device 串行设备路径
     * @param ec 错误码
     */
    serial_port(io_context& io_context, const std::string& device, error_code& ec);

    /**
     * @brief 析构函数
     */
    ~serial_port();

    /**
     * @brief 打开串行设备
     * @param device 串行设备路径
     * @param ec 错误码
     * @return 操作是否成功
     */
    bool open(const std::string& device, error_code& ec);

    /**
     * @brief 检查串行端口是否打开
     * @return 是否打开
     */
    bool is_open() const;

    /**
     * @brief 关闭串行端口
     * @param ec 错误码
     */
    void close(error_code& ec);

    /**
     * @brief 获取设备名称
     * @return 设备名称
     */
    std::string device() const;

    /**
     * @brief 发送数据
     * @param data 数据指针
     * @param size 数据大小
     * @param ec 错误码
     * @return 发送的字节数
     */
    std::size_t send(const void* data, std::size_t size, error_code& ec);

    /**
     * @brief 发送数据
     * @param buffer 数据缓冲区
     * @param ec 错误码
     * @return 发送的字节数
     */
    std::size_t send(const const_buffer& buffer, error_code& ec);

    /**
     * @brief 异步发送数据
     * @param data 数据指针
     * @param size 数据大小
     * @param handler 完成处理程序
     */
    template <typename WriteHandler>
    void async_send(const void* data, std::size_t size, WriteHandler&& handler);

    /**
     * @brief 异步发送数据
     * @param buffer 数据缓冲区
     * @param handler 完成处理程序
     */
    template <typename WriteHandler>
    void async_send(const const_buffer& buffer, WriteHandler&& handler);

    /**
     * @brief 接收数据
     * @param data 数据指针
     * @param max_size 最大接收字节数
     * @param ec 错误码
     * @return 接收的字节数
     */
    std::size_t receive(void* data, std::size_t max_size, error_code& ec);

    /**
     * @brief 接收数据
     * @param buffer 数据缓冲区
     * @param ec 错误码
     * @return 接收的字节数
     */
    std::size_t receive(const mutable_buffer& buffer, error_code& ec);

    /**
     * @brief 异步接收数据
     * @param data 数据指针
     * @param max_size 最大接收字节数
     * @param handler 完成处理程序
     */
    template <typename ReadHandler>
    void async_receive(void* data, std::size_t max_size, ReadHandler&& handler);

    /**
     * @brief 异步接收数据
     * @param buffer 数据缓冲区
     * @param handler 完成处理程序
     */
    template <typename ReadHandler>
    void async_receive(const mutable_buffer& buffer, ReadHandler&& handler);

    /**
     * @brief 设置波特率
     * @param rate 波特率
     * @param ec 错误码
     * @return 操作是否成功
     */
    bool set_baud_rate(baud_rate rate, error_code& ec);

    /**
     * @brief 设置波特率
     * @param rate 波特率（整数值）
     * @param ec 错误码
     * @return 操作是否成功
     */
    bool set_baud_rate(unsigned int rate, error_code& ec);

    /**
     * @brief 获取波特率
     * @param ec 错误码
     * @return 波特率
     */
    unsigned int baud_rate(error_code& ec) const;

    /**
     * @brief 设置数据位
     * @param size 数据位
     * @param ec 错误码
     * @return 操作是否成功
     */
    bool set_character_size(character_size size, error_code& ec);

    /**
     * @brief 获取数据位
     * @param ec 错误码
     * @return 数据位
     */
    character_size character_size(error_code& ec) const;

    /**
     * @brief 设置停止位
     * @param bits 停止位
     * @param ec 错误码
     * @return 操作是否成功
     */
    bool set_stop_bits(stop_bits bits, error_code& ec);

    /**
     * @brief 获取停止位
     * @param ec 错误码
     * @return 停止位
     */
    stop_bits stop_bits(error_code& ec) const;

    /**
     * @brief 设置奇偶校验
     * @param p 奇偶校验
     * @param ec 错误码
     * @return 操作是否成功
     */
    bool set_parity(parity p, error_code& ec);

    /**
     * @brief 获取奇偶校验
     * @param ec 错误码
     * @return 奇偶校验
     */
    parity parity(error_code& ec) const;

    /**
     * @brief 设置流控制
     * @param fc 流控制
     * @param ec 错误码
     * @return 操作是否成功
     */
    bool set_flow_control(flow_control fc, error_code& ec);

    /**
     * @brief 获取流控制
     * @param ec 错误码
     * @return 流控制
     */
    flow_control flow_control(error_code& ec) const;

private:
    class serial_port_service;
    std::shared_ptr<serial_port_service> service_;
    std::string device_;
};

// 实现部分

template <typename WriteHandler>
inline void serial_port::async_send(const void* data, std::size_t size, WriteHandler&& handler) {
    if (!service_) {
        io_context& ioc = service_->get_io_context();
        ioc.post([handler = std::forward<WriteHandler>(handler)]() {
            handler(make_error_code(error::not_connected), 0);
        });
        return;
    }
    
    service_->async_send(data, size, std::forward<WriteHandler>(handler));
}

template <typename WriteHandler>
inline void serial_port::async_send(const const_buffer& buffer, WriteHandler&& handler) {
    async_send(buffer.data(), buffer.size(), std::forward<WriteHandler>(handler));
}

template <typename ReadHandler>
inline void serial_port::async_receive(void* data, std::size_t max_size, ReadHandler&& handler) {
    if (!service_) {
        io_context& ioc = service_->get_io_context();
        ioc.post([handler = std::forward<ReadHandler>(handler)]() {
            handler(make_error_code(error::not_connected), 0);
        });
        return;
    }
    
    service_->async_receive(data, max_size, std::forward<ReadHandler>(handler));
}

template <typename ReadHandler>
inline void serial_port::async_receive(const mutable_buffer& buffer, ReadHandler&& handler) {
    async_receive(buffer.data(), buffer.size(), std::forward<ReadHandler>(handler));
}

} // namespace asio

#endif // UASIO_SERIAL_PORT_H 