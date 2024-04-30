// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGER_H
#define LOGGER_H

#include "logging/config.h"
#include "logging/logger.h"

#include <sstream>

namespace deepin_cross {

enum LogLevel {
    debug = 0,
    info = 1,
    warning = 2,
    error = 3,
    fatal = 4
};

inline LogLevel g_logLevel = warning;

class Logger
{
public:
    static void init(const std::string &logpath, const std::string &logname);

    Logger(const char* fname, unsigned line, int level);
    ~Logger();

    std::ostringstream& stream();

private:
    CppLogging::Logger _logger;
    std::ostringstream _buffer;
    int _lv;
};

template <int N>
constexpr const char* path_base(const char(&s)[N], int i = N - 1) {
    return (s[i] == '/' || s[i] == '\\') ? (s + i + 1) : (i == 0 ? s : path_base(s, i - 1));
}

template <int N>
constexpr int path_base_len(const char(&s)[N], int i = N - 1) {
    return (s[i] == '/' || s[i] == '\\') ? (N - 2 - i) : (i == 0 ? N-1 : path_base_len(s, i - 1));
}

}

using namespace deepin_cross;

#define _LOG_FNAME deepin_cross::path_base(__FILE__)
//#define _LOG_FNLEN deepin_cross::path_base_len(__FILE__)
#define _LOG_FILELINE _LOG_FNAME,__LINE__

#define _LOG_STREAM(lv)  deepin_cross::Logger(_LOG_FILELINE, lv).stream()

#define DLOG  if (deepin_cross::g_logLevel <= deepin_cross::debug)   _LOG_STREAM(deepin_cross::debug)
#define LOG   if (deepin_cross::g_logLevel <= deepin_cross::info)    _LOG_STREAM(deepin_cross::info)
#define WLOG  if (deepin_cross::g_logLevel <= deepin_cross::warning) _LOG_STREAM(deepin_cross::warning)
#define ELOG  if (deepin_cross::g_logLevel <= deepin_cross::error)   _LOG_STREAM(deepin_cross::error)
#define FLOG  if (deepin_cross::g_logLevel <= deepin_cross::fatal)   _LOG_STREAM(deepin_cross::fatal)

// conditional log
#define DLOG_IF(cond) if (cond) DLOG
#define  LOG_IF(cond) if (cond) LOG
#define WLOG_IF(cond) if (cond) WLOG
#define ELOG_IF(cond) if (cond) ELOG
#define FLOG_IF(cond) if (cond) FLOG

#endif // LOGGER_H
