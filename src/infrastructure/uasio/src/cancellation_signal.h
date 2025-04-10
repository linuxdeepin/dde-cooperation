#ifndef UASIO_CANCELLATION_SIGNAL_H
#define UASIO_CANCELLATION_SIGNAL_H

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>

namespace uasio {

/// 取消操作的类型枚举
enum class cancellation_type {
    /// 不执行取消操作
    none = 0,
    
    /// 终止操作
    terminal = 1,
    
    /// 部分操作
    partial = 2,
    
    /// 终止和部分操作都执行
    total = terminal | partial
};

/// 是否包含指定的取消类型
constexpr bool contains_type(cancellation_type overall, cancellation_type check) {
    return (static_cast<int>(overall) & static_cast<int>(check)) != 0;
}

// 前向声明
class cancellation_slot;
class cancellation_signal;

/**
 * @brief 取消操作的发起者
 */
class cancellation_signal {
public:
    /// 构造函数
    cancellation_signal();
    
    /// 析构函数
    ~cancellation_signal() = default;

    /// 不允许复制
    cancellation_signal(const cancellation_signal&) = delete;
    cancellation_signal& operator=(const cancellation_signal&) = delete;

    /// 允许移动
    cancellation_signal(cancellation_signal&&) = default;
    cancellation_signal& operator=(cancellation_signal&&) = default;

    /// 发出取消信号
    void emit(cancellation_type type);

    /// 创建一个取消处理器槽
    cancellation_slot slot();

private:
    struct state {
        std::mutex mutex_;
        std::vector<cancellation_slot> slots_;
        cancellation_type type_ = cancellation_type::none;
    };
    
    std::shared_ptr<state> state_;
    
    friend class cancellation_slot;
};

/**
 * @brief 取消操作的处理器
 */
class cancellation_slot {
public:
    /// 默认构造函数创建一个未连接的槽
    cancellation_slot() noexcept;

    /// 检查槽是否已连接
    bool connected() const noexcept;

    /// 分配一个处理器函数
    template <typename Handler>
    void assign(Handler&& handler) {
        if (connected()) {
            auto* raw_handler = new auto(std::forward<Handler>(handler));
            handler_ = [raw_handler](cancellation_type type) {
                (*raw_handler)(type);
                delete raw_handler;
            };
        }
    }

    /// 清除处理器函数
    void clear();

private:
    std::shared_ptr<cancellation_signal::state> state_;
    std::function<void(cancellation_type)> handler_;
    int index_;

    // 调用处理器
    void call(cancellation_type type) {
        if (handler_) {
            handler_(type);
        }
    }

    friend class cancellation_signal;
};

// 实现
inline cancellation_signal::cancellation_signal()
    : state_(std::make_shared<state>()) {}

inline void cancellation_signal::emit(cancellation_type type) {
    std::unique_lock<std::mutex> lock(state_->mutex_);
    
    // 检查是否已经发出了这个类型的信号
    if (contains_type(state_->type_, type)) {
        return;
    }
    
    // 更新信号类型
    state_->type_ = static_cast<cancellation_type>(
        static_cast<int>(state_->type_) | static_cast<int>(type));
    
    // 获取所有处理器的副本
    auto handlers = state_->slots_;
    lock.unlock();
    
    // 调用所有处理器
    for (auto& handler : handlers) {
        if (handler.connected()) {
            handler.call(type);
        }
    }
}

inline cancellation_slot cancellation_signal::slot() {
    std::lock_guard<std::mutex> lock(state_->mutex_);
    
    cancellation_slot slot;
    slot.state_ = state_;
    slot.index_ = static_cast<int>(state_->slots_.size());
    state_->slots_.push_back(slot);
    
    return slot;
}

inline cancellation_slot::cancellation_slot() noexcept
    : state_(), handler_(), index_(-1) {}

inline bool cancellation_slot::connected() const noexcept {
    return state_ != nullptr && index_ >= 0;
}

inline void cancellation_slot::clear() {
    if (connected()) {
        std::unique_lock<std::mutex> lock(state_->mutex_);
        handler_ = nullptr;
    }
}

} // namespace uasio

#endif // UASIO_CANCELLATION_SIGNAL_H 