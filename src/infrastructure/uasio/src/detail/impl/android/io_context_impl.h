// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ASIO_DETAIL_IMPL_ANDROID_IO_CONTEXT_IMPL_H
#define ASIO_DETAIL_IMPL_ANDROID_IO_CONTEXT_IMPL_H

#include "../../io_context_impl.h"
#include <android/log.h>
#include <atomic>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace asio {
namespace detail {
namespace android_impl {

class io_context_impl : public asio::detail::io_context_impl {
public:
    io_context_impl();
    ~io_context_impl() override;
    
    // 实现基类接口
    void run() override;
    void stop() override;
    bool stopped() const noexcept override;
    void post(std::function<void()>&& handler) override;
    
private:
    void init_looper();
    void cleanup_looper();
    
    struct android_poll_source {
        int id;
        int fd;
        int events;
        void (*process)(struct android_poll_source* ps);
        void* data;
    };
    
    int looper_fd_;
    int event_fd_;
    std::atomic<bool> stopped_;
    
    std::mutex mutex_;
    std::queue<std::function<void()>> work_queue_;
    
    std::vector<std::thread> threads_;
};

} // namespace android_impl
} // namespace detail
} // namespace asio

#endif // ASIO_DETAIL_IMPL_ANDROID_IO_CONTEXT_IMPL_H 