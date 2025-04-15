// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_APPENDERS_CONSOLE_APPENDER_H
#define LOGGING_APPENDERS_CONSOLE_APPENDER_H

#include "logging/appender.h"

namespace Logging {

//! Console (stdout) appender
/*!
    Console appender prints the given logging record into
    the console or system output stream (stdout).

    Thread-safe.
*/
class ConsoleAppender : public Appender
{
public:
    ConsoleAppender() = default;
    ConsoleAppender(const ConsoleAppender&) = delete;
    ConsoleAppender(ConsoleAppender&&) = delete;
    virtual ~ConsoleAppender() = default;

    ConsoleAppender& operator=(const ConsoleAppender&) = delete;
    ConsoleAppender& operator=(ConsoleAppender&&) = delete;

    // Implementation of Appender
    void AppendRecord(Record& record) override;
    void Flush() override;
};

} // namespace Logging

/*! \example console.cpp Console logger example */

#endif // LOGGING_APPENDERS_CONSOLE_APPENDER_H
