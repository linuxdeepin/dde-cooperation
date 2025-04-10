#ifndef UASIO_PACKAGED_TASK_H
#define UASIO_PACKAGED_TASK_H

#include "future.h"
#include "promise.h"
#include <functional>
#include <memory>
#include <type_traits>

namespace uasio {

/**
 * @brief 任务封装类，将函数与Future-Promise机制结合
 * @tparam R 任务返回类型
 * @tparam Args 任务参数类型
 */
template <typename R, typename... Args>
class packaged_task {
public:
    /// 任务函数类型
    using task_type = std::function<R(Args...)>;
    
    /// 默认构造函数
    packaged_task() noexcept = default;
    
    /// 从函数构造
    template <typename F>
    explicit packaged_task(F&& f)
        : func_(std::forward<F>(f)),
          promise_() {}
    
    /// 移动构造函数
    packaged_task(packaged_task&& other) noexcept
        : func_(std::move(other.func_)),
          promise_(std::move(other.promise_)) {}
    
    /// 移动赋值函数
    packaged_task& operator=(packaged_task&& other) noexcept {
        if (this != &other) {
            func_ = std::move(other.func_);
            promise_ = std::move(other.promise_);
        }
        return *this;
    }
    
    /// 禁止复制
    packaged_task(const packaged_task&) = delete;
    packaged_task& operator=(const packaged_task&) = delete;
    
    /// 析构函数
    ~packaged_task() = default;
    
    /// 获取关联的future
    future<R> get_future() {
        return promise_.get_future();
    }
    
    /// 检查任务是否有效
    bool valid() const noexcept {
        return static_cast<bool>(func_);
    }
    
    /// 交换两个packaged_task
    void swap(packaged_task& other) noexcept {
        std::swap(func_, other.func_);
        promise_.swap(other.promise_);
    }
    
    /// 调用任务函数，并设置结果到关联的future
    void operator()(Args... args) {
        if (!valid()) {
            throw std::future_error(std::future_errc::no_state);
        }
        
        try {
            invoke_and_set_value(std::move(args)...);
        } catch (...) {
            promise_.set_exception(std::current_exception());
        }
    }
    
    /// 重置任务
    void reset() {
        packaged_task tmp(std::move(func_));
        *this = std::move(tmp);
    }
    
private:
    /// 调用函数并设置结果值（非void返回类型）
    template <typename... T>
    typename std::enable_if<!std::is_void<R>::value>::type
    invoke_and_set_value(T&&... args) {
        promise_.set_value(func_(std::forward<T>(args)...));
    }
    
    /// 调用函数并设置结果值（void返回类型）
    template <typename... T>
    typename std::enable_if<std::is_void<R>::value>::type
    invoke_and_set_value(T&&... args) {
        func_(std::forward<T>(args)...);
        promise_.set_value();
    }
    
    /// 任务函数
    task_type func_;
    
    /// Promise对象
    promise<R> promise_;
};

} // namespace uasio

#endif // UASIO_PACKAGED_TASK_H 