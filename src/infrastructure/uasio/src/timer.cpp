// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "timer.h"
#include <iostream>
#include <thread>

namespace uasio {

//
// deadline_timer 实现
//

deadline_timer::deadline_timer(io_context& io_context)
    : service_(std::make_shared<deadline_timer_service>(io_context))
    , expires_at_(clock_type::now())
    , io_context_(io_context)
{
}

deadline_timer::deadline_timer(io_context& io_context, const time_point& expiry_time)
    : service_(std::make_shared<deadline_timer_service>(io_context))
    , expires_at_(expiry_time)
    , io_context_(io_context)
{
}

deadline_timer::deadline_timer(io_context& io_context, const duration& expiry_time)
    : service_(std::make_shared<deadline_timer_service>(io_context))
    , expires_at_(clock_type::now() + expiry_time)
    , io_context_(io_context)
{
}

deadline_timer::~deadline_timer()
{
    // 取消所有定时器操作
    cancel();
}

std::size_t deadline_timer::cancel()
{
    if (service_) {
        return service_->cancel();
    }
    return 0;
}

void deadline_timer::expires_at(const time_point& expiry_time)
{
    expires_at_ = expiry_time;
}

deadline_timer::time_point deadline_timer::expires_at() const
{
    return expires_at_;
}

void deadline_timer::expires_from_now(const duration& expiry_time)
{
    expires_at_ = clock_type::now() + expiry_time;
}

deadline_timer::duration deadline_timer::expires_from_now() const
{
    auto now = clock_type::now();
    if (expires_at_ <= now) {
        return duration::zero();
    }
    return expires_at_ - now;
}

//
// deadline_timer_service 实现
//

deadline_timer_service::deadline_timer_service(io_context& io_context)
    : io_context_(io_context)
    , timer_checker_running_(false)
{
}

deadline_timer_service::~deadline_timer_service()
{
    // 取消所有定时器操作
    cancel();
}

void deadline_timer_service::async_wait(
    const deadline_timer::time_point& expires_at,
    std::function<void(const uasio::error_code&)>&& handler)
{
    bool need_start_checker = false;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // 添加定时器操作
        timer_op op{expires_at, std::move(handler)};
        timers_.push(std::move(op));
        
        // 如果没有检查器运行，需要启动一个
        if (!timer_checker_running_) {
            timer_checker_running_ = true;
            need_start_checker = true;
        }
    }
    
    // 如果需要启动检查器，则提交检查任务
    if (need_start_checker) {
        io_context_.post([this]() {
            check_timers();
        });
    }
}

std::size_t deadline_timer_service::cancel()
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 处理所有定时器操作，发送取消事件
    std::size_t count = 0;
    while (!timers_.empty()) {
        auto& top = timers_.top();
        auto handler = std::move(top.handler);
        timers_.pop();
        
        if (handler) {
            // 发送取消错误码
            uasio::error_code ec(1, uasio::error_category()); // 假设 1 表示取消
            handler(ec);
            ++count;
        }
    }
    
    return count;
}

void deadline_timer_service::check_timers()
{
    bool reschedule = false;
    
    // 处理已到期的定时器
    for (;;) {
        std::function<void(const uasio::error_code&)> handler;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            // 如果没有更多定时器，停止检查
            if (timers_.empty()) {
                timer_checker_running_ = false;
                break;
            }
            
            // 获取下一个定时器
            auto now = deadline_timer::clock_type::now();
            auto& top = timers_.top();
            
            if (top.expires_at <= now) {
                // 定时器已到期，处理它
                handler = std::move(top.handler);
                timers_.pop();
            } else {
                // 定时器未到期，需要重新调度
                reschedule = true;
                break;
            }
        }
        
        // 调用处理函数（在锁外）
        if (handler) {
            uasio::error_code ec; // 成功
            handler(ec);
        }
    }
    
    // 如果需要重新调度，计算下一个定时器的延迟并提交新的检查任务
    if (reschedule) {
        deadline_timer::duration delay;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (timers_.empty()) {
                timer_checker_running_ = false;
                return;
            }
            
            // 计算到下一个定时器的延迟
            auto now = deadline_timer::clock_type::now();
            auto next_expiry = timers_.top().expires_at;
            delay = (next_expiry > now) ? (next_expiry - now) : deadline_timer::duration::zero();
        }
        
        // 使用 sleep 模拟定时
        std::this_thread::sleep_for(delay);
        
        // 提交新的检查任务
        io_context_.post([this]() {
            check_timers();
        });
    }
}

} // namespace uasio 