// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_TIMER_H
#define UASIO_TIMER_H

#include "io_context.h"
#include "error.h"
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

namespace uasio {

// 前向声明
class deadline_timer_service;

/**
 * @brief 定时器类，用于异步等待指定时间
 */
class deadline_timer {
public:
    // 时间点和时间长度类型
    using clock_type = std::chrono::steady_clock;
    using time_point = clock_type::time_point;
    using duration = clock_type::duration;

    /**
     * @brief 构造函数
     * @param io_context IO上下文
     */
    explicit deadline_timer(io_context& io_context);
    
    /**
     * @brief 构造函数，带初始到期时间
     * @param io_context IO上下文
     * @param expiry_time 到期时间
     */
    deadline_timer(io_context& io_context, const time_point& expiry_time);
    
    /**
     * @brief 构造函数，带初始时间长度
     * @param io_context IO上下文
     * @param expiry_time 到期时间长度
     */
    deadline_timer(io_context& io_context, const duration& expiry_time);
    
    /**
     * @brief 析构函数
     */
    ~deadline_timer();
    
    /**
     * @brief 取消所有挂起的异步操作
     * @return 取消的操作数
     */
    std::size_t cancel();
    
    /**
     * @brief 设置到期时间
     * @param expiry_time 到期时间
     */
    void expires_at(const time_point& expiry_time);
    
    /**
     * @brief 获取到期时间
     * @return 到期时间
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
            return;
        }
        
        // 包装回调函数
        auto callback = [handler = std::forward<WaitHandler>(handler)](const uasio::error_code& ec) mutable {
            handler(ec);
        };
        
        // 提交异步等待
        service_->async_wait(expires_at_, std::move(callback));
    }
    
private:
    // 定时器服务实现
    std::shared_ptr<deadline_timer_service> service_;
    
    // 到期时间
    time_point expires_at_;
    
    // IO上下文
    io_context& io_context_;
};

/**
 * @brief 定时器服务，用于管理定时器
 */
class deadline_timer_service {
public:
    /**
     * @brief 构造函数
     * @param io_context IO上下文
     */
    explicit deadline_timer_service(io_context& io_context);
    
    /**
     * @brief 析构函数
     */
    ~deadline_timer_service();
    
    /**
     * @brief 异步等待
     * @param expires_at 到期时间
     * @param handler 回调函数
     */
    void async_wait(const deadline_timer::time_point& expires_at,
                   std::function<void(const uasio::error_code&)>&& handler);
    
    /**
     * @brief 取消所有挂起的异步操作
     * @return 取消的操作数
     */
    std::size_t cancel();
    
private:
    // 定时器任务结构
    struct timer_op {
        deadline_timer::time_point expires_at;
        std::function<void(const uasio::error_code&)> handler;
        
        // 用于优先队列的比较函数
        bool operator>(const timer_op& other) const {
            return expires_at > other.expires_at;
        }
    };
    
    // 检查并处理到期的定时器
    void check_timers();
    
    // 定时器队列（优先队列，最早到期的在前）
    using timer_queue = std::priority_queue<timer_op, std::vector<timer_op>, std::greater<>>;
    timer_queue timers_;
    
    // 线程安全
    std::mutex mutex_;
    
    // IO上下文
    io_context& io_context_;
    
    // 是否有定时器检查任务在运行
    bool timer_checker_running_;
};

} // namespace uasio

#endif // UASIO_TIMER_H 