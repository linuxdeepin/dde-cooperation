// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logging/appenders/error_appender.h"

#include <cstdio>

namespace Logging {

void ErrorAppender::AppendRecord(Record& record)
{
    // Skip logging records without layout
    if (record.raw.empty())
        return;

    // Append logging record content
    std::fwrite(record.raw.data(), 1, record.raw.size() - 1, stderr);
}

void ErrorAppender::Flush()
{
    // Flush stream
    std::fflush(stderr);
}

} // namespace Logging
