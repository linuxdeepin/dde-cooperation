// SPDX-FileCopyrightText: 2018 - 2023 Deepin Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef UTILS_PTR_H
#define UTILS_PTR_H

#include <memory>

template <typename T, typename D>
std::unique_ptr<T, D> make_handle(T *handle, D deleter) {
    return std::unique_ptr<T, D>{handle, deleter};
}

#endif // !UTILS_PTR_H
