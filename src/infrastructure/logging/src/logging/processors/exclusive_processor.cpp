// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logging/processors/exclusive_processor.h"

namespace Logging {

bool ExclusiveProcessor::ProcessRecord(Record& record)
{
    // Check if the logging processor started
    if (!IsStarted())
        return true;

    // Filter the given logging record
    if (!FilterRecord(record))
        return true;

    // Layout the given logging record
    if (_layout && _layout->IsStarted())
        _layout->LayoutRecord(record);

    // Append the given logging record
    for (auto& appender : _appenders)
        if (appender && appender->IsStarted())
            appender->AppendRecord(record);

    // Process the given logging record with sub processors
    for (auto& processor : _processors)
        if (processor && processor->IsStarted() && !processor->ProcessRecord(record))
            return false;

    // Logging record was exclusively processed!
    return false;
}

} // namespace Logging
