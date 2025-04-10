// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "io_context.h"
#include "detail/io_context_impl.h"
#include <system_error>
#include <vector>
#include <cstring>

#ifndef _WIN32
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>

namespace asio {
namespace detail {

// Linux epoll 实现
class epoll_reactor {
public:
    epoll_reactor()
        : epoll_fd_(::epoll_create1(EPOLL_CLOEXEC)),
          shutdown_(false)
    {
        if (epoll_fd_ < 0) {
            throw std::system_error(
                std::error_code(errno, std::system_category()),
                "epoll_create failed");
        }
        
        // 创建用于唤醒的管道
        if (::pipe2(wake_fd_, O_NONBLOCK | O_CLOEXEC) < 0) {
            ::close(epoll_fd_);
            throw std::system_error(
                std::error_code(errno, std::system_category()),
                "pipe creation failed");
        }
        
        // 将读端添加到 epoll 中
        epoll_event ev = {};
        ev.events = EPOLLIN;
        ev.data.ptr = nullptr; // 特殊标记，表示唤醒事件
        if (::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, wake_fd_[0], &ev) < 0) {
            ::close(wake_fd_[0]);
            ::close(wake_fd_[1]);
            ::close(epoll_fd_);
            throw std::system_error(
                std::error_code(errno, std::system_category()),
                "epoll_ctl failed");
        }
    }
    
    ~epoll_reactor() {
        ::close(wake_fd_[0]);
        ::close(wake_fd_[1]);
        ::close(epoll_fd_);
    }
    
    // 运行一次事件循环
    void run_one(io_context_impl& ctx) {
        if (shutdown_)
            return;
        
        epoll_event events[128];
        int num_events = ::epoll_wait(epoll_fd_, events, 128, -1);
        
        if (num_events < 0) {
            if (errno == EINTR)
                return; // 被信号中断，重试
                
            throw std::system_error(
                std::error_code(errno, std::system_category()),
                "epoll_wait failed");
        }
        
        for (int i = 0; i < num_events; ++i) {
            void* ptr = events[i].data.ptr;
            
            if (ptr == nullptr) {
                // 唤醒事件，消费数据
                char buffer[128];
                while (::read(wake_fd_[0], buffer, sizeof(buffer)) > 0)
                    ; // 清空管道
                continue;
            }
            
            // 调用对应的处理函数
            ctx.process_io_event(ptr, events[i].events);
        }
    }
    
    // 唤醒事件循环
    void wake_up() {
        if (!shutdown_) {
            char byte = 0;
            ::write(wake_fd_[1], &byte, 1);
        }
    }
    
    // 关闭
    void shutdown() {
        shutdown_ = true;
        wake_up();
    }
    
    // 注册描述符
    int register_descriptor(int fd, void* data, uint32_t events) {
        epoll_event ev = {};
        ev.events = events;
        ev.data.ptr = data;
        
        return ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
    }
    
    // 修改描述符
    int modify_descriptor(int fd, void* data, uint32_t events) {
        epoll_event ev = {};
        ev.events = events;
        ev.data.ptr = data;
        
        return ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
    }
    
    // 注销描述符
    int deregister_descriptor(int fd) {
        epoll_event ev = {}; // Linux kernel 2.6.9+ 忽略这个参数，但仍需提供
        return ::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &ev);
    }
    
private:
    int epoll_fd_;
    int wake_fd_[2]; // 唤醒管道 [0] 读端，[1] 写端
    bool shutdown_;
};

// 在 io_context_impl 中使用 epoll_reactor
void io_context_impl::init_reactor() {
    reactor_ = new epoll_reactor();
}

void io_context_impl::destroy_reactor() {
    if (reactor_) {
        delete static_cast<epoll_reactor*>(reactor_);
        reactor_ = nullptr;
    }
}

void io_context_impl::run_reactor() {
    if (reactor_) {
        static_cast<epoll_reactor*>(reactor_)->run_one(*this);
    }
}

void io_context_impl::wake_reactor() {
    if (reactor_) {
        static_cast<epoll_reactor*>(reactor_)->wake_up();
    }
}

void io_context_impl::shutdown_reactor() {
    if (reactor_) {
        static_cast<epoll_reactor*>(reactor_)->shutdown();
    }
}

// 注册描述符
int io_context_impl::register_descriptor(int fd, void* data, uint32_t events) {
    if (reactor_) {
        return static_cast<epoll_reactor*>(reactor_)->register_descriptor(fd, data, events);
    }
    return -1;
}

// 修改描述符
int io_context_impl::modify_descriptor(int fd, void* data, uint32_t events) {
    if (reactor_) {
        return static_cast<epoll_reactor*>(reactor_)->modify_descriptor(fd, data, events);
    }
    return -1;
}

// 注销描述符
int io_context_impl::deregister_descriptor(int fd) {
    if (reactor_) {
        return static_cast<epoll_reactor*>(reactor_)->deregister_descriptor(fd);
    }
    return -1;
}

} // namespace detail
} // namespace asio

#endif // !_WIN32 