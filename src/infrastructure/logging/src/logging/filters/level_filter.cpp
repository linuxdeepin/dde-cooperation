// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logging/filters/level_filter.h"

namespace Logging {

void LevelFilter::Update(Level level, bool positive)
{
    _positive = positive;
    _from = Level::NONE;
    _to = level;
}

void LevelFilter::Update(Level from, Level to, bool positive)
{
    _positive = positive;
    if (from <= to)
    {
        _from = from;
        _to = to;
    }
    else
    {
        _from = to;
        _to = from;
    }
}

bool LevelFilter::FilterRecord(Record& record)
{
    if (_positive)
        return ((record.level >= _from) && (record.level <= _to));
    else
        return ((record.level < _from) || (record.level > _to));
}

} // namespace Logging
