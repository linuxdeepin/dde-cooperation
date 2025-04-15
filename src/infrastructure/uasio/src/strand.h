// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_STRAND_H
#define UASIO_STRAND_H

#include "io_context.h"
#include "executor.h"
#include <mutex>
#include <deque>
#include <memory>
#include <atomic>

namespace uasio {

/**
 * @brief strand类提供线程安全的序列化执行器
 * 
 * 确保通过同一个strand对象提交的所有任务都会按顺序执行，不会并发执行。
 * 这对于避免多线程环境下的数据竞争和同步问题非常有用。
 */
class strand {
public:
    /**
     * @brief 构造函数
     * @param io_context IO上下文对象
     */
    explicit strand(uasio::io_context& io_context);
    
    /**
     * @brief 获取关联的IO上下文
     * @return IO上下文的引用
     */
    uasio::io_context& get_io_context() noexcept;
    
    /**
     * @brief 检查当前线程是否正在执行strand中的任务
     * @return 如果当前线程正在执行strand中的任务，则返回true
     */
    bool running_in_this_thread() const noexcept;
    
    /**
     * @brief 在strand中执行任务
     * @param f 要执行的任务函数
     */
    template <typename Function>
    void execute(Function&& f) const;
    
    /**
     * @brief 在strand中派发任务（如果在strand线程中，则直接执行；否则通过post提交）
     * @param f 要执行的任务函数
     */
    template <typename Function>
    void dispatch(Function&& f) const;
    
    /**
     * @brief 在strand中异步提交任务（总是将任务加入队列）
     * @param f 要执行的任务函数
     */
    template <typename Function>
    void post(Function&& f) const;
    
private:
    class strand_impl;
    std::shared_ptr<strand_impl> impl_;
};

//------------------ 实现部分 ------------------

class strand::strand_impl : public std::enable_shared_from_this<strand_impl> {
public:
    explicit strand_impl(uasio::io_context& io_ctx) 
        : io_context_(io_ctx), executing_(false) {}
    
    uasio::io_context& get_io_context() noexcept {
        return io_context_;
    }
    
    bool running_in_this_thread() const noexcept {
        return current_thread_in_strand_.load();
    }
    
    template <typename Function>
    void execute(Function&& f) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (!executing_) {
            // 没有正在执行的任务，立即执行
            executing_ = true;
            lock.unlock();
            
            // 设置线程标记
            thread_strand_guard guard(*this);
            
            // 执行任务
            std::forward<Function>(f)();
            
            // 尝试执行下一个任务
            run_next_task();
        } else {
            // 已有任务在执行，将此任务加入队列
            tasks_.push_back(std::function<void()>(std::forward<Function>(f)));
        }
    }
    
    template <typename Function>
    void dispatch(Function&& f) {
        if (current_thread_in_strand_.load()) {
            // 当前线程已在strand中，直接执行
            std::forward<Function>(f)();
        } else {
            // 当前线程不在strand中，通过execute提交
            execute(std::forward<Function>(f));
        }
    }
    
    template <typename Function>
    void post(Function&& f) {
        // 将任务包装并提交到io_context
        auto self = shared_from_this();
        io_context_.post([self, f = std::function<void()>(std::forward<Function>(f))]() {
            self->execute(f);
        });
    }
    
private:
    // 线程标记守卫
    class thread_strand_guard {
    public:
        explicit thread_strand_guard(strand_impl& strand) : strand_(strand) {
            strand_.current_thread_in_strand_.store(true);
        }
        
        ~thread_strand_guard() {
            strand_.current_thread_in_strand_.store(false);
        }
        
    private:
        strand_impl& strand_;
    };
    
    // 运行队列中的下一个任务
    void run_next_task() {
        std::function<void()> next_task;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            if (tasks_.empty()) {
                // 没有更多任务
                executing_ = false;
                return;
            }
            
            // 获取并移除下一个任务
            next_task = std::move(tasks_.front());
            tasks_.pop_front();
        }
        
        // 设置线程标记
        thread_strand_guard guard(*this);
        
        // 执行任务
        next_task();
        
        // 递归处理下一个任务
        run_next_task();
    }
    
    uasio::io_context& io_context_;
    std::mutex mutex_;
    std::deque<std::function<void()>> tasks_;
    bool executing_;
    std::atomic<bool> current_thread_in_strand_{false};
};

//------------------ 公共方法实现 ------------------

inline strand::strand(uasio::io_context& io_context)
    : impl_(std::make_shared<strand_impl>(io_context))
{
}

inline uasio::io_context& strand::get_io_context() noexcept
{
    return impl_->get_io_context();
}

inline bool strand::running_in_this_thread() const noexcept
{
    return impl_->running_in_this_thread();
}

template <typename Function>
void strand::execute(Function&& f) const
{
    impl_->execute(std::forward<Function>(f));
}

template <typename Function>
void strand::dispatch(Function&& f) const
{
    impl_->dispatch(std::forward<Function>(f));
}

template <typename Function>
void strand::post(Function&& f) const
{
    impl_->post(std::forward<Function>(f));
}

} // namespace uasio

#endif // UASIO_STRAND_H 