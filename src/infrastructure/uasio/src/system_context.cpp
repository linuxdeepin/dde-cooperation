#include "system_context.h"
#include <thread>

namespace uasio {

// 系统上下文单例实例
system_context& system_context::get() noexcept {
    static system_context instance;
    return instance;
}

// 构造函数
system_context::system_context()
    : ctx_(1), // 单线程IO上下文
      work_(std::make_unique<io_context::work>(ctx_)),
      stopped_(false) {
}

// 获取IO上下文
io_context& system_context::context() noexcept {
    return ctx_;
}

// 停止系统上下文
void system_context::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!stopped_) {
        stopped_ = true;
        work_.reset();
        ctx_.stop();
    }
}

// 重启系统上下文
void system_context::restart() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stopped_) {
        ctx_.restart();
        work_ = std::make_unique<io_context::work>(ctx_);
        stopped_ = false;
    }
}

} // namespace uasio 