// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef STREAMBUF_H
#define STREAMBUF_H

#include "buffer.h"
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <cstring>

namespace uasio {

/**
 * @brief 流缓冲区类，提供动态大小的缓冲区用于数据流处理
 */
class streambuf {
public:
    /**
     * @brief 默认构造函数
     * @param max_size 最大缓冲区大小
     */
    explicit streambuf(std::size_t max_size = 1024 * 1024)
        : max_size_(max_size)
    {
        // 初始分配一小块内存
        storage_.resize(1024);
        // 设置读写指针
        read_pos_ = 0;
        write_pos_ = 0;
    }
    
    /**
     * @brief 获取可读取数据的缓冲区
     * @return 可读取数据的常量缓冲区
     */
    uasio::const_buffer data() const noexcept
    {
        if (read_pos_ == write_pos_) {
            return uasio::const_buffer();
        }
        return uasio::const_buffer(&storage_[read_pos_], write_pos_ - read_pos_);
    }
    
    /**
     * @brief 获取可写入数据的缓冲区
     * @return 可写入数据的可变缓冲区
     */
    uasio::mutable_buffer prepare(std::size_t size)
    {
        if (size > max_size_) {
            throw std::length_error("streambuf too long");
        }
        
        // 计算剩余容量
        std::size_t capacity = storage_.size() - write_pos_;
        
        // 如果剩余容量不足，先尝试将数据向前移动
        if (capacity < size && read_pos_ > 0) {
            std::memmove(&storage_[0], &storage_[read_pos_], write_pos_ - read_pos_);
            write_pos_ -= read_pos_;
            read_pos_ = 0;
            capacity = storage_.size() - write_pos_;
        }
        
        // 如果仍然不足，扩展存储空间
        if (capacity < size) {
            std::size_t new_size = storage_.size() + size - capacity;
            // 确保不超过最大大小
            if (new_size > max_size_) {
                new_size = max_size_;
                if (storage_.size() == new_size) {
                    throw std::length_error("streambuf too long");
                }
            }
            storage_.resize(new_size);
        }
        
        return uasio::mutable_buffer(&storage_[write_pos_], size);
    }
    
    /**
     * @brief 确认数据已写入缓冲区
     * @param size 已写入的数据大小
     */
    void commit(std::size_t size) noexcept
    {
        std::size_t available = storage_.size() - write_pos_;
        write_pos_ += (std::min)(size, available);
    }
    
    /**
     * @brief 消费已读取的数据
     * @param size 已读取的数据大小
     */
    void consume(std::size_t size) noexcept
    {
        std::size_t available = write_pos_ - read_pos_;
        read_pos_ += (std::min)(size, available);
        
        // 如果所有数据都已消费，重置读写指针
        if (read_pos_ == write_pos_) {
            read_pos_ = write_pos_ = 0;
        }
    }
    
    /**
     * @brief 获取可读取数据的大小
     * @return 可读取数据的大小
     */
    std::size_t size() const noexcept
    {
        return write_pos_ - read_pos_;
    }
    
    /**
     * @brief 获取最大缓冲区大小
     * @return 最大缓冲区大小
     */
    std::size_t max_size() const noexcept
    {
        return max_size_;
    }
    
    /**
     * @brief 获取当前分配的缓冲区大小
     * @return 当前分配的缓冲区大小
     */
    std::size_t capacity() const noexcept
    {
        return storage_.size();
    }
    
    /**
     * @brief 检查缓冲区是否为空
     * @return 如果缓冲区为空，返回 true
     */
    bool empty() const noexcept
    {
        return read_pos_ == write_pos_;
    }
    
    /**
     * @brief 将缓冲区的内容转换为字符串
     * @return 包含缓冲区内容的字符串
     */
    std::string str() const
    {
        return std::string(
            reinterpret_cast<const char*>(&storage_[read_pos_]), 
            write_pos_ - read_pos_
        );
    }
    
    /**
     * @brief 清空缓冲区
     */
    void clear()
    {
        read_pos_ = write_pos_ = 0;
    }
    
private:
    // 存储数据的向量
    std::vector<char> storage_;
    // 读指针位置
    std::size_t read_pos_;
    // 写指针位置
    std::size_t write_pos_;
    // 最大缓冲区大小
    std::size_t max_size_;
};

} // namespace uasio

#endif // STREAMBUF_H 