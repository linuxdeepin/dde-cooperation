// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "detail/impl/android/io_context_impl.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>

namespace asio {
namespace detail {
namespace android_impl {

io_context_impl::io_context_impl()
    : looper_fd_(-1)
    , event_fd_(-1)
    , stopped_(false)
{
    init_looper();
}

io_context_impl::~io_context_impl()
{
    stop();
    cleanup_looper();
}

void io_context_impl::init_looper()
{
    looper_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (looper_fd_ == -1) {
        __android_log_print(ANDROID_LOG_ERROR, "ASIO", "epoll_create1 failed: %s", strerror(errno));
        throw std::system_error(errno, std::system_category(), "epoll_create1 failed");
    }
    
    event_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (event_fd_ == -1) {
        close(looper_fd_);
        __android_log_print(ANDROID_LOG_ERROR, "ASIO", "eventfd failed: %s", strerror(errno));
        throw std::system_error(errno, std::system_category(), "eventfd failed");
    }
    
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = event_fd_;
    
    if (epoll_ctl(looper_fd_, EPOLL_CTL_ADD, event_fd_, &ev) == -1) {
        close(event_fd_);
        close(looper_fd_);
        __android_log_print(ANDROID_LOG_ERROR, "ASIO", "epoll_ctl failed: %s", strerror(errno));
        throw std::system_error(errno, std::system_category(), "epoll_ctl failed");
    }
}

void io_context_impl::cleanup_looper()
{
    if (event_fd_ != -1) {
        close(event_fd_);
        event_fd_ = -1;
    }
    if (looper_fd_ != -1) {
        close(looper_fd_);
        looper_fd_ = -1;
    }
}

void io_context_impl::run()
{
    const int max_events = 64;
    struct epoll_event events[max_events];
    
    while (!stopped_) {
        int nfds = epoll_wait(looper_fd_, events, max_events, -1);
        if (nfds == -1) {
            if (errno == EINTR) {
                continue;
            }
            __android_log_print(ANDROID_LOG_ERROR, "ASIO", "epoll_wait failed: %s", strerror(errno));
            throw std::system_error(errno, std::system_category(), "epoll_wait failed");
        }
        
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == event_fd_) {
                uint64_t value;
                if (read(event_fd_, &value, sizeof(value)) == -1) {
                    if (errno != EAGAIN) {
                        __android_log_print(ANDROID_LOG_ERROR, "ASIO", "read event_fd failed: %s", strerror(errno));
                        throw std::system_error(errno, std::system_category(), "read event_fd failed");
                    }
                }
                
                std::function<void()> handler;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    if (!work_queue_.empty()) {
                        handler = std::move(work_queue_.front());
                        work_queue_.pop();
                    }
                }
                
                if (handler) {
                    handler();
                }
            }
        }
    }
}

void io_context_impl::stop()
{
    stopped_ = true;
    uint64_t value = 1;
    if (write(event_fd_, &value, sizeof(value)) == -1) {
        __android_log_print(ANDROID_LOG_ERROR, "ASIO", "write event_fd failed: %s", strerror(errno));
        throw std::system_error(errno, std::system_category(), "write event_fd failed");
    }
}

bool io_context_impl::stopped() const noexcept
{
    return stopped_;
}

void io_context_impl::post(std::function<void()>&& handler)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        work_queue_.push(std::move(handler));
    }
    
    uint64_t value = 1;
    if (write(event_fd_, &value, sizeof(value)) == -1) {
        __android_log_print(ANDROID_LOG_ERROR, "ASIO", "write event_fd failed: %s", strerror(errno));
        throw std::system_error(errno, std::system_category(), "write event_fd failed");
    }
}

} // namespace android_impl
} // namespace detail
} // namespace asio 