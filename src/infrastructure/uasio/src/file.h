// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_FILE_H
#define UASIO_FILE_H

#include "io_context.h"
#include "error.h"
#include "buffer.h"
#include <string>
#include <memory>
#include <functional>

namespace uasio {

/**
 * @brief 文件打开模式枚举
 */
enum class file_mode {
    read_only,       // 只读模式
    write_only,      // 只写模式
    read_write,      // 读写模式
    append,          // 追加模式
    truncate         // 截断模式（如果文件已存在，则清空内容）
};

/**
 * @brief 文件定位方式枚举
 */
enum class seek_basis {
    start,           // 文件开始
    current,         // 当前位置
    end              // 文件结尾
};

/**
 * @brief 异步文件类
 */
class file {
public:
    /**
     * @brief 构造函数
     * @param io_context IO上下文
     */
    explicit file(io_context& io_context);

    /**
     * @brief 构造函数，打开指定文件
     * @param io_context IO上下文
     * @param path 文件路径
     * @param mode 打开模式
     * @param ec 错误码
     */
    file(io_context& io_context, const std::string& path, 
         file_mode mode, error_code& ec);

    /**
     * @brief 析构函数
     */
    ~file();

    /**
     * @brief 打开文件
     * @param path 文件路径
     * @param mode 打开模式
     * @param ec 错误码
     * @return 操作是否成功
     */
    bool open(const std::string& path, file_mode mode, error_code& ec);

    /**
     * @brief 检查文件是否打开
     * @return 是否打开
     */
    bool is_open() const;

    /**
     * @brief 关闭文件
     * @param ec 错误码
     */
    void close(error_code& ec);

    /**
     * @brief 获取文件路径
     * @return 文件路径
     */
    std::string path() const;

    /**
     * @brief 获取文件大小
     * @param ec 错误码
     * @return 文件大小
     */
    std::size_t size(error_code& ec) const;

    /**
     * @brief 读取数据
     * @param data 数据指针
     * @param size 要读取的字节数
     * @param ec 错误码
     * @return 读取的字节数
     */
    std::size_t read(void* data, std::size_t size, error_code& ec);

    /**
     * @brief 读取数据
     * @param buffer 数据缓冲区
     * @param ec 错误码
     * @return 读取的字节数
     */
    std::size_t read(const mutable_buffer& buffer, error_code& ec);

    /**
     * @brief 异步读取数据
     * @param data 数据指针
     * @param size 要读取的字节数
     * @param handler 完成处理程序
     */
    template <typename ReadHandler>
    void async_read(void* data, std::size_t size, ReadHandler&& handler);

    /**
     * @brief 异步读取数据
     * @param buffer 数据缓冲区
     * @param handler 完成处理程序
     */
    template <typename ReadHandler>
    void async_read(const mutable_buffer& buffer, ReadHandler&& handler);

    /**
     * @brief 写入数据
     * @param data 数据指针
     * @param size 要写入的字节数
     * @param ec 错误码
     * @return 写入的字节数
     */
    std::size_t write(const void* data, std::size_t size, error_code& ec);

    /**
     * @brief 写入数据
     * @param buffer 数据缓冲区
     * @param ec 错误码
     * @return 写入的字节数
     */
    std::size_t write(const const_buffer& buffer, error_code& ec);

    /**
     * @brief 异步写入数据
     * @param data 数据指针
     * @param size 要写入的字节数
     * @param handler 完成处理程序
     */
    template <typename WriteHandler>
    void async_write(const void* data, std::size_t size, WriteHandler&& handler);

    /**
     * @brief 异步写入数据
     * @param buffer 数据缓冲区
     * @param handler 完成处理程序
     */
    template <typename WriteHandler>
    void async_write(const const_buffer& buffer, WriteHandler&& handler);

    /**
     * @brief 设置文件读写位置
     * @param offset 偏移量
     * @param basis 定位基准
     * @param ec 错误码
     * @return 新的文件位置
     */
    std::size_t seek(std::int64_t offset, seek_basis basis, error_code& ec);

    /**
     * @brief 获取当前文件读写位置
     * @param ec 错误码
     * @return 当前位置
     */
    std::size_t tell(error_code& ec) const;

    /**
     * @brief 将缓冲区内容刷新到磁盘
     * @param ec 错误码
     * @return 操作是否成功
     */
    bool flush(error_code& ec);

    /**
     * @brief 异步刷新缓冲区
     * @param handler 完成处理程序
     */
    template <typename FlushHandler>
    void async_flush(FlushHandler&& handler);

private:
    class file_service;
    std::shared_ptr<file_service> service_;
    std::string path_;
};

// 实现部分

template <typename ReadHandler>
inline void file::async_read(void* data, std::size_t size, ReadHandler&& handler) {
    if (!service_ || !is_open()) {
        io_context& ioc = service_->get_io_context();
        ioc.post([handler = std::forward<ReadHandler>(handler)]() {
            handler(make_error_code(error::not_connected), 0);
        });
        return;
    }
    
    service_->async_read(data, size, std::forward<ReadHandler>(handler));
}

template <typename ReadHandler>
inline void file::async_read(const mutable_buffer& buffer, ReadHandler&& handler) {
    async_read(buffer.data(), buffer.size(), std::forward<ReadHandler>(handler));
}

template <typename WriteHandler>
inline void file::async_write(const void* data, std::size_t size, WriteHandler&& handler) {
    if (!service_ || !is_open()) {
        io_context& ioc = service_->get_io_context();
        ioc.post([handler = std::forward<WriteHandler>(handler)]() {
            handler(make_error_code(error::not_connected), 0);
        });
        return;
    }
    
    service_->async_write(data, size, std::forward<WriteHandler>(handler));
}

template <typename WriteHandler>
inline void file::async_write(const const_buffer& buffer, WriteHandler&& handler) {
    async_write(buffer.data(), buffer.size(), std::forward<WriteHandler>(handler));
}

template <typename FlushHandler>
inline void file::async_flush(FlushHandler&& handler) {
    if (!service_ || !is_open()) {
        io_context& ioc = service_->get_io_context();
        ioc.post([handler = std::forward<FlushHandler>(handler)]() {
            handler(make_error_code(error::not_connected));
        });
        return;
    }
    
    service_->async_flush(std::forward<FlushHandler>(handler));
}

} // namespace uasio

#endif // UASIO_FILE_H 