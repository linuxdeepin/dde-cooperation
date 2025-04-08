// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace Logging {

template <class TOutputStream>
inline TOutputStream& operator<<(TOutputStream& stream, TimeRollingPolicy policy)
{
    switch (policy)
    {
        case TimeRollingPolicy::YEAR:
            stream << "Year";
            break;
        case TimeRollingPolicy::MONTH:
            stream << "Month";
            break;
        case TimeRollingPolicy::DAY:
            stream << "Day";
            break;
        case TimeRollingPolicy::HOUR:
            stream << "Hour";
            break;
        case TimeRollingPolicy::MINUTE:
            stream << "Minute";
            break;
        case TimeRollingPolicy::SECOND:
            stream << "Second";
            break;
        default:
            stream << "<unknown>";
            break;
    }
    return stream;
}

} // namespace Logging
