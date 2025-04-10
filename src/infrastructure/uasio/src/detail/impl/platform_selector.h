// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ASIO_DETAIL_IMPL_PLATFORM_SELECTOR_H
#define ASIO_DETAIL_IMPL_PLATFORM_SELECTOR_H

#include <memory>
#include <functional>

namespace asio {
namespace detail {

// io_context 的平台相关实现接口
class io_context_impl;

class platform_selector {
public:
    static std::unique_ptr<io_context_impl> create_io_context_impl();
    
private:
    static bool is_windows();
    static bool is_android();
    static bool is_linux();
};

} // namespace detail
} // namespace asio

#endif // ASIO_DETAIL_IMPL_PLATFORM_SELECTOR_H 