// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_APPENDERS_NULL_APPENDER_H
#define LOGGING_APPENDERS_NULL_APPENDER_H

#include "logging/appender.h"

namespace Logging {

//! Null appender
/*!
    Null appender does nothing with a given logging record.

    Thread-safe.
*/
class NullAppender : public Appender
{
public:
    NullAppender() = default;
    NullAppender(const NullAppender&) = delete;
    NullAppender(NullAppender&&) = delete;
    virtual ~NullAppender() = default;

    NullAppender& operator=(const NullAppender&) = delete;
    NullAppender& operator=(NullAppender&&) = delete;

    // Implementation of Appender
    void AppendRecord(Record& record) override {}
};

} // namespace Logging

#endif // LOGGING_APPENDERS_NULL_APPENDER_H
