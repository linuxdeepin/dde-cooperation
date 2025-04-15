// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logging/processors/sync_processor.h"

namespace Logging {

bool SyncProcessor::ProcessRecord(Record& record)
{
    BaseKit::Locker<BaseKit::CriticalSection> locker(_lock);

    // Process the given logging record under the critical section lock
    return Processor::ProcessRecord(record);
}

void SyncProcessor::Flush()
{
    BaseKit::Locker<BaseKit::CriticalSection> locker(_lock);

    // Flush under the critical section lock
    return Processor::Flush();
}

} // namespace Logging
