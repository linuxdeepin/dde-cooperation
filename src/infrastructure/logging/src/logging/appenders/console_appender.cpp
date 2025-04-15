// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logging/appenders/console_appender.h"

#include "system/console.h"

#include <cstdio>

namespace Logging {

void ConsoleAppender::AppendRecord(Record& record)
{
    // Skip logging records without layout
    if (record.raw.empty())
        return;

    // Setup console color depends on the logging level
    switch (record.level)
    {
        case Level::NONE:
            BaseKit::Console::SetColor(BaseKit::Color::DARKGREY);
            break;
        case Level::FATAL:
            BaseKit::Console::SetColor(BaseKit::Color::WHITE, BaseKit::Color::LIGHTRED);
            break;
        case Level::ERROR:
            BaseKit::Console::SetColor(BaseKit::Color::LIGHTRED);
            break;
        case Level::WARN:
            BaseKit::Console::SetColor(BaseKit::Color::YELLOW);
            break;
        case Level::INFO:
            BaseKit::Console::SetColor(BaseKit::Color::WHITE);
            break;
        case Level::DEBUG:
            BaseKit::Console::SetColor(BaseKit::Color::WHITE);
            break;
        case Level::ALL:
            BaseKit::Console::SetColor(BaseKit::Color::GREY);
            break;
    }

    // Append logging record content
    std::fwrite(record.raw.data(), 1, record.raw.size() - 1, stdout);

    // Reset console color
    BaseKit::Console::SetColor(BaseKit::Color::WHITE);
}

void ConsoleAppender::Flush()
{
    // Flush stream
    std::fflush(stdout);
}

} // namespace Logging
