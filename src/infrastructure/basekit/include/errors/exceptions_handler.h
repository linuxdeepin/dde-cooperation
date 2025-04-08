// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


#ifndef BASEKIT_ERRORS_EXCEPTIONS_HANDLER_H
#define BASEKIT_ERRORS_EXCEPTIONS_HANDLER_H

#include "filesystem/exceptions.h"
#include "system/stack_trace.h"
#include "utility/singleton.h"

#include <cassert>
#include <functional>
#include <memory>

namespace BaseKit {

//! Exceptions handler
class ExceptionsHandler : public BaseKit::Singleton<ExceptionsHandler>
{
   friend Singleton<ExceptionsHandler>;

public:
    ExceptionsHandler(const ExceptionsHandler&) = delete;
    ExceptionsHandler(ExceptionsHandler&&) = delete;
    ~ExceptionsHandler();

    ExceptionsHandler& operator=(const ExceptionsHandler&) = delete;
    ExceptionsHandler& operator=(ExceptionsHandler&&) = delete;

    //! Setup new global exceptions handler function
    /*!
        This method should be called once for the current process.
        It is recommended to call the method just after the current process start!

        \param handler - Exceptions handler function
    */
    static void SetupHandler(const std::function<void (const SystemException&, const StackTrace&)>& handler);
    //! Setup exceptions handler for the current process
    /*!
        This method should be called once for the current process.
        It is recommended to call the method just after the current process start!
    */
    static void SetupProcess();
    //! Setup exceptions handler for the current thread
    /*!
        This method should be called once for the current thread.
        It is recommended to call the method just after the current thread start!
    */
    static void SetupThread();

private:
    class Impl;

    Impl& impl() noexcept { return reinterpret_cast<Impl&>(_storage); }
    const Impl& impl() const noexcept { return reinterpret_cast<Impl const&>(_storage); }

    static const size_t StorageSize = 72;
#if defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
    static const size_t StorageAlign = 16;
#else
    static const size_t StorageAlign = 8;
#endif
    alignas(StorageAlign) std::byte _storage[StorageSize];

    ExceptionsHandler();
};


} // namespace BaseKit

#endif // BASEKIT_ERRORS_EXCEPTIONS_HANDLER_H
