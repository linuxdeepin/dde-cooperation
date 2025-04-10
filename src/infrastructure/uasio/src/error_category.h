// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_ERROR_CATEGORY_H
#define UASIO_ERROR_CATEGORY_H

#include <string>

namespace asio {

class error_category {
public:
    virtual ~error_category() = default;
    virtual const char* name() const noexcept = 0;
    virtual std::string message(int value) const = 0;
    
    bool operator==(const error_category& other) const noexcept {
        return this == &other;
    }
    
    bool operator!=(const error_category& other) const noexcept {
        return !(*this == other);
    }
};

} // namespace asio

#endif // UASIO_ERROR_CATEGORY_H 