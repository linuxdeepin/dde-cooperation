// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_APPENDERS_DEBUG_APPENDER_H
#define LOGGING_APPENDERS_DEBUG_APPENDER_H

#include "logging/appender.h"

namespace Logging {

//! Debug appender
/*!
    Debug appender prints the given logging record into
    the attached debugger if present for Windows system.
    Under Unix systems this appender prints the given
    logging record into the system error stream (stderr).

    Thread-safe.
*/
class DebugAppender : public Appender
{
public:
    DebugAppender() = default;
    DebugAppender(const DebugAppender&) = delete;
    DebugAppender(DebugAppender&&) = delete;
    virtual ~DebugAppender() = default;

    DebugAppender& operator=(const DebugAppender&) = delete;
    DebugAppender& operator=(DebugAppender&&) = delete;

    // Implementation of Appender
    void AppendRecord(Record& record) override;
    void Flush() override;
};

} // namespace Logging

#endif // LOGGING_APPENDERS_DEBUG_APPENDER_H
