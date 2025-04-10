// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_DETAIL_IO_CONTEXT_IMPL_H
#define UASIO_DETAIL_IO_CONTEXT_IMPL_H

#include <functional>
#include <cstdint>

namespace asio {
namespace detail {

// io_context 的平台相关实现接口
class io_context_impl {
public:
    io_context_impl() : reactor_(nullptr) {}
    
    virtual ~io_context_impl() {
        destroy_reactor();
    }
    
    // 运行事件循环
    virtual void run() = 0;
    
    // 停止事件循环
    virtual void stop() = 0;
    
    // 检查是否已停止
    virtual bool stopped() const noexcept = 0;
    
    // 提交任务
    virtual void post(std::function<void()>&& handler) = 0;
    
    // 处理IO事件
    virtual void process_io_event(void* data, uint32_t events) = 0;
    
    // 反应器相关方法
    void init_reactor();
    void destroy_reactor();
    void run_reactor();
    void wake_reactor();
    void shutdown_reactor();
    
    // 描述符管理方法
    int register_descriptor(int fd, void* data, uint32_t events);
    int modify_descriptor(int fd, void* data, uint32_t events);
    int deregister_descriptor(int fd);
    
protected:
    void* reactor_; // 平台相关的反应器实现
};

} // namespace detail
} // namespace asio

#endif // UASIO_DETAIL_IO_CONTEXT_IMPL_H 