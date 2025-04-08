// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_APPENDERS_ERROR_APPENDER_H
#define LOGGING_APPENDERS_ERROR_APPENDER_H

#include "logging/appender.h"

namespace Logging {

//! Error (stderr) appender
/*!
    Error appender prints the given logging record into
    the console or system error stream (stderr).

    Thread-safe.
*/
class ErrorAppender : public Appender
{
public:
    ErrorAppender() = default;
    ErrorAppender(const ErrorAppender&) = delete;
    ErrorAppender(ErrorAppender&&) = delete;
    virtual ~ErrorAppender() = default;

    ErrorAppender& operator=(const ErrorAppender&) = delete;
    ErrorAppender& operator=(ErrorAppender&&) = delete;

    // Implementation of Appender
    void AppendRecord(Record& record) override;
    void Flush() override;
};

} // namespace Logging

#endif // LOGGING_APPENDERS_ERROR_APPENDER_H
