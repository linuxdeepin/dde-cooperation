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

template <int N>
constexpr const char* path_base(const char(&s)[N], int i = N - 1) {
    return (s[i] == '/' || s[i] == '\\') ? (s + i + 1) : (i == 0 ? s : path_base(s, i - 1));
}

template <int N>
constexpr int path_base_len(const char(&s)[N], int i = N - 1) {
    return (s[i] == '/' || s[i] == '\\') ? (N - 2 - i) : (i == 0 ? N-1 : path_base_len(s, i - 1));
}

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

}

#endif // LOGGER_H
