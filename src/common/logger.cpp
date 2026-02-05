// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logger.h"
#include <mutex>

using namespace deepin_cross;

const char *Logger::_levels[] = {"Debug  ", "Info   ", "Warning", "Error  ", "Fatal  "};

Logger::Logger()
    : _initialized(false)  // atomic<bool> 的初始化
{
}

Logger::~Logger()
{
}

Logger::ThreadLocalData& Logger::getThreadLocalData()
{
    // thread_local 确保每个线程都有独立的缓冲区和级别
    thread_local ThreadLocalData data;
    return data;
}

LogStream Logger::log(const char* fname, unsigned line, int level)
{
    ThreadLocalData& data = getThreadLocalData();
    data.level = level;
    data.buffer << "["<< _levels[level] << "]" << " [" << fname << ':' << line << "] ";
    return LogStream(*this, data);
}

void Logger::init(const std::string &logpath, const std::string &logname) {
    // 使用互斥锁确保 init 只被调用一次，且不会被并发调用
    std::lock_guard<std::mutex> lock(_initMutex);

    // 使用 atomic load 确保线程安全地检查初始化状态
    if (_initialized.load(std::memory_order_acquire)) {
        return;
    }

    BaseKit::Path savepath = BaseKit::Path(logpath);
    // Create a custom text layout pattern {LocalDate} {LocalTime}
    // std::string fileAndLine = std::string(__FILE__) + ":" + std::to_string(__LINE__);
    const std::string pattern = "{LocalYear}-{LocalMonth}-{LocalDay} {LocalHour}:{LocalMinute}:{LocalSecond}.{Millisecond} {Message} {EndLine}";


    // Create default logging sink processor with a text layout
    auto sink = std::make_shared<Logging::AsyncWaitFreeProcessor>(std::make_shared<Logging::TextLayout>(pattern));

    // Add console appender
    sink->appenders().push_back(std::make_shared<Logging::ConsoleAppender>());
    // Add syslog appender
    sink->appenders().push_back(std::make_shared<Logging::SyslogAppender>());

    // Add file appender
    //BaseKit::Path logfile = logpath + "/" + logname + ".log";
    //sink->appenders().push_back(std::make_shared<Logging::FileAppender>(logfile));

    // Add file appender with time-based rolling policy and archivation
    // sink->appenders().push_back(std::make_shared<Logging::RollingFileAppender>(savepath, Logging::TimeRollingPolicy::DAY, logname + "_{LocalDate}.log", true));

    // Add rolling file appender which rolls after append 100MB of logs and will keep only 5 recent archives
    sink->appenders().push_back(std::make_shared<Logging::RollingFileAppender>(savepath, logname, "log", 104857600, 5, true));

    // Configure example logger
    Logging::Config::ConfigLogger("dde-cooperation", sink);

    // Startup the logging infrastructure
    Logging::Config::Startup();

    // last: get the configed logger
    _logger = Logging::Config::CreateLogger("dde-cooperation");

    // 标记日志系统已初始化（使用 atomic store 确保线程安全）
    _initialized.store(true, std::memory_order_release);
}

void Logger::stop()
{
    std::lock_guard<std::mutex> lock(_initMutex);
    // Shutdown the logging
    Logging::Config::Shutdown();
    // 使用 atomic store 确保线程安全
    _initialized.store(false, std::memory_order_release);
}

void Logger::logout(ThreadLocalData& data, const std::string& message)
{
    // 检查日志系统是否已初始化，防止访问未初始化的 _logger
    // 使用 atomic load 确保线程安全
    if (!_initialized.load(std::memory_order_acquire)) {
        // 未初始化时，使用 stderr 作为备用输出
        fprintf(stderr, "%s\n", message.c_str());
        fflush(stderr);
        // 清除错误标志和内容，避免后续日志调用受影响
        data.buffer.clear();
        data.buffer.str("");
        return;
    }

    switch (data.level) {
    case debug:
        _logger.Debug(message);
        break;
    case info:
        _logger.Info(message);
        break;
    case warning:
        _logger.Warn(message);
        break;
    case error:
        _logger.Error(message);
        break;
    case fatal:
        _logger.Fatal(message);
        break;
    default:
        break;
    }
    // 清除错误标志和内容
    data.buffer.clear();
    data.buffer.str("");
}
