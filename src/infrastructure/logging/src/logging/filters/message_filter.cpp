// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logging/filters/message_filter.h"

namespace Logging {

bool MessageFilter::FilterRecord(Record& record)
{
    bool result = std::regex_match(record.message, _pattern);
    return _positive ? result : !result;
}

} // namespace Logging
