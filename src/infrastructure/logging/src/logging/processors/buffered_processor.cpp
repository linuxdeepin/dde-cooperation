// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logging/processors/buffered_processor.h"

namespace Logging {

bool BufferedProcessor::ProcessRecord(Record& record)
{
    // Check if the logging processor started
    if (!IsStarted())
        return true;

    // Process all buffered logging records if the buffer limit is reached
    if ((_buffer.size() + 1) > _limit)
        ProcessBufferedRecords();

    // Move the given logging record into the buffer
    _buffer.emplace_back(std::move(record));

    // Always return false to stop further logging record processing
    return false;
}

void BufferedProcessor::ProcessBufferedRecords()
{
    // Process all buffered logging records
    for (auto& record : _buffer)
        Processor::ProcessRecord(record);

    // Clear buffer
    _buffer.clear();
}

void BufferedProcessor::Flush()
{
    // Check if the logging processor started
    if (!IsStarted())
        return;

    // Process all buffered logging records
    ProcessBufferedRecords();

    // Flush the logging processor
    Processor::Flush();
}

} // namespace Logging
