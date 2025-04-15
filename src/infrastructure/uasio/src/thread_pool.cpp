#include "thread_pool.h"

namespace uasio {

thread_pool::thread_pool(std::size_t num_threads)
    : io_ctx_(static_cast<int>(num_threads)),
      work_(std::make_unique<io_context::work>(io_ctx_)),
      stopped_(false),
      joined_(false) {
    // 如果请求的线程数为0，则使用硬件支持的线程数
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
    }
}

thread_pool::~thread_pool() {
    if (!stopped_) {
        stop();
    }
    
    if (!joined_) {
        join();
    }
}

std::size_t thread_pool::thread_count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return threads_.size();
}

io_context& thread_pool::get_io_context() {
    return io_ctx_;
}

void thread_pool::run() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 如果线程池已停止，则重新启动
    if (stopped_) {
        io_ctx_.restart();
        work_ = std::make_unique<io_context::work>(io_ctx_);
        stopped_ = false;
        joined_ = false;
    }
    
    // 如果工作线程为空，则创建线程
    if (threads_.empty()) {
        // 获取线程数并启动工作线程
        const std::size_t num_threads = threads_.capacity() > 0 ? 
            threads_.capacity() : std::thread::hardware_concurrency();
        threads_.reserve(num_threads);
        
        for (std::size_t i = 0; i < num_threads; ++i) {
            threads_.emplace_back([this]() {
                // 在线程中运行IO上下文直到io_ctx_.stop()被调用或工作被取消
                io_ctx_.run();
            });
        }
    }
}

void thread_pool::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!stopped_) {
        // 取消工作守卫，允许IO上下文在没有更多工作时退出
        work_.reset();
        
        // 停止IO上下文，强制所有线程退出
        io_ctx_.stop();
        
        stopped_ = true;
    }
}

void thread_pool::join() {
    std::vector<std::thread> threads_to_join;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!joined_) {
            threads_to_join.swap(threads_);
            joined_ = true;
        }
    }
    
    // 等待所有线程完成
    for (auto& thread : threads_to_join) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

} // namespace uasio 