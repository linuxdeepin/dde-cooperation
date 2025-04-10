// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_STEADY_TIMER_H
#define UASIO_STEADY_TIMER_H

#include "io_context.h"
#include "error.h"
#include <chrono>
#include <functional>
#include <mutex>
#include <queue>
#include <memory>
#include <utility>

namespace uasio {

/**
 * @brief 基于steady_clock的定时器
 * 
 * 提供基于std::chrono::steady_clock的定时器功能，不受系统时间调整的影响，
 * 适合需要精确计时的场景。
 */
class steady_timer {
public:
    /// 时钟类型
    using clock_type = std::chrono::steady_clock;
    /// 时间点类型
    using time_point = clock_type::time_point;
    /// 持续时间类型
    using duration = clock_type::duration;
    
    /**
     * @brief 构造函数
     * @param io_context IO上下文对象
     */
    explicit steady_timer(uasio::io_context& io_context);
    
    /**
     * @brief 析构函数
     */
    ~steady_timer();
    
    /**
     * @brief 取消所有异步等待操作
     * @return 取消的操作数量
     */
    std::size_t cancel();
    
    /**
     * @brief 设置到期的绝对时间点
     * @param expiry_time 到期时间点
     */
    void expires_at(const time_point& expiry_time);
    
    /**
     * @brief 获取到期的绝对时间点
     * @return 到期时间点
     */
    time_point expires_at() const;
    
    /**
     * @brief 设置到期的时间长度（相对当前时间）
     * @param expiry_time 到期时间长度
     */
    void expires_from_now(const duration& expiry_time);
    
    /**
     * @brief 获取到期的时间长度（相对当前时间）
     * @return 到期时间长度
     */
    duration expires_from_now() const;
    
    /**
     * @brief 异步等待定时器到期
     * @param handler 到期后的回调函数，格式为 void(uasio::error_code)
     */
    template <typename WaitHandler>
    void async_wait(WaitHandler&& handler) {
        if (!service_) {
            handler(uasio::make_error_code(uasio::error::operation_aborted));
            return;
        }
        service_->async_wait(expires_at_, std::forward<WaitHandler>(handler));
    }
    
private:
    class steady_timer_service {
    public:
        explicit steady_timer_service(uasio::io_context& io_context)
            : io_context_(io_context), shutdown_(false) {
            worker_thread_ = std::thread([this]() { worker_thread_func(); });
        }
        
        ~steady_timer_service() {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                shutdown_ = true;
                // 清空定时器队列
                while (!timer_queue_.empty()) {
                    timer_queue_.pop();
                }
            }
            cv_.notify_all();
            if (worker_thread_.joinable()) {
                worker_thread_.join();
            }
        }
        
        void async_wait(const time_point& expires_at,
                       std::function<void(const uasio::error_code&)>&& handler) {
            std::lock_guard<std::mutex> lock(mutex_);
            
            if (shutdown_) {
                io_context_.post([handler]() {
                    handler(uasio::make_error_code(uasio::error::operation_aborted));
                });
                return;
            }
            
            timer_queue_.push(timer_op{expires_at, std::move(handler)});
            
            // 如果新添加的定时器比当前处理的定时器更早到期，唤醒工作线程
            if (timer_queue_.size() == 1 || expires_at < timer_queue_.top().expires_at) {
                cv_.notify_one();
            }
        }
        
        std::size_t cancel() {
            std::lock_guard<std::mutex> lock(mutex_);
            
            std::size_t cancelled_count = timer_queue_.size();
            
            // 取消所有定时器
            while (!timer_queue_.empty()) {
                auto op = timer_queue_.top();
                timer_queue_.pop();
                
                io_context_.post([handler = std::move(op.handler)]() {
                    handler(uasio::make_error_code(uasio::error::operation_aborted));
                });
            }
            
            return cancelled_count;
        }
        
    private:
        struct timer_op {
            time_point expires_at;
            std::function<void(const uasio::error_code&)> handler;
            
            // 用于优先队列的比较函数
            bool operator>(const timer_op& other) const {
                return expires_at > other.expires_at;
            }
        };
        
        // 工作线程函数
        void worker_thread_func() {
            while (true) {
                std::unique_lock<std::mutex> lock(mutex_);
                
                if (shutdown_) {
                    break;
                }
                
                if (timer_queue_.empty()) {
                    // 如果没有定时器，等待直到有新的定时器添加或者关闭
                    cv_.wait(lock);
                    continue;
                }
                
                auto next_timer = timer_queue_.top();
                auto now = clock_type::now();
                
                if (next_timer.expires_at <= now) {
                    // 定时器已到期，处理它
                    timer_queue_.pop();
                    
                    // 解锁互斥量，以便其他线程可以添加定时器
                    auto handler = std::move(next_timer.handler);
                    lock.unlock();
                    
                    // 在IO上下文中执行回调
                    io_context_.post([handler]() {
                        handler(uasio::make_error_code(0));
                    });
                } else {
                    // 定时器未到期，等待直到下一个定时器到期或有新的定时器添加
                    cv_.wait_until(lock, next_timer.expires_at);
                }
            }
        }
        
        uasio::io_context& io_context_;
        std::mutex mutex_;
        std::condition_variable cv_;
        std::priority_queue<timer_op, std::vector<timer_op>, std::greater<timer_op>> timer_queue_;
        std::thread worker_thread_;
        bool shutdown_;
    };
    
    std::shared_ptr<steady_timer_service> service_;
    time_point expires_at_;
};

// 实现部分

inline steady_timer::steady_timer(uasio::io_context& io_context)
    : service_(std::make_shared<steady_timer_service>(io_context)),
      expires_at_(clock_type::now())
{
}

inline steady_timer::~steady_timer()
{
    cancel();
}

inline std::size_t steady_timer::cancel()
{
    return service_->cancel();
}

inline void steady_timer::expires_at(const time_point& expiry_time)
{
    expires_at_ = expiry_time;
}

inline steady_timer::time_point steady_timer::expires_at() const
{
    return expires_at_;
}

inline void steady_timer::expires_from_now(const duration& expiry_time)
{
    expires_at_ = clock_type::now() + expiry_time;
}

inline steady_timer::duration steady_timer::expires_from_now() const
{
    time_point now = clock_type::now();
    if (expires_at_ > now) {
        return expires_at_ - now;
    } else {
        return duration::zero();
    }
}

} // namespace uasio

#endif // UASIO_STEADY_TIMER_H 