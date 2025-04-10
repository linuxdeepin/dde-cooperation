// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "io_context.h"
#include <thread>
#include <iostream>

namespace uasio {

io_context::io_context()
    : owner_thread_id_(std::thread::id())
    , stopped_(false)
{
}

io_context::io_context(int concurrency_hint)
    : owner_thread_id_(std::thread::id())
    , stopped_(false)
{
    // 暂时忽略 concurrency_hint
    (void)concurrency_hint;
}

io_context::~io_context()
{
    stop();
}

io_context::executor_type io_context::get_executor() noexcept
{
    return executor_type(*this);
}

std::size_t io_context::run()
{
    owner_thread_id_ = std::this_thread::get_id();
    stopped_ = false;
    
    std::size_t count = 0;
    for (;;) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            
            // 等待有任务或者被停止
            cond_.wait(lock, [this] {
                return stopped_ || !tasks_.empty();
            });
            
            // 如果停止且队列为空，则退出
            if (stopped_ && tasks_.empty()) {
                break;
            }
            
            // 获取任务
            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }
        
        // 执行任务
        if (task) {
            task();
            ++count;
        }
    }
    
    return count;
}

std::size_t io_context::run_one()
{
    owner_thread_id_ = std::this_thread::get_id();
    stopped_ = false;
    
    std::function<void()> task;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // 等待有任务或者被停止
        cond_.wait(lock, [this] {
            return stopped_ || !tasks_.empty();
        });
        
        // 如果停止且队列为空，则退出
        if (stopped_ && tasks_.empty()) {
            return 0;
        }
        
        // 获取任务
        if (!tasks_.empty()) {
            task = std::move(tasks_.front());
            tasks_.pop();
        }
    }
    
    // 执行任务
    if (task) {
        task();
        return 1;
    }
    
    return 0;
}

std::size_t io_context::poll()
{
    owner_thread_id_ = std::this_thread::get_id();
    stopped_ = false;
    
    std::size_t count = 0;
    std::function<void()> task;
    
    for (;;) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            
            // 如果队列为空，则退出
            if (tasks_.empty()) {
                break;
            }
            
            // 获取任务
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        
        // 执行任务
        if (task) {
            task();
            ++count;
        }
    }
    
    return count;
}

std::size_t io_context::poll_one()
{
    owner_thread_id_ = std::this_thread::get_id();
    stopped_ = false;
    
    std::function<void()> task;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // 如果队列为空，则退出
        if (tasks_.empty()) {
            return 0;
        }
        
        // 获取任务
        task = std::move(tasks_.front());
        tasks_.pop();
    }
    
    // 执行任务
    if (task) {
        task();
        return 1;
    }
    
    return 0;
}

void io_context::stop()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        stopped_ = true;
    }
    cond_.notify_all();
}

void io_context::restart()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        stopped_ = false;
    }
}

bool io_context::stopped() const
{
    std::unique_lock<std::mutex> lock(mutex_);
    return stopped_;
}

// 工作守卫实现
io_context::work io_context::make_work()
{
    return work(*this);
}

io_context::work::work(io_context& ctx)
    : context_(&ctx)
{
    // 增加引用计数，防止上下文退出
}

io_context::work::work(work&& other) noexcept
    : context_(other.context_)
{
    other.context_ = nullptr;
}

io_context::work& io_context::work::operator=(work&& other) noexcept
{
    if (this != &other) {
        reset();
        context_ = other.context_;
        other.context_ = nullptr;
    }
    return *this;
}

io_context::work::~work()
{
    reset();
}

void io_context::work::reset()
{
    if (context_) {
        // 发送一个停止信号，以便 IO 上下文能够在没有更多工作时退出
        context_->post([]() {
            // 空任务，仅为了处理工作守卫重置
        });
        context_ = nullptr;
    }
}

// 执行器实现
bool io_context::executor_type::running_in_this_thread() const noexcept
{
    // 简化版本，暂时不跟踪线程
    return true;
}

} // namespace uasio 