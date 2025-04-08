// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logging/filters/switch_filter.h"

namespace Logging {

void SwitchFilter::Update(bool enabled)
{
    _enabled = enabled;
}

bool SwitchFilter::FilterRecord(Record& record)
{
    return _enabled;
}

} // namespace Logging
