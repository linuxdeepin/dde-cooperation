#ifndef UASIO_SYSTEM_CONTEXT_H
#define UASIO_SYSTEM_CONTEXT_H

#include "io_context.h"
#include <memory>
#include <mutex>

namespace uasio {

/**
 * @brief 系统上下文类，提供全局执行上下文
 */
class system_context {
public:
    /// 获取系统上下文单例实例
    static system_context& get() noexcept;

    /// 获取IO上下文
    io_context& context() noexcept;

    /// 通知系统上下文停止
    void stop();

    /// 重启系统上下文
    void restart();

private:
    /// 私有构造函数，防止直接创建实例
    system_context();

    /// 禁止复制构造
    system_context(const system_context&) = delete;
    system_context& operator=(const system_context&) = delete;

    /// IO上下文
    io_context ctx_;

    /// 工作守卫，防止IO上下文在没有工作时退出
    std::unique_ptr<io_context::work> work_;

    /// 标记系统是否已停止
    bool stopped_;

    /// 保护系统上下文的互斥锁
    mutable std::mutex mutex_;
};

// 获取系统上下文的IO上下文
inline io_context& system_executor() noexcept {
    return system_context::get().context();
}

} // namespace uasio

#endif // UASIO_SYSTEM_CONTEXT_H 