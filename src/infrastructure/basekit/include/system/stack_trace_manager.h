// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


#ifndef BASEKIT_SYSTEM_STACK_TRACE_MANAGER_H
#define BASEKIT_SYSTEM_STACK_TRACE_MANAGER_H

#include "errors/exceptions.h"
#include "utility/singleton.h"

#include <memory>

namespace BaseKit {

//! Stack trace manager
/*!
    Provides interface to initialize and cleanup stack trace snapshots capturing.

    Not thread-safe.
*/
class StackTraceManager : public BaseKit::Singleton<StackTraceManager>
{
   friend Singleton<StackTraceManager>;

public:
    StackTraceManager(const StackTraceManager&) = delete;
    StackTraceManager(StackTraceManager&&) = delete;
    ~StackTraceManager();

    StackTraceManager& operator=(const StackTraceManager&) = delete;
    StackTraceManager& operator=(StackTraceManager&&) = delete;

    //! Initialize stack trace manager
    /*!
        This method should be called before you start capture any stack trace snapshots.
        It is recommended to call the method just after the current process start!
    */
    static void Initialize();
    //! Cleanup stack trace manager
    /*!
        This method should be called just before the current process exits!
    */
    static void Cleanup();

private:
    class Impl;

    Impl& impl() noexcept { return reinterpret_cast<Impl&>(_storage); }
    const Impl& impl() const noexcept { return reinterpret_cast<Impl const&>(_storage); }

    static const size_t StorageSize = 4;
    static const size_t StorageAlign = 1;
    alignas(StorageAlign) std::byte _storage[StorageSize];

    StackTraceManager();
};

} // namespace BaseKit

#endif // BASEKIT_SYSTEM_STACK_TRACE_MANAGER_H
