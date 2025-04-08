// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logging/appenders/syslog_appender.h"

#if defined(unix) || defined(__unix) || defined(__unix__)
#include <syslog.h>
#endif

namespace Logging {

SyslogAppender::SyslogAppender()
{
#if defined(unix) || defined(__unix) || defined(__unix__)
    openlog(nullptr, LOG_NDELAY | LOG_PID, LOG_USER);
#endif
}

SyslogAppender::~SyslogAppender()
{
#if defined(unix) || defined(__unix) || defined(__unix__)
    closelog();
#endif
}

void SyslogAppender::AppendRecord(Record& record)
{
    // Skip logging records without layout
    if (record.raw.empty())
        return;

#if defined(unix) || defined(__unix) || defined(__unix__)
    // Setup syslog priority depends on the logging level
    int priority;
    switch (record.level)
    {
        case Level::FATAL:
            priority = LOG_CRIT;
            break;
        case Level::ERROR:
            priority = LOG_ERR;
            break;
        case Level::WARN:
            priority = LOG_WARNING;
            break;
        case Level::INFO:
            priority = LOG_INFO;
            break;
        case Level::DEBUG:
            priority = LOG_DEBUG;
            break;
        default:
            priority = LOG_INFO;
            break;
    }

    // Append logging record content
    syslog(priority, "%.*s", (int)record.raw.size() - 1, (char*)record.raw.data());
#endif
}

} // namespace Logging
