// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_APPENDERS_OSTREAM_APPENDER_H
#define LOGGING_APPENDERS_OSTREAM_APPENDER_H

#include "logging/appender.h"

#include <iostream>

namespace Logging {

//! Output stream (std::ostream) appender
/*!
    Output stream (std::ostream) appender prints the given logging record
    into the given instance of std::ostream.

    Not thread-safe.
*/
class OstreamAppender : public Appender
{
public:
    //! Initialize the appender with a given output stream
    /*!
         \param stream - Output stream
    */
    explicit OstreamAppender(std::ostream& stream) : _ostream(stream) {}
    OstreamAppender(const OstreamAppender&) = delete;
    OstreamAppender(OstreamAppender&&) = delete;
    virtual ~OstreamAppender() = default;

    OstreamAppender& operator=(const OstreamAppender&) = delete;
    OstreamAppender& operator=(OstreamAppender&&) = delete;

    // Implementation of Appender
    void AppendRecord(Record& record) override;
    void Flush() override;

private:
    std::ostream& _ostream;
};

} // namespace Logging

#endif // LOGGING_APPENDERS_OSTREAM_APPENDER_H
