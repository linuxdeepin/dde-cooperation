#ifndef UASIO_WAIT_GROUP_H
#define UASIO_WAIT_GROUP_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <functional>

namespace uasio {

/**
 * @brief 等待组，用于等待多个异步操作完成
 */
class wait_group {
public:
    /// 构造函数
    wait_group() : counter_(0) {}

    /// 增加等待计数
    void add(int delta = 1) {
        if (delta <= 0) {
            return;
        }
        counter_ += delta;
    }

    /// 减少等待计数
    void done() {
        if (counter_ > 0) {
            if (--counter_ == 0) {
                std::lock_guard<std::mutex> lock(mutex_);
                cv_.notify_all();
            }
        }
    }

    /// 等待所有操作完成
    void wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return counter_ == 0; });
    }

    /// 等待所有操作完成，带超时
    template <typename Rep, typename Period>
    bool wait_for(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, timeout, [this]() { return counter_ == 0; });
    }

    /// 等待所有操作完成，带截止时间
    template <typename Clock, typename Duration>
    bool wait_until(const std::chrono::time_point<Clock, Duration>& deadline) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_until(lock, deadline, [this]() { return counter_ == 0; });
    }

    /// 获取当前计数值
    int count() const noexcept {
        return counter_.load();
    }

    /// 创建一个任务结束时自动调用done()的包装器
    template <typename Function>
    auto wrap(Function&& func) {
        add(1);
        return [this, f = std::forward<Function>(func)](auto&&... args) {
            // 确保done()始终被调用，即使函数抛出异常
            struct guard {
                wait_group* wg;
                ~guard() { wg->done(); }
            } g{this};
            
            // 调用原始函数
            return f(std::forward<decltype(args)>(args)...);
        };
    }

private:
    std::atomic<int> counter_;
    std::condition_variable cv_;
    std::mutex mutex_;
};

} // namespace uasio

#endif // UASIO_WAIT_GROUP_H 