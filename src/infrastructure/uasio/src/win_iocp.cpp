// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "io_context.h"
#include "detail/io_context_impl.h"
#include <system_error>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <mswsock.h>

namespace asio {
namespace detail {

// Windows IOCP 实现
class iocp_reactor {
public:
    iocp_reactor()
        : iocp_handle_(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)),
          shutdown_(false)
    {
        if (iocp_handle_ == NULL) {
            throw std::system_error(
                std::error_code(::GetLastError(), std::system_category()),
                "CreateIoCompletionPort failed");
        }
    }
    
    ~iocp_reactor() {
        if (iocp_handle_ != NULL) {
            ::CloseHandle(iocp_handle_);
        }
    }
    
    // 运行一次事件循环
    void run_one(io_context_impl& ctx) {
        if (shutdown_)
            return;
        
        DWORD bytes_transferred = 0;
        ULONG_PTR completion_key = 0;
        LPOVERLAPPED overlapped = NULL;
        
        BOOL result = ::GetQueuedCompletionStatus(
            iocp_handle_,
            &bytes_transferred,
            &completion_key,
            &overlapped,
            INFINITE);
        
        if (overlapped) {
            // 检查是否是唤醒消息
            if (completion_key == 0 && bytes_transferred == 0) {
                // 唤醒消息
                return;
            }
            
            // 处理IO事件
            ctx.process_io_event(overlapped, result ? 0 : ::GetLastError());
        }
        else if (!result) {
            // 发生错误
            DWORD error = ::GetLastError();
            if (error != WAIT_TIMEOUT) {
                throw std::system_error(
                    std::error_code(error, std::system_category()),
                    "GetQueuedCompletionStatus failed");
            }
        }
    }
    
    // 唤醒事件循环
    void wake_up() {
        if (!shutdown_) {
            ::PostQueuedCompletionStatus(
                iocp_handle_,
                0,
                0,
                NULL);
        }
    }
    
    // 关闭
    void shutdown() {
        shutdown_ = true;
        wake_up();
    }
    
    // 注册句柄
    int register_handle(HANDLE handle, void* completion_key) {
        HANDLE result = ::CreateIoCompletionPort(
            handle,
            iocp_handle_,
            reinterpret_cast<ULONG_PTR>(completion_key),
            0);
        
        return (result == iocp_handle_) ? 0 : -1;
    }
    
private:
    HANDLE iocp_handle_;
    bool shutdown_;
};

// 在 io_context_impl 中使用 iocp_reactor
void io_context_impl::init_reactor() {
    reactor_ = new iocp_reactor();
}

void io_context_impl::destroy_reactor() {
    if (reactor_) {
        delete static_cast<iocp_reactor*>(reactor_);
        reactor_ = nullptr;
    }
}

void io_context_impl::run_reactor() {
    if (reactor_) {
        static_cast<iocp_reactor*>(reactor_)->run_one(*this);
    }
}

void io_context_impl::wake_reactor() {
    if (reactor_) {
        static_cast<iocp_reactor*>(reactor_)->wake_up();
    }
}

void io_context_impl::shutdown_reactor() {
    if (reactor_) {
        static_cast<iocp_reactor*>(reactor_)->shutdown();
    }
}

// 注册描述符（Windows 使用 HANDLE）
int io_context_impl::register_descriptor(int fd, void* data, uint32_t /*events*/) {
    if (reactor_) {
        return static_cast<iocp_reactor*>(reactor_)->register_handle(
            reinterpret_cast<HANDLE>(fd), data);
    }
    return -1;
}

// Windows IOCP 不需要显式修改监听事件
int io_context_impl::modify_descriptor(int /*fd*/, void* /*data*/, uint32_t /*events*/) {
    return 0;
}

// Windows IOCP 不需要显式注销（关闭 HANDLE 即可）
int io_context_impl::deregister_descriptor(int /*fd*/) {
    return 0;
}

} // namespace detail
} // namespace asio

#endif // _WIN32 