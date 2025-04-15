// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_EXECUTOR_H
#define UASIO_EXECUTOR_H

#include <functional>
#include <type_traits>
#include <utility>

namespace asio {

class executor {
public:
    executor() noexcept = default;
    virtual ~executor() = default;
    
    virtual void execute(std::function<void()>&& f) = 0;
    virtual bool running_in_this_thread() const noexcept = 0;
    
    // 检查函数能否安全地在当前线程执行
    template<typename Function>
    void dispatch(Function&& f) {
        // 如果在当前线程，直接执行；否则，通过 execute 提交
        if (running_in_this_thread()) {
            // 直接执行函数
            std::forward<Function>(f)();
        } else {
            // 通过 execute 提交
            execute(std::function<void()>(std::forward<Function>(f)));
        }
    }
    
    // 总是异步提交函数，不在当前线程执行
    template<typename Function>
    void post(Function&& f) {
        // 通过 execute 提交
        execute(std::function<void()>(std::forward<Function>(f)));
    }
};

} // namespace asio

#endif // UASIO_EXECUTOR_H 