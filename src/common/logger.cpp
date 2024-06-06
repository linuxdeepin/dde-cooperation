// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logger.h"

using namespace deepin_cross;

std::ostringstream& Logger::stream()
{
    return _buffer;
}

Logger::Logger(const char* fname, unsigned line, int level)
{
    _lv = level;
    const char *levels[] = {"Debug  ", "Info   ", "Warning", "Error  ", "Fatal  "};
    _buffer << "["<< levels[level] << "]" << " [" << fname << ':' << line << "] ";
}

void Logger::init(const std::string &logpath, const std::string &logname) {
    CppCommon::Path savepath = CppCommon::Path(logpath);
    // Create a custom text layout pattern {LocalDate} {LocalTime}
    // std::string fileAndLine = std::string(__FILE__) + ":" + std::to_string(__LINE__);
    const std::string pattern = "{LocalYear}-{LocalMonth}-{LocalDay} {LocalHour}:{LocalMinute}:{LocalSecond}.{Millisecond} {Message} {EndLine}";


    // Create default logging sink processor with a text layout
    auto sink = std::make_shared<CppLogging::AsyncWaitProcessor>(std::make_shared<CppLogging::TextLayout>(pattern));

    // Add console appender
    sink->appenders().push_back(std::make_shared<CppLogging::ConsoleAppender>());
    // Add syslog appender
    sink->appenders().push_back(std::make_shared<CppLogging::SyslogAppender>());

    // Add file appender
    //CppCommon::Path logfile = logpath + "/" + logname + ".log";
    //sink->appenders().push_back(std::make_shared<CppLogging::FileAppender>(logfile));

    // Add file appender with time-based rolling policy and archivation
    // sink->appenders().push_back(std::make_shared<CppLogging::RollingFileAppender>(savepath, CppLogging::TimeRollingPolicy::DAY, logname + "_{LocalDate}.log", true));

    // Add rolling file appender which rolls after append 100MB of logs and will keep only 5 recent archives
    sink->appenders().push_back(std::make_shared<CppLogging::RollingFileAppender>(savepath, logname, "log", 104857600, 5, true));

    // Configure example logger
    CppLogging::Config::ConfigLogger(sink);

    // Startup the logging infrastructure
    CppLogging::Config::Startup();
}

Logger::~Logger()
{
    switch (_lv) {
    case 0:
        _logger.Debug(_buffer.str());
        break;
    case 1:
        _logger.Info(_buffer.str());
        break;
    case 2:
        _logger.Warn(_buffer.str());
        break;
    case 3:
        _logger.Error(_buffer.str());
        break;
    case 4:
        _logger.Fatal(_buffer.str());
        break;
    default:
        break;
    }
}
