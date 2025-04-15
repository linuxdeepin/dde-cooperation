// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_APPENDER_H
#define LOGGING_APPENDER_H

#include "logging/element.h"
#include "logging/record.h"

namespace Logging {

//! Logging appender interface
/*!
    Logging appender takes an instance of a single logging record
    and store it content in some storage or show it in console.

    \see NullAppender
    \see ConsoleAppender
    \see DebugAppender
    \see ErrorAppender
    \see MemoryAppender
    \see OstreamAppender
    \see FileAppender
    \see RollingFileAppender
    \see SysLogAppender
*/
class Appender : public Element
{
public:
    //! Append the given logging record
    /*!
         \param record - Logging record
    */
    virtual void AppendRecord(Record& record) = 0;

    //! Flush the logging appender
    virtual void Flush() {}
};

} // namespace Logging

#endif // LOGGING_APPENDER_H
