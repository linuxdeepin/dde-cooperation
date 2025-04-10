// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "detail/impl/windows/io_context_impl.h"
#include <system_error>

namespace asio {
namespace detail {
namespace windows_impl {

io_context_impl::io_context_impl()
    : iocp_(INVALID_HANDLE_VALUE)
    , event_(INVALID_HANDLE_VALUE)
    , stopped_(false)
{
    init_iocp();
}

io_context_impl::~io_context_impl()
{
    stop();
    cleanup_iocp();
}

void io_context_impl::init_iocp()
{
    iocp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (iocp_ == INVALID_HANDLE_VALUE) {
        throw std::system_error(GetLastError(), std::system_category(), "CreateIoCompletionPort failed");
    }
    
    event_ = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (event_ == INVALID_HANDLE_VALUE) {
        CloseHandle(iocp_);
        throw std::system_error(GetLastError(), std::system_category(), "CreateEvent failed");
    }
}

void io_context_impl::cleanup_iocp()
{
    if (event_ != INVALID_HANDLE_VALUE) {
        CloseHandle(event_);
        event_ = INVALID_HANDLE_VALUE;
    }
    if (iocp_ != INVALID_HANDLE_VALUE) {
        CloseHandle(iocp_);
        iocp_ = INVALID_HANDLE_VALUE;
    }
}

void io_context_impl::run()
{
    while (!stopped_) {
        DWORD bytes_transferred;
        ULONG_PTR completion_key;
        LPOVERLAPPED overlapped;
        
        BOOL result = GetQueuedCompletionStatus(
            iocp_,
            &bytes_transferred,
            &completion_key,
            &overlapped,
            INFINITE);
            
        if (!result) {
            DWORD error = GetLastError();
            if (error == WAIT_TIMEOUT) {
                continue;
            }
            throw std::system_error(error, std::system_category(), "GetQueuedCompletionStatus failed");
        }
        
        if (completion_key == 0) { // 特殊标记，表示有新的工作项
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

void io_context_impl::stop()
{
    stopped_ = true;
    if (!PostQueuedCompletionStatus(iocp_, 0, 0, nullptr)) {
        throw std::system_error(GetLastError(), std::system_category(), "PostQueuedCompletionStatus failed");
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
    
    if (!PostQueuedCompletionStatus(iocp_, 0, 0, nullptr)) {
        throw std::system_error(GetLastError(), std::system_category(), "PostQueuedCompletionStatus failed");
    }
}

} // namespace windows_impl
} // namespace detail
} // namespace asio 