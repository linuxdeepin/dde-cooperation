// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


#ifndef BASEKIT_ERRORS_FATAL_H
#define BASEKIT_ERRORS_FATAL_H

#include "errors/system_error.h"
#include "system/source_location.h"
#include "system/stack_trace.h"

#include <string>

//! Fatal abort execution extended macro
/*!
    Fatal abort execution with the current location.
*/
#define fatality(...) BaseKit::fatal(__LOCATION__, __STACK__, __VA_ARGS__)

namespace BaseKit {

//! Fatal abort execution
/*!
    Fatal aborts execution. It will print fatal message into the std::cerr stream,
    get and print the last system error code and message, stack trace if available.
    Finally abort() function will be called!

    Thread-safe.

    \param location - Source location
    \param trace - Stack trace
    \param message - Fatal message
    \param error - System error code
*/
void fatal(const SourceLocation& location, const StackTrace& trace, const std::string& message, int error = SystemError::GetLast()) noexcept;
//! Fatal abort execution
/*!
    Fatal aborts execution. It will print fatal exception into the std::cerr stream,
    get and print the last system error code and message, stack trace if available.
    Finally abort() function will be called!

    Thread-safe.

    \param location - Source location
    \param trace - Stack trace
    \param fatal - Fatal exception
*/
void fatal(const SourceLocation& location, const StackTrace& trace, const std::exception& fatal) noexcept;


} // namespace BaseKit

#endif // BASEKIT_ERRORS_FATAL_H
