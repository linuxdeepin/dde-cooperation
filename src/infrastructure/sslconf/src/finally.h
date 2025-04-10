// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SSLCONF_FINALLY_H
#define SSLCONF_FINALLY_H

#include <utility>

namespace sslconf {

// this implements a common pattern of executing an action at the end of function

template<class Callable>
class final_action {
public:
    final_action() noexcept {}
    final_action(Callable callable) noexcept : callable_{callable} {}

    ~final_action() noexcept
    {
        if (!invoked_) {
            callable_();
        }
    }

    final_action(final_action&& other) noexcept :
        callable_{std::move(other.callable_)}
    {
        std::swap(invoked_, other.invoked_);
    }

    final_action(const final_action&) = delete;
    final_action& operator=(const final_action&) = delete;
private:
    bool invoked_ = false;
    Callable callable_;
};

template<class Callable>
inline final_action<Callable> finally(Callable&& callable) noexcept
{
    return final_action<Callable>(std::forward<Callable>(callable));
}

} // namespace sslconf

#endif
