#ifndef UASIO_FUTURE_H
#define UASIO_FUTURE_H

#include <memory>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>
#include <vector>
#include <exception>
#include <type_traits>
#include "error.h"

namespace uasio {

// 前置声明
template <typename T> class promise;
template <typename T> class packaged_task;

/**
 * @brief Future类，提供异步操作的结果获取机制
 * @tparam T 结果类型
 */
template <typename T>
class future {
public:
    /// 默认构造函数，创建无效的future
    future() noexcept = default;
    
    /// 移动构造函数
    future(future&& other) noexcept 
        : state_(std::move(other.state_)) {}
    
    /// 移动赋值运算符
    future& operator=(future&& other) noexcept {
        if (this != &other) {
            state_ = std::move(other.state_);
        }
        return *this;
    }
    
    /// 析构函数
    ~future() = default;
    
    /// 禁止复制
    future(const future&) = delete;
    future& operator=(const future&) = delete;
    
    /// 检查future是否有效
    bool valid() const noexcept {
        return state_ != nullptr;
    }
    
    /// 等待结果可用
    void wait() const {
        if (!valid()) {
            throw std::runtime_error("future has no shared state");
        }
        
        std::unique_lock<std::mutex> lock(state_->mutex);
        state_->cv.wait(lock, [this] { return state_->ready; });
    }
    
    /// 带超时的等待
    template <typename Rep, typename Period>
    bool wait_for(const std::chrono::duration<Rep, Period>& timeout_duration) const {
        if (!valid()) {
            throw std::runtime_error("future has no shared state");
        }
        
        std::unique_lock<std::mutex> lock(state_->mutex);
        return state_->cv.wait_for(lock, timeout_duration, [this] { return state_->ready; });
    }
    
    /// 带截止时间的等待
    template <typename Clock, typename Duration>
    bool wait_until(const std::chrono::time_point<Clock, Duration>& timeout_time) const {
        if (!valid()) {
            throw std::runtime_error("future has no shared state");
        }
        
        std::unique_lock<std::mutex> lock(state_->mutex);
        return state_->cv.wait_until(lock, timeout_time, [this] { return state_->ready; });
    }
    
    /// 获取结果（移动）
    T get() {
        if (!valid()) {
            throw std::runtime_error("future has no shared state");
        }
        
        wait();
        
        std::unique_lock<std::mutex> lock(state_->mutex);
        state_->retrieve_exception();
        
        if constexpr (std::is_void_v<T>) {
            // 对于void类型，不返回值
            auto state = std::move(state_);
            return;
        } else {
            // 对于非void类型，移动返回值
            auto state = std::move(state_);
            return std::move(state->value.value());
        }
    }
    
    /// 检查future是否就绪
    bool is_ready() const noexcept {
        if (!valid()) {
            return false;
        }
        
        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->ready;
    }
    
    /// 添加完成时的回调函数
    template <typename F>
    void then(F&& func) {
        if (!valid()) {
            throw std::runtime_error("future has no shared state");
        }
        
        std::unique_lock<std::mutex> lock(state_->mutex);
        
        if (state_->ready) {
            // 如果已就绪，立即调用回调
            lock.unlock();
            invoke_callback(std::forward<F>(func));
        } else {
            // 否则，存储回调
            state_->callbacks.emplace_back([this, f = std::forward<F>(func)]() mutable {
                invoke_callback(std::move(f));
            });
        }
    }
    
private:
    /// 允许promise, packaged_task访问私有成员
    template <typename U> friend class promise;
    template <typename U> friend class packaged_task;
    
    /// 共享状态结构
    struct shared_state {
        std::mutex mutex;
        std::condition_variable cv;
        bool ready = false;
        std::exception_ptr exception;
        
        // 对于非void类型的值
        std::conditional_t<std::is_void_v<T>, 
                           std::monostate, 
                           std::optional<T>> value;
        
        // 回调函数列表
        std::vector<std::function<void()>> callbacks;
        
        // 设置值（非void类型）
        template <typename U = T>
        typename std::enable_if<!std::is_void_v<U>>::type
        set_value(U&& val) {
            std::unique_lock<std::mutex> lock(mutex);
            if (ready) {
                throw std::runtime_error("value already set");
            }
            
            value = std::forward<U>(val);
            ready = true;
            
            auto callbacks_copy = std::move(callbacks);
            lock.unlock();
            
            cv.notify_all();
            
            // 执行所有回调
            for (auto& callback : callbacks_copy) {
                callback();
            }
        }
        
        // 设置值（void类型）
        template <typename U = T>
        typename std::enable_if<std::is_void_v<U>>::type
        set_value() {
            std::unique_lock<std::mutex> lock(mutex);
            if (ready) {
                throw std::runtime_error("value already set");
            }
            
            ready = true;
            
            auto callbacks_copy = std::move(callbacks);
            lock.unlock();
            
            cv.notify_all();
            
            // 执行所有回调
            for (auto& callback : callbacks_copy) {
                callback();
            }
        }
        
        // 设置异常
        void set_exception(std::exception_ptr e) {
            std::unique_lock<std::mutex> lock(mutex);
            if (ready) {
                throw std::runtime_error("value already set");
            }
            
            exception = e;
            ready = true;
            
            auto callbacks_copy = std::move(callbacks);
            lock.unlock();
            
            cv.notify_all();
            
            // 执行所有回调
            for (auto& callback : callbacks_copy) {
                callback();
            }
        }
        
        // 检查并抛出异常
        void retrieve_exception() {
            if (exception) {
                std::rethrow_exception(exception);
            }
        }
    };
    
    /// 从共享状态构造
    explicit future(std::shared_ptr<shared_state> state) noexcept
        : state_(std::move(state)) {}
    
    /// 调用回调函数
    template <typename F>
    auto invoke_callback(F&& func) -> decltype(func()) {
        try {
            if constexpr (std::is_void_v<T>) {
                // 对于void类型，直接调用回调
                state_->retrieve_exception();
                return func();
            } else {
                // 对于非void类型，传递值给回调
                state_->retrieve_exception();
                return func(state_->value.value());
            }
        } catch (...) {
            // 捕获并重新抛出异常
            throw;
        }
    }
    
    /// 共享状态
    std::shared_ptr<shared_state> state_;
};

// 特化void类型的future
template <>
class future<void> {
public:
    /// 默认构造函数，创建无效的future
    future() noexcept = default;
    
    /// 移动构造函数
    future(future&& other) noexcept 
        : state_(std::move(other.state_)) {}
    
    /// 移动赋值运算符
    future& operator=(future&& other) noexcept {
        if (this != &other) {
            state_ = std::move(other.state_);
        }
        return *this;
    }
    
    /// 析构函数
    ~future() = default;
    
    /// 禁止复制
    future(const future&) = delete;
    future& operator=(const future&) = delete;
    
    /// 检查future是否有效
    bool valid() const noexcept {
        return state_ != nullptr;
    }
    
    /// 等待结果可用
    void wait() const {
        if (!valid()) {
            throw std::runtime_error("future has no shared state");
        }
        
        std::unique_lock<std::mutex> lock(state_->mutex);
        state_->cv.wait(lock, [this] { return state_->ready; });
    }
    
    /// 带超时的等待
    template <typename Rep, typename Period>
    bool wait_for(const std::chrono::duration<Rep, Period>& timeout_duration) const {
        if (!valid()) {
            throw std::runtime_error("future has no shared state");
        }
        
        std::unique_lock<std::mutex> lock(state_->mutex);
        return state_->cv.wait_for(lock, timeout_duration, [this] { return state_->ready; });
    }
    
