#ifndef UASIO_PROMISE_H
#define UASIO_PROMISE_H

#include "future.h"
#include <exception>

namespace uasio {

/**
 * @brief Promise类，用于设置Future的结果值
 * @tparam T 结果类型
 */
template <typename T>
class promise {
public:
    /// 默认构造函数
    promise()
        : state_(std::make_shared<typename future<T>::shared_state>()),
          future_retrieved_(false) {}
    
    /// 移动构造函数
    promise(promise&& other) noexcept
        : state_(std::move(other.state_)),
          future_retrieved_(other.future_retrieved_) {
        other.future_retrieved_ = false;
    }
    
    /// 移动赋值运算符
    promise& operator=(promise&& other) noexcept {
        if (this != &other) {
            state_ = std::move(other.state_);
            future_retrieved_ = other.future_retrieved_;
            other.future_retrieved_ = false;
        }
        return *this;
    }
    
    /// 禁止复制
    promise(const promise&) = delete;
    promise& operator=(const promise&) = delete;
    
    /// 析构函数
    ~promise() {
        if (state_) {
            // 如果promise被销毁但没有设置值，设置一个异常
            try {
                std::unique_lock<std::mutex> lock(state_->mutex, std::try_to_lock);
                if (lock.owns_lock() && !state_->ready) {
                    state_->set_exception(std::make_exception_ptr(
                        std::future_error(std::future_errc::broken_promise)));
                }
            } catch (...) {
                // 忽略任何异常
            }
        }
    }
    
    /// 获取关联的future
    future<T> get_future() {
        if (!state_) {
            throw std::future_error(std::future_errc::no_state);
        }
        
        if (future_retrieved_) {
            throw std::future_error(std::future_errc::future_already_retrieved);
        }
        
        future_retrieved_ = true;
        return future<T>(state_);
    }
    
    /// 设置值（非void类型）
    template <typename U = T>
    typename std::enable_if<!std::is_void<U>::value>::type
    set_value(U&& value) {
        if (!state_) {
            throw std::future_error(std::future_errc::no_state);
        }
        
        state_->set_value(std::forward<U>(value));
    }
    
    /// 设置值（void类型）
    template <typename U = T>
    typename std::enable_if<std::is_void<U>::value>::type
    set_value() {
        if (!state_) {
            throw std::future_error(std::future_errc::no_state);
        }
        
        state_->set_value();
    }
    
    /// 设置异常
    void set_exception(std::exception_ptr p) {
        if (!state_) {
            throw std::future_error(std::future_errc::no_state);
        }
        
        state_->set_exception(p);
    }
    
    /// 交换两个promise
    void swap(promise& other) noexcept {
        std::swap(state_, other.state_);
        std::swap(future_retrieved_, other.future_retrieved_);
    }
    
private:
    /// 共享状态
    std::shared_ptr<typename future<T>::shared_state> state_;
    
    /// 标记future是否已被获取
    bool future_retrieved_;
};

/// 特化void类型的promise
template <>
class promise<void> {
public:
    /// 默认构造函数
    promise()
        : state_(std::make_shared<future<void>::shared_state>()),
          future_retrieved_(false) {}
    
    /// 移动构造函数
    promise(promise&& other) noexcept
        : state_(std::move(other.state_)),
          future_retrieved_(other.future_retrieved_) {
        other.future_retrieved_ = false;
    }
    
    /// 移动赋值运算符
    promise& operator=(promise&& other) noexcept {
        if (this != &other) {
            state_ = std::move(other.state_);
            future_retrieved_ = other.future_retrieved_;
            other.future_retrieved_ = false;
        }
        return *this;
    }
    
    /// 禁止复制
    promise(const promise&) = delete;
    promise& operator=(const promise&) = delete;
    
    /// 析构函数
    ~promise() {
        if (state_) {
            // 如果promise被销毁但没有设置值，设置一个异常
            try {
                std::unique_lock<std::mutex> lock(state_->mutex, std::try_to_lock);
                if (lock.owns_lock() && !state_->ready) {
                    state_->set_exception(std::make_exception_ptr(
                        std::future_error(std::future_errc::broken_promise)));
                }
            } catch (...) {
                // 忽略任何异常
            }
        }
    }
    
    /// 获取关联的future
    future<void> get_future() {
        if (!state_) {
            throw std::future_error(std::future_errc::no_state);
        }
        
        if (future_retrieved_) {
            throw std::future_error(std::future_errc::future_already_retrieved);
        }
        
        future_retrieved_ = true;
        return future<void>(state_);
    }
    
    /// 设置值
    void set_value() {
        if (!state_) {
            throw std::future_error(std::future_errc::no_state);
        }
        
        state_->set_value();
    }
    
    /// 设置异常
    void set_exception(std::exception_ptr p) {
        if (!state_) {
            throw std::future_error(std::future_errc::no_state);
        }
        
        state_->set_exception(p);
    }
    
    /// 交换两个promise
    void swap(promise& other) noexcept {
        std::swap(state_, other.state_);
        std::swap(future_retrieved_, other.future_retrieved_);
    }
    
private:
    /// 共享状态
    std::shared_ptr<future<void>::shared_state> state_;
    
    /// 标记future是否已被获取
    bool future_retrieved_;
};

} // namespace uasio

#endif // UASIO_PROMISE_H 