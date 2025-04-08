// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BASEKIT_UTILITY_COUNTOF_H
#define BASEKIT_UTILITY_COUNTOF_H

namespace BaseKit {

//! Count of elements in static array
template <typename T, size_t N>
constexpr size_t countof(const T (&)[N]) noexcept { return N; }

//! Count of elements in any other STL container
template <typename T>
size_t countof(const T& container) noexcept { return container.size(); }

} // namespace BaseKit

#endif // BASEKIT_UTILITY_COUNTOF_H
