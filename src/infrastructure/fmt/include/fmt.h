#pragma once
#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <type_traits>
#include <iostream>

namespace fmt {

// 基础类型定义
class FormatError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct FormatSpec {
    char type = 0;
    int width = 0;
    int precision = -1;
    char fill = ' ';
    bool align_left = false;
    bool numeric = false;
    bool zero_padding = false;
};

class memory_buffer {
private:
    std::vector<char> buffer_;
    
public:
    void append(const char* str, size_t size) {
        if (str && size > 0) {
            buffer_.reserve(buffer_.size() + size);
            buffer_.insert(buffer_.end(), str, str + size);
        }
    }
    
    void append(size_t count, char ch) {
        buffer_.insert(buffer_.end(), count, ch);
    }
    
    void push_back(char c) { buffer_.push_back(c); }
    const char* data() const { return buffer_.data(); }
    size_t size() const { return buffer_.size(); }
    void clear() { buffer_.clear(); }
};

class value_type {
public:
    virtual ~value_type() = default;
    virtual std::string to_string() const = 0;
};

// 主模板定义
template<typename T, typename = void>
class format_arg : public value_type {
public:
    static const bool is_string_type = std::is_same_v<T, std::string>;
    
    const T& value_;
public:
    explicit format_arg(const T& value) : value_(value) {}
    
    std::string to_string() const override {
        if constexpr(std::is_same_v<T, std::string>) {
            return value_;
        } else if constexpr(std::is_arithmetic_v<T>) {
            return std::to_string(value_);
        } else {
            std::ostringstream ss;
            ss << value_;
            return ss.str();
        }
    }
    
    bool empty() const { 
        if constexpr(std::is_same_v<T, std::string>) {
            return value_.empty();
        }
        return false; 
    }
};

// const char*显式特化
template<>
class format_arg<const char*> : public value_type {
    const char* value_;
public:
    explicit format_arg(const char* value) : value_(value) {}
    
    std::string to_string() const override {
        return value_ ? std::string(value_) : std::string();
    }
    
    bool empty() const { 
        return !value_ || !*value_;
    }
};

// 其他类定义
class format_context {
    memory_buffer& buffer_;
    std::vector<std::unique_ptr<value_type>> args_;
    size_t next_arg_id_{0};
    
public:
    using iterator = const char*;
    
    explicit format_context(memory_buffer& buf) : buffer_(buf) {}
    memory_buffer& buffer() { return buffer_; }
    
    template<typename T>
    void push_arg(T&& arg) {
        args_.push_back(std::make_unique<format_arg<std::decay_t<T>>>(std::forward<T>(arg)));
    }
    
    const value_type* next_arg() { 
        return next_arg_id_ < args_.size() ? args_[next_arg_id_++].get() : nullptr;
    }
};

namespace detail {

class core_formatter {
public:
    template<typename Context>
    typename Context::iterator format(const char* fmt, Context& ctx);
    
    // 用于ostream输出的格式化
    void format_value(std::ostream& out, const std::string& value,
                     const FormatSpec& spec);

    // 用于memory_buffer输出的格式化
    void format_value(memory_buffer& buf, const std::string& value,
                     const FormatSpec& spec);

    FormatSpec parse_format_spec(const char* fmt);
};

} // namespace detail

// 添加可变参数模板的format函数实现
template<typename... Args>
std::string format(const std::string& fmt_str, const Args&... args) {
    memory_buffer buffer;
    format_context ctx(buffer);
    
    // 添加所有参数到context中
    (ctx.push_arg(args), ...);
    
    // 使用core_formatter进行格式化
    detail::core_formatter().format(fmt_str.c_str(), ctx);

    // 从buffer构造结果字符串并返回
    return std::string(buffer.data(), buffer.size());
}


} // namespace fmt

