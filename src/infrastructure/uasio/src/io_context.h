// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_IO_CONTEXT_H
#define UASIO_IO_CONTEXT_H

#include "execution_context.h"
#include <functional>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

namespace uasio {

class io_context : public execution_context {
public:
    // 执行器类型
    class executor_type;
    
    // 默认构造函数
    io_context();
    
    // 带并发提示的构造函数
    explicit io_context(int concurrency_hint);
    
    // 不允许拷贝
    io_context(const io_context&) = delete;
    io_context& operator=(const io_context&) = delete;
    
    // 析构函数
    ~io_context();
    
    // 获取关联执行器
    executor_type get_executor() noexcept;
    
    // 运行事件循环，直到没有更多工作或被停止
    std::size_t run();
    
    // 运行一次事件循环迭代
    std::size_t run_one();
    
    // 轮询是否有工作要做，完成所有可用工作
    std::size_t poll();
    
    // 轮询是否有工作要做，完成一个工作
    std::size_t poll_one();
    
    // 停止事件循环
    void stop();
    
    // 重启事件循环
    void restart();
    
    // 检查是否已停止
    bool stopped() const;
    
    // 提交一个任务到 io_context
    template <typename CompletionHandler>
    void post(CompletionHandler&& handler) {
        std::unique_lock<std::mutex> lock(mutex_);
        tasks_.push(std::forward<CompletionHandler>(handler));
        lock.unlock();
        cond_.notify_one();
    }
    
    // 在当前线程或提交到 io_context 执行任务
    template <typename CompletionHandler>
    void dispatch(CompletionHandler&& handler) {
        // 如果在本线程，直接执行
        if (std::this_thread::get_id() == owner_thread_id_) {
            handler();
        } else {
            post(std::forward<CompletionHandler>(handler));
        }
    }
    
    // 工作守卫类，防止 io_context 退出
    class work {
    public:
        explicit work(io_context& ctx);
        work(const work&) = delete;
        work& operator=(const work&) = delete;
        work(work&& other) noexcept;
        work& operator=(work&& other) noexcept;
        ~work();
        
        void reset();
        
    private:
        io_context* context_;
    };
    
    // 创建工作守卫
    work make_work();
    
private:
    // 当前运行线程ID
    std::thread::id owner_thread_id_;
    
    // 任务队列
    std::queue<std::function<void()>> tasks_;
    
    // 同步对象
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    
    // 停止标志
    bool stopped_;
};

// 执行器类型定义
class io_context::executor_type {
public:
    executor_type() noexcept : context_(nullptr) {}
    explicit executor_type(io_context& ctx) noexcept : context_(&ctx) {}
    
    bool running_in_this_thread() const noexcept;
    
    template <typename Function>
    void execute(Function&& f) const {
        if (context_) {
            context_->post(std::forward<Function>(f));
        }
    }
    
    template <typename Function>
    void dispatch(Function&& f) const {
        if (context_) {
            context_->dispatch(std::forward<Function>(f));
        }
    }
    
    template <typename Function>
    void post(Function&& f) const {
        if (context_) {
            context_->post(std::forward<Function>(f));
        }
    }
    
    bool operator==(const executor_type& other) const noexcept {
        return context_ == other.context_;
    }
    
    bool operator!=(const executor_type& other) const noexcept {
        return !(*this == other);
    }
    
private:
    io_context* context_;
};

} // namespace uasio

#endif // UASIO_IO_CONTEXT_H 