// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace Logging {

template <class TOutputStream>
inline TOutputStream& operator<<(TOutputStream& stream, Level level)
{
    switch (level)
    {
        case Level::NONE:
            stream << "None";
            break;
        case Level::FATAL:
            stream << "Fatal";
            break;
        case Level::ERROR:
            stream << "Error";
            break;
        case Level::WARN:
            stream << "Warn";
            break;
        case Level::INFO:
            stream << "Info";
            break;
        case Level::DEBUG:
            stream << "Debug";
            break;
        case Level::ALL:
            stream << "All";
            break;
        default:
            stream << "<unknown>";
            break;
    }
    return stream;
}

} // namespace Logging
