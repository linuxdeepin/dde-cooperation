// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BUFFER_H
#define BUFFER_H

#include <cstddef>
#include <vector>
#include <array>
#include <string>
#include <cstring>
#include <memory>

namespace uasio {

/**
 * @brief 表示一个内存区域的基本接口
 */
class const_buffer {
public:
    /**
     * @brief 默认构造函数
     */
    const_buffer() noexcept
        : data_(nullptr), size_(0) {}
    
    /**
     * @brief 构造函数
     * @param data 指向数据的指针
     * @param size 数据大小
     */
    const_buffer(const void* data, std::size_t size) noexcept
        : data_(data), size_(size) {}
    
    /**
     * @brief 获取数据指针
     * @return 数据指针
     */
    const void* data() const noexcept { return data_; }
    
    /**
     * @brief 获取数据大小
     * @return 数据大小
     */
    std::size_t size() const noexcept { return size_; }
    
    /**
     * @brief 重置缓冲区
     */
    void reset() noexcept { data_ = nullptr; size_ = 0; }
    
    /**
     * @brief 偏移缓冲区
     * @param n 偏移量
     * @return 偏移后的缓冲区
     */
    const_buffer operator+(std::size_t n) const noexcept {
        if (n >= size_) {
            return const_buffer();
        }
        const char* data_ptr = static_cast<const char*>(data_);
        return const_buffer(data_ptr + n, size_ - n);
    }
    
private:
    const void* data_;
    std::size_t size_;
};

/**
 * @brief 表示一个可修改的内存区域的基本接口
 */
class mutable_buffer {
public:
    /**
     * @brief 默认构造函数
     */
    mutable_buffer() noexcept
        : data_(nullptr), size_(0) {}
    
    /**
     * @brief 构造函数
     * @param data 指向数据的指针
     * @param size 数据大小
     */
    mutable_buffer(void* data, std::size_t size) noexcept
        : data_(data), size_(size) {}
    
    /**
     * @brief 获取数据指针
     * @return 数据指针
     */
    void* data() const noexcept { return data_; }
    
    /**
     * @brief 获取数据大小
     * @return 数据大小
     */
    std::size_t size() const noexcept { return size_; }
    
    /**
     * @brief 重置缓冲区
     */
    void reset() noexcept { data_ = nullptr; size_ = 0; }
    
    /**
     * @brief 偏移缓冲区
     * @param n 偏移量
     * @return 偏移后的缓冲区
     */
    mutable_buffer operator+(std::size_t n) const noexcept {
        if (n >= size_) {
            return mutable_buffer();
        }
        char* data_ptr = static_cast<char*>(data_);
        return mutable_buffer(data_ptr + n, size_ - n);
    }
    
    /**
     * @brief 隐式转换为 const_buffer
     */
    operator const_buffer() const noexcept {
        return const_buffer(data_, size_);
    }
    
private:
    void* data_;
    std::size_t size_;
};

/**
 * @brief 创建一个 const_buffer 对象
 * @param data 指向数据的指针
 * @param size 数据大小
 * @return const_buffer 对象
 */
inline const_buffer buffer(const void* data, std::size_t size) noexcept {
    return const_buffer(data, size);
}

/**
 * @brief 创建一个 mutable_buffer 对象
 * @param data 指向数据的指针
 * @param size 数据大小
 * @return mutable_buffer 对象
 */
inline mutable_buffer buffer(void* data, std::size_t size) noexcept {
    return mutable_buffer(data, size);
}

/**
 * @brief 从 std::string 创建一个 const_buffer 对象
 * @param str 字符串
 * @return const_buffer 对象
 */
inline const_buffer buffer(const std::string& str) noexcept {
    return const_buffer(str.data(), str.size());
}

/**
 * @brief 从数组创建一个 const_buffer 对象
 * @tparam T 数组元素类型
 * @tparam N 数组大小
 * @param arr 数组
 * @return const_buffer 对象
 */
template <typename T, std::size_t N>
inline const_buffer buffer(const std::array<T, N>& arr) noexcept {
    return const_buffer(arr.data(), arr.size() * sizeof(T));
}

/**
 * @brief 从数组创建一个 mutable_buffer 对象
 * @tparam T 数组元素类型
 * @tparam N 数组大小
 * @param arr 数组
 * @return mutable_buffer 对象
 */
template <typename T, std::size_t N>
inline mutable_buffer buffer(std::array<T, N>& arr) noexcept {
    return mutable_buffer(arr.data(), arr.size() * sizeof(T));
}

/**
 * @brief 从 std::vector 创建一个 const_buffer 对象
 * @tparam T 向量元素类型
 * @param vec 向量
 * @return const_buffer 对象
 */
template <typename T>
inline const_buffer buffer(const std::vector<T>& vec) noexcept {
    return const_buffer(vec.data(), vec.size() * sizeof(T));
}

/**
 * @brief 从 std::vector 创建一个 mutable_buffer 对象
 * @tparam T 向量元素类型
 * @param vec 向量
 * @return mutable_buffer 对象
 */
template <typename T>
inline mutable_buffer buffer(std::vector<T>& vec) noexcept {
    return mutable_buffer(vec.data(), vec.size() * sizeof(T));
}

} // namespace uasio

#endif // BUFFER_H 