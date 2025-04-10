#ifndef UASIO_THREAD_POOL_H
#define UASIO_THREAD_POOL_H

#include "execution_context.h"
#include "io_context.h"
#include <memory>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>

namespace uasio {

/**
 * @brief 线程池类，管理工作线程组
 */
class thread_pool : public execution_context {
public:
    /// 构造函数
    explicit thread_pool(std::size_t num_threads = std::thread::hardware_concurrency());

    /// 析构函数
    virtual ~thread_pool();

    /// 不允许复制
    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    /// 获取工作线程数量
    std::size_t thread_count() const noexcept;

    /// 获取底层IO上下文
    io_context& get_io_context();

    /// 运行线程池
    void run();

    /// 停止线程池
    void stop();

    /// 等待所有线程完成
    void join();

    /// 在线程池中执行任务
    template <typename Function>
    void post(Function&& func) {
        io_ctx_.post(std::forward<Function>(func));
    }

    /// 在线程池中调度任务
    template <typename Function>
    void dispatch(Function&& func) {
        io_ctx_.dispatch(std::forward<Function>(func));
    }

private:
    /// IO上下文
    io_context io_ctx_;

    /// 工作守卫，防止IO上下文在没有工作时退出
    std::unique_ptr<io_context::work> work_;

    /// 工作线程数组
    std::vector<std::thread> threads_;

    /// 线程池状态
    mutable std::mutex mutex_;
    std::atomic<bool> stopped_;
    std::atomic<bool> joined_;
};

} // namespace uasio

#endif // UASIO_THREAD_POOL_H 