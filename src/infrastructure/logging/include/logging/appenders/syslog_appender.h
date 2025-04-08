// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_APPENDERS_SYSLOG_APPENDER_H
#define LOGGING_APPENDERS_SYSLOG_APPENDER_H

#include "logging/appender.h"

namespace Logging {

//! Syslog appender
/*!
    Syslog appender forward logging record to the syslog() system call
    for Unix systems. Under Windows systems this appender does nothing!

    Thread-safe.
*/
class SyslogAppender : public Appender
{
public:
    SyslogAppender();
    SyslogAppender(const SyslogAppender&) = delete;
    SyslogAppender(SyslogAppender&&) = delete;
    virtual ~SyslogAppender();

    SyslogAppender& operator=(const SyslogAppender&) = delete;
    SyslogAppender& operator=(SyslogAppender&&) = delete;

    // Implementation of Appender
    void AppendRecord(Record& record) override;
};

} // namespace Logging

/*! \example syslog.cpp Syslog logger example */

#endif // LOGGING_APPENDERS_SYSLOG_APPENDER_H