    /// 带截止时间的等待
    template <typename Clock, typename Duration>
    bool wait_until(const std::chrono::time_point<Clock, Duration>& timeout_time) const {
        if (!valid()) {
            throw std::runtime_error("future has no shared state");
        }
        
        std::unique_lock<std::mutex> lock(state_->mutex);
        return state_->cv.wait_until(lock, timeout_time, [this] { return state_->ready; });
    }
    
    /// 获取结果（可能抛出异常）
    void get() {
        if (!valid()) {
            throw std::runtime_error("future has no shared state");
        }
        
        wait();
        
        std::unique_lock<std::mutex> lock(state_->mutex);
        auto state = std::move(state_);
        state->retrieve_exception();
    }
    
    /// 检查future是否就绪
    bool is_ready() const noexcept {
        if (!valid()) {
            return false;
        }
        
        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->ready;
    }
    
    /// 添加完成时的回调函数
    template <typename F>
    void then(F&& func) {
        if (!valid()) {
            throw std::runtime_error("future has no shared state");
        }
        
        std::unique_lock<std::mutex> lock(state_->mutex);
        
        if (state_->ready) {
            // 如果已就绪，立即调用回调
            lock.unlock();
            invoke_callback(std::forward<F>(func));
        } else {
            // 否则，存储回调
            state_->callbacks.emplace_back([this, f = std::forward<F>(func)]() mutable {
                invoke_callback(std::move(f));
            });
        }
    }
    
private:
    /// 允许promise, packaged_task访问私有成员
    friend class promise<void>;
    friend class packaged_task<void>;
    
    /// 共享状态结构
    struct shared_state {
        std::mutex mutex;
        std::condition_variable cv;
        bool ready = false;
        std::exception_ptr exception;
        
        // 回调函数列表
        std::vector<std::function<void()>> callbacks;
        
        // 设置值
        void set_value() {
            std::unique_lock<std::mutex> lock(mutex);
            if (ready) {
                throw std::runtime_error("value already set");
            }
            
            ready = true;
            
            auto callbacks_copy = std::move(callbacks);
            lock.unlock();
            
            cv.notify_all();
            
            // 执行所有回调
            for (auto& callback : callbacks_copy) {
                callback();
            }
        }
        
        // 设置异常
        void set_exception(std::exception_ptr e) {
            std::unique_lock<std::mutex> lock(mutex);
            if (ready) {
                throw std::runtime_error("value already set");
            }
            
            exception = e;
            ready = true;
            
            auto callbacks_copy = std::move(callbacks);
            lock.unlock();
            
            cv.notify_all();
            
            // 执行所有回调
            for (auto& callback : callbacks_copy) {
                callback();
            }
        }
        
        // 检查并抛出异常
        void retrieve_exception() {
            if (exception) {
                std::rethrow_exception(exception);
            }
        }
    };
    
    /// 从共享状态构造
    explicit future(std::shared_ptr<shared_state> state) noexcept
        : state_(std::move(state)) {}
    
    /// 调用回调函数
    template <typename F>
    auto invoke_callback(F&& func) -> decltype(func()) {
        try {
            state_->retrieve_exception();
            return func();
        } catch (...) {
            // 捕获并重新抛出异常
            throw;
        }
    }
    
    /// 共享状态
    std::shared_ptr<shared_state> state_;
};

/**
 * @brief 创建已就绪的future
 * @tparam T 结果类型
 * @param value 结果值
 * @return 包含结果的future
 */
template <typename T>
future<typename std::decay<T>::type> make_ready_future(T&& value) {
    using result_type = typename std::decay<T>::type;
    auto state = std::make_shared<typename future<result_type>::shared_state>();
    state->set_value(std::forward<T>(value));
    return future<result_type>(std::move(state));
}

/// 创建空的已就绪future
inline future<void> make_ready_future() {
    auto state = std::make_shared<future<void>::shared_state>();
    state->set_value();
    return future<void>(std::move(state));
}

/**
 * @brief 创建包含异常的future
 * @tparam T 结果类型
 * @param e 异常指针
 * @return 包含异常的future
 */
template <typename T>
future<T> make_exceptional_future(std::exception_ptr e) {
    auto state = std::make_shared<typename future<T>::shared_state>();
    state->set_exception(e);
    return future<T>(std::move(state));
}

/**
 * @brief 创建包含异常的future
 * @tparam T 结果类型
 * @tparam E 异常类型
 * @param e 异常对象
 * @return 包含异常的future
 */
template <typename T, typename E>
future<T> make_exceptional_future(E e) {
    try {
        throw e;
    } catch (...) {
        return make_exceptional_future<T>(std::current_exception());
    }
}

/**
 * @brief 等待多个future全部完成
 * @tparam InputIt future迭代器类型
 * @param first 开始迭代器
 * @param last 结束迭代器
 * @return 包含所有结果的future
 */
template <typename InputIt>
auto when_all(InputIt first, InputIt last) 
    -> future<std::vector<typename std::iterator_traits<InputIt>::value_type>> {
    using future_type = typename std::iterator_traits<InputIt>::value_type;
    using result_type = std::vector<future_type>;
    
    // 如果没有future，返回空vector
    if (first == last) {
        return make_ready_future(result_type{});
    }
    
    // 保存所有输入future
    std::vector<future_type> futures;
    futures.reserve(std::distance(first, last));
    for (auto it = first; it != last; ++it) {
        futures.push_back(std::move(*it));
    }
    
    // 创建共享状态
    auto state = std::make_shared<typename future<result_type>::shared_state>();
    
    // 创建计数器
    auto counter = std::make_shared<std::atomic<std::size_t>>(futures.size());
    
    // 为每个future添加回调
    for (auto& f : futures) {
        if (!f.valid()) {
            // 无效future，直接完成一个计数
            if (--(*counter) == 0) {
                state->set_value(std::move(futures));
            }
        } else {
            f.then([f_copy = std::move(f), counter, state, futures_copy = futures]() mutable {
                // 当任何一个future完成时，检查所有future是否都完成
                if (--(*counter) == 0) {
                    state->set_value(std::move(futures_copy));
                }
            });
        }
    }
    
    return future<result_type>(std::move(state));
}

/**
 * @brief 等待多个future中任意一个完成
 * @tparam InputIt future迭代器类型
 * @param first 开始迭代器
 * @param last 结束迭代器
 * @return 包含第一个完成的future的索引
 */
template <typename InputIt>
future<std::size_t> when_any(InputIt first, InputIt last) {
    using future_type = typename std::iterator_traits<InputIt>::value_type;
    
    // 如果没有future，返回错误
    if (first == last) {
        return make_exceptional_future<std::size_t>(
            std::make_exception_ptr(std::runtime_error("empty future sequence")));
    }
    
    // 保存所有输入future
    std::vector<future_type> futures;
    futures.reserve(std::distance(first, last));
    for (auto it = first; it != last; ++it) {
        futures.push_back(std::move(*it));
    }
    
    // 创建共享状态
    auto state = std::make_shared<typename future<std::size_t>::shared_state>();
    
    // 创建已完成标志
    auto done = std::make_shared<std::atomic<bool>>(false);
    
    // 为每个future添加回调
    for (std::size_t i = 0; i < futures.size(); ++i) {
        auto& f = futures[i];
        
        if (!f.valid()) {
            // 无效future，跳过
            continue;
        }
        
        if (f.is_ready()) {
            // 如果已经有ready的future，立即返回
            bool expected = false;
            if (done->compare_exchange_strong(expected, true)) {
                state->set_value(i);
                break;
            }
        } else {
            // 添加回调
            f.then([i, done, state]() {
                bool expected = false;
                if (done->compare_exchange_strong(expected, true)) {
                    state->set_value(i);
                }
            });
        }
    }
    
    return future<std::size_t>(std::move(state));
}

} // namespace uasio

#endif // UASIO_FUTURE_H 