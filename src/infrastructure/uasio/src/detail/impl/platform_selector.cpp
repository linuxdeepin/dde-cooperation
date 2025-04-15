// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "detail/impl/platform_selector.h"
#include "detail/io_context_impl.h"

#ifdef _WIN32
#include "detail/impl/windows/io_context_impl.h"
#elif defined(__ANDROID__)
#include "detail/impl/android/io_context_impl.h"
#else
#include "detail/impl/linux/io_context_impl.h"
#endif

namespace asio {
namespace detail {

std::unique_ptr<io_context_impl> platform_selector::create_io_context_impl()
{
#ifdef _WIN32
    return std::make_unique<windows_impl::io_context_impl>();
#elif defined(__ANDROID__)
    return std::make_unique<android_impl::io_context_impl>();
#else
    return std::make_unique<linux_impl::io_context_impl>();
#endif
}

bool platform_selector::is_windows()
{
#ifdef _WIN32
    return true;
#else
    return false;
#endif
}

bool platform_selector::is_android()
{
#ifdef __ANDROID__
    return true;
#else
    return false;
#endif
}

bool platform_selector::is_linux()
{
#if !defined(_WIN32) && !defined(__ANDROID__)
    return true;
#else
    return false;
#endif
}

} // namespace detail
} // namespace asio 