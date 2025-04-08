// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_FILTERS_SWITCH_FILTER_H
#define LOGGING_FILTERS_SWITCH_FILTER_H

#include "logging/filter.h"

#include <atomic>

namespace Logging {

//! Switch filter
/*!
    Switch filters is used to turn on/off logging.

    Thread-safe.
*/
class SwitchFilter : public Filter
{
public:
    //! Initialize switch filter with a given enabled flag
    /*!
         \param enabled - Enabled flag
    */
    explicit SwitchFilter(bool enabled) { Update(enabled); }
    SwitchFilter(const SwitchFilter&) = delete;
    SwitchFilter(SwitchFilter&&) = delete;
    virtual ~SwitchFilter() = default;

    SwitchFilter& operator=(const SwitchFilter&) = delete;
    SwitchFilter& operator=(SwitchFilter&&) = delete;

    //! Get the filter enabled flag
    bool enabled() const noexcept { return _enabled; }

    //! Update switch filter with a given enabled flag
    /*!
         \param enabled - Enabled flag
    */
    void Update(bool enabled);

    // Implementation of Filter
    bool FilterRecord(Record& record) override;

private:
    std::atomic<bool> _enabled;
};

} // namespace Logging

#endif // LOGGING_FILTERS_SWITCH_FILTER_H
