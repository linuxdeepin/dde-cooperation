// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


#ifndef BASEKIT_MEMORY_MEMORY_LEAKS_DEBUG_H
#define BASEKIT_MEMORY_MEMORY_LEAKS_DEBUG_H

#if defined(__clang__)
#pragma clang system_header
#elif defined(__GNUC__)
#pragma GCC system_header
#elif defined(_MSC_VER)
#pragma system_header
#endif

//! @cond INTERNALS
#if defined(_MSC_VER)
#include <vld.h>
#endif
//! @endcond

#endif // BASEKIT_MEMORY_MEMORY_LEAKS_DEBUG_H
