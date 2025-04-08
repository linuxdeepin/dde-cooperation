// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logging/appenders/ostream_appender.h"

namespace Logging {

void OstreamAppender::AppendRecord(Record& record)
{
    // Skip logging records without layout
    if (record.raw.empty())
        return;

    // Append logging record content
    _ostream.write((char*)record.raw.data(), record.raw.size() - 1);
}

void OstreamAppender::Flush()
{
    // Flush stream
    _ostream.flush();
}

} // namespace Logging
