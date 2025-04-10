// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_SIGNAL_SET_H
#define UASIO_SIGNAL_SET_H

#include "io_context.h"
#include "error.h"
#include <signal.h>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace uasio {

/**
 * @brief 信号集类，用于处理系统信号
 * 
 * 提供异步处理系统信号的功能，如SIGINT, SIGTERM等。
 * 支持注册多个信号并为其指定处理函数。
 */
class signal_set {
public:
    /**
     * @brief 构造函数
     * @param io_context IO上下文对象
     */
    explicit signal_set(uasio::io_context& io_context);
    
    /**
     * @brief 析构函数
     * 
     * 会自动取消所有注册的信号
     */
    ~signal_set();
    
    /**
     * @brief 添加信号到集合
     * @param signal_number 信号编号
     * @param ec 错误码
     * @return 操作是否成功
     */
    bool add(int signal_number, uasio::error_code& ec);
    
    /**
     * @brief 从集合中移除信号
     * @param signal_number 信号编号
     * @param ec 错误码
     * @return 操作是否成功
     */
    bool remove(int signal_number, uasio::error_code& ec);
    
    /**
     * @brief 清空信号集合
     * @param ec 错误码
     * @return 操作是否成功
     */
    bool clear(uasio::error_code& ec);
    
    /**
     * @brief 取消所有异步等待操作
     * @return 取消的操作数量
     */
    std::size_t cancel();
    
    /**
     * @brief 异步等待信号
     * @param handler 信号处理函数
     */
    template <typename SignalHandler>
    void async_wait(SignalHandler&& handler);
    
private:
    class signal_set_service;
    std::shared_ptr<signal_set_service> service_;
};

// 实现部分

class signal_set::signal_set_service : public std::enable_shared_from_this<signal_set_service> {
public:
    explicit signal_set_service(uasio::io_context& io_ctx)
        : io_context_(io_ctx), registered_count_(0) {}
    
    ~signal_set_service() {
        clear();
    }
    
    // 全局信号处理函数和注册表
    static std::mutex signals_mutex_;
    static std::unordered_map<int, std::vector<std::weak_ptr<signal_set_service>>> registered_services_;
    
    static void signal_handler(int signal_number) {
        std::lock_guard<std::mutex> lock(signals_mutex_);
        
        auto it = registered_services_.find(signal_number);
        if (it != registered_services_.end()) {
            for (auto service_it = it->second.begin(); service_it != it->second.end();) {
                if (auto service = service_it->lock()) {
                    service->deliver_signal(signal_number);
                    ++service_it;
                } else {
                    // 清理已失效的弱引用
                    service_it = it->second.erase(service_it);
                }
            }
        }
    }
    
    bool add(int signal_number, uasio::error_code& ec) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // 已经注册过该信号
        if (signals_.count(signal_number) > 0) {
            ec = uasio::make_error_code(1);  // 使用operation_aborted代替already_open
            return false;
        }
        
        {
            std::lock_guard<std::mutex> global_lock(signals_mutex_);
            
            // 如果是第一个注册该信号的服务，设置信号处理函数
            bool first_registration = registered_services_[signal_number].empty();
            
            if (first_registration) {
                struct sigaction sa;
                sa.sa_handler = &signal_handler;
                sigemptyset(&sa.sa_mask);
                sa.sa_flags = 0;
                
                if (sigaction(signal_number, &sa, nullptr) == -1) {
                    ec = uasio::make_error_code(errno);
                    registered_services_.erase(signal_number);
                    return false;
                }
            }
            
            // 将当前服务添加到注册表
            registered_services_[signal_number].push_back(weak_from_this());
        }
        
        signals_.insert(signal_number);
        ++registered_count_;
        
        ec = uasio::make_error_code(0);
        return true;
    }
    
    bool remove(int signal_number, uasio::error_code& ec) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // 未注册该信号
        if (signals_.count(signal_number) == 0) {
            ec = uasio::make_error_code(18);  // 使用invalid_argument代替not_found
            return false;
        }
        
        {
            std::lock_guard<std::mutex> global_lock(signals_mutex_);
            
            auto it = registered_services_.find(signal_number);
            if (it != registered_services_.end()) {
                // 移除当前服务
                auto& services = it->second;
                for (auto service_it = services.begin(); service_it != services.end();) {
                    if (auto service = service_it->lock()) {
                        if (service.get() == this) {
                            service_it = services.erase(service_it);
                        } else {
                            ++service_it;
                        }
                    } else {
                        // 清理已失效的弱引用
                        service_it = services.erase(service_it);
                    }
                }
                
                // 如果没有服务注册该信号，恢复默认处理
                if (services.empty()) {
                    struct sigaction sa;
                    sa.sa_handler = SIG_DFL;
                    sigemptyset(&sa.sa_mask);
                    sa.sa_flags = 0;
                    
                    if (sigaction(signal_number, &sa, nullptr) == -1) {
                        ec = uasio::make_error_code(errno);
                        return false;
                    }
                    
                    registered_services_.erase(signal_number);
                }
            }
        }
        
        signals_.erase(signal_number);
        --registered_count_;
        
        ec = uasio::make_error_code(0);
        return true;
    }
    
    bool clear(uasio::error_code& ec) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<int> signals_to_remove(signals_.begin(), signals_.end());
        
        bool success = true;
        for (int signal_number : signals_to_remove) {
            uasio::error_code local_ec;
            if (!remove(signal_number, local_ec)) {
                success = false;
                if (!ec) ec = local_ec;
            }
        }
        
        return success;
    }
    
    // 添加无参数版本的clear方法
    bool clear() {
        uasio::error_code ec;
        return clear(ec);
    }
    
    std::size_t cancel() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::size_t cancelled_count = handlers_.size();
        
        // 取消所有等待的处理函数，以操作取消错误调用它们
        for (auto& handler : handlers_) {
            io_context_.post([handler]() {
                handler(uasio::make_error_code(uasio::error::operation_aborted), 0);
            });
        }
        
        handlers_.clear();
        
        return cancelled_count;
    }
    
    void deliver_signal(int signal_number) {
        std::vector<std::function<void(const uasio::error_code&, int)>> handlers_to_call;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            // 复制处理函数列表，以便在不持有锁的情况下调用
            handlers_to_call = handlers_;
            handlers_.clear();
        }
        
        // 在IO上下文中调用处理函数
        for (auto& handler : handlers_to_call) {
            io_context_.post([handler, signal_number]() {
                handler(uasio::make_error_code(0), signal_number);
            });
        }
    }
    
    template <typename SignalHandler>
    void async_wait(SignalHandler&& handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (signals_.empty()) {
            // 没有注册信号，直接调用处理函数
            io_context_.post([handler]() {
                handler(uasio::make_error_code(18), 0);  // 使用invalid_argument代替not_found
            });
            return;
        }
        
        // 添加到处理函数列表
        handlers_.push_back(std::forward<SignalHandler>(handler));
    }
    
private:
    uasio::io_context& io_context_;
    std::mutex mutex_;
    std::unordered_set<int> signals_;
    std::vector<std::function<void(const uasio::error_code&, int)>> handlers_;
    std::size_t registered_count_;
};

// 静态成员初始化
std::mutex signal_set::signal_set_service::signals_mutex_;
std::unordered_map<int, std::vector<std::weak_ptr<signal_set::signal_set_service>>> 
    signal_set::signal_set_service::registered_services_;

inline signal_set::signal_set(uasio::io_context& io_context)
    : service_(std::make_shared<signal_set_service>(io_context))
{
}

inline signal_set::~signal_set()
{
    service_->clear();  // 使用无参数版本
}

inline bool signal_set::add(int signal_number, uasio::error_code& ec)
{
    return service_->add(signal_number, ec);
}

inline bool signal_set::remove(int signal_number, uasio::error_code& ec)
{
    return service_->remove(signal_number, ec);
}

inline bool signal_set::clear(uasio::error_code& ec)
{
    return service_->clear(ec);
}

inline std::size_t signal_set::cancel()
{
    return service_->cancel();
}

template <typename SignalHandler>
void signal_set::async_wait(SignalHandler&& handler)
{
    service_->async_wait(std::forward<SignalHandler>(handler));
}

} // namespace uasio

#endif // UASIO_SIGNAL_SET_H 