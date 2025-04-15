// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_FILTERS_LOGGER_FILTER_H
#define LOGGING_FILTERS_LOGGER_FILTER_H

#include "logging/filter.h"

#include <atomic>
#include <string>

namespace Logging {

//! Logger filter
/*!
    Logger filters out logging records which logger filed is not matched
    to the given pattern.

    Thread-safe.
*/
class LoggerFilter : public Filter
{
public:
    //! Initialize logger filter with a given pattern
    /*!
         \param pattern - Logger pattern
         \param positive - Positive filtration (default is true)
    */
    explicit LoggerFilter(const std::string& pattern, bool positive = true) : _positive(positive), _pattern(pattern) {}
    LoggerFilter(const LoggerFilter&) = delete;
    LoggerFilter(LoggerFilter&&) = delete;
    virtual ~LoggerFilter() = default;

    LoggerFilter& operator=(const LoggerFilter&) = delete;
    LoggerFilter& operator=(LoggerFilter&&) = delete;

    //! Get the positive filtration flag
    bool positive() const noexcept { return _positive; }

    //! Get the logger pattern
    const std::string& pattern() const noexcept { return _pattern; }

    // Implementation of Filter
    bool FilterRecord(Record& record) override;

private:
    std::atomic<bool> _positive;
    std::string _pattern;
};

} // namespace Logging

#endif // LOGGING_FILTERS_LOGGER_FILTER_H
