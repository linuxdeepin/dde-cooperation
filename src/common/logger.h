// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGER_H
#define LOGGER_H

#include "logging/config.h"
#include "logging/logger.h"

#include <sstream>
#include <mutex>
#include <atomic>

namespace deepin_cross {

enum LogLevel {
    debug = 0,
    info = 1,
    warning = 2,
    error = 3,
    fatal = 4
};

inline LogLevel g_logLevel = warning;

template <int N>
constexpr const char* path_base(const char(&s)[N], int i = N - 1) {
    return (s[i] == '/' || s[i] == '\\') ? (s + i + 1) : (i == 0 ? s : path_base(s, i - 1));
}

template <int N>
constexpr int path_base_len(const char(&s)[N], int i = N - 1) {
    return (s[i] == '/' || s[i] == '\\') ? (N - 2 - i) : (i == 0 ? N-1 : path_base_len(s, i - 1));
}

class LogStream;
class Logger : public BaseKit::Singleton<Logger>
{
    friend BaseKit::Singleton<Logger>;
public:
    Logger();

    ~Logger();

    void init(const std::string &logpath, const std::string &logname);
    void stop();

    LogStream log(const char* fname, unsigned line, int level);

    // 使用 thread_local 确保每个线程有自己的缓冲区和级别
    struct ThreadLocalData {
        std::ostringstream buffer;
        int level;
        ThreadLocalData() : level(0) {}
    };

    ThreadLocalData& getThreadLocalData();
    void logout(ThreadLocalData& data, const std::string& message);

private:
    static const char *_levels[];
    Logging::Logger _logger;
    std::atomic<bool> _initialized;  // 使用原子变量防止数据竞争

    // 互斥锁保护 init/stop 操作
    std::mutex _initMutex;
};

// 代理类 LogStream
class LogStream {
public:
    LogStream(Logger& logger, Logger::ThreadLocalData& data) : _logger(logger), _data(data) {};

    ~LogStream() {
        // 在析构时调用输出
        _logger.logout(_data, _data.buffer.str());
    }

    template<typename T>
    LogStream& operator<<(const T& data) {
        _data.buffer << data;
        return *this;
    };

    // 处理 std::endl 和其他操纵符
    LogStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
        manip(_data.buffer);
        _logger.logout(_data, _data.buffer.str());
        return *this;
    };

private:
    Logger& _logger;
    Logger::ThreadLocalData& _data;
};

}

#endif // LOGGER_H
