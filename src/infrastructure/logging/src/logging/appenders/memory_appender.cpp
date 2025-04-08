// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logging/appenders/memory_appender.h"

namespace Logging {

void MemoryAppender::AppendRecord(Record& record)
{
    // Skip logging records without layout
    if (record.raw.empty())
        return;

    // Append logging record content into the allocated memory buffer
    _buffer.insert(_buffer.end(), record.raw.begin(), record.raw.end());
}

} // namespace Logging
