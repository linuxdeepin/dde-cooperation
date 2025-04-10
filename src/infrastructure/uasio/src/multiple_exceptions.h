#ifndef UASIO_MULTIPLE_EXCEPTIONS_H
#define UASIO_MULTIPLE_EXCEPTIONS_H

#include <exception>
#include <memory>
#include <system_error>
#include <vector>
#include <sstream>

namespace uasio {

/**
 * @brief 异常捕获类，用于捕获异步调用链中的多个异常
 */
class multiple_exceptions : public std::exception {
public:
    /// 默认构造函数
    multiple_exceptions() noexcept {}

    /// 析构函数
    virtual ~multiple_exceptions() noexcept {}

    /// 返回异常描述信息
    virtual const char* what() const noexcept override {
        if (!what_message_.empty()) {
            return what_message_.c_str();
        }

        try {
            std::ostringstream oss;
            oss << "multiple exceptions: " << exceptions_.size() << " captured";
            what_message_ = oss.str();
            return what_message_.c_str();
        } catch (...) {
            return "multiple exceptions";
        }
    }

    /// 获取捕获的异常数量
    std::size_t size() const noexcept {
        return exceptions_.size();
    }

    /// 添加异常
    void add_exception(std::exception_ptr ex) {
        if (ex) {
            exceptions_.emplace_back(std::move(ex));
        }
    }

    /// 获取指定索引的异常
    std::exception_ptr at(std::size_t index) const {
        if (index >= exceptions_.size()) {
            throw std::out_of_range("index out of range in multiple_exceptions::at");
        }
        return exceptions_[index];
    }

    /// 获取所有捕获的异常
    const std::vector<std::exception_ptr>& exceptions() const noexcept {
        return exceptions_;
    }

    /// 抛出第一个异常
    void throw_first_exception() const {
        if (!exceptions_.empty()) {
            std::rethrow_exception(exceptions_.front());
        }
    }

private:
    std::vector<std::exception_ptr> exceptions_;
    mutable std::string what_message_;
};

/**
 * @brief 尝试捕获异常并封装到multiple_exceptions中
 * @param func 可能抛出异常的函数
 * @param ex 已有的multiple_exceptions实例，可以为空
 */
template <typename Function>
void capture_exception(Function&& func, multiple_exceptions& ex) {
    try {
        func();
    } catch (...) {
        ex.add_exception(std::current_exception());
    }
}

/**
 * @brief 抛出多异常对象（如果至少有一个异常）
 * @param ex multiple_exceptions实例
 */
inline void throw_if_exceptions(const multiple_exceptions& ex) {
    if (ex.size() > 0) {
        throw ex;
    }
}

} // namespace uasio

#endif // UASIO_MULTIPLE_EXCEPTIONS_H 