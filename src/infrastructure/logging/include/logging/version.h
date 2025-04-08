// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_VERSION_H
#define LOGGING_VERSION_H

/*! \mainpage C++ Logging Library

C++ Logging Library provides functionality to log different events with a high
throughput in multi-thread environment into different sinks (console, files,
rolling files, syslog, etc.). Logging configuration is very flexible and gives
functionality to build flexible logger hierarchy with combination of logging
processors (sync, async), filters, layouts (binary, text) and appenders.

This document contains CppLogging API references.

Library description, features, requirements and usage examples can be  find  on
GitHub: https://github.com/chronoxor/CppLogging

*/

/*!
    \namespace Logging
    \brief C++ Logging project definitions
*/
namespace Logging {

//! Project version
const char version[] = "1.0.4.0";

} // namespace Logging

#endif // LOGGING_VERSION_H
