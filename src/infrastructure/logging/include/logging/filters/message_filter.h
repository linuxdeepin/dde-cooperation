// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_FILTERS_MESSAGE_FILTER_H
#define LOGGING_FILTERS_MESSAGE_FILTER_H

#include "logging/filter.h"

#include <atomic>
#include <regex>

namespace Logging {

//! Message filter
/*!
    Message filters out logging records which message field is not matched
    to the given regular expression pattern.

    Thread-safe.
*/
class MessageFilter : public Filter
{
public:
    //! Initialize message filter with a given regular expression pattern
    /*!
         \param pattern - Regular expression pattern
         \param positive - Positive filtration (default is true)
    */
    explicit MessageFilter(const std::regex& pattern, bool positive = true) : _positive(positive), _pattern(pattern) {}
    MessageFilter(const MessageFilter&) = delete;
    MessageFilter(MessageFilter&&) = delete;
    virtual ~MessageFilter() = default;

    MessageFilter& operator=(const MessageFilter&) = delete;
    MessageFilter& operator=(MessageFilter&&) = delete;

    //! Get the positive filtration flag
    bool positive() const noexcept { return _positive; }

    //! Get the message regular expression pattern
    const std::regex& pattern() const noexcept { return _pattern; }

    // Implementation of Filter
    bool FilterRecord(Record& record) override;

private:
    std::atomic<bool> _positive;
    std::regex _pattern;
};

} // namespace Logging

#endif // LOGGING_FILTERS_MESSAGE_FILTER_H
