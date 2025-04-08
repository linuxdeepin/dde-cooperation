// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logging/filters/logger_filter.h"

namespace Logging {

bool LoggerFilter::FilterRecord(Record& record)
{
    bool result = (_pattern.compare(record.logger) == 0);
    return _positive ? result : !result;
}

} // namespace Logging
