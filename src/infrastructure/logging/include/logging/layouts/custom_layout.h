// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_LAYOUTS_CUSTOM_LAYOUT_H
#define LOGGING_LAYOUTS_CUSTOM_LAYOUT_H

#include "logging/layout.h"

#include <memory>
#include <string>
#include <functional>

namespace Logging {

//! Custom layout
/*!
    Custom layout converts the given logging record using a user-provided
    formatting function. This allows for complete flexibility in how logging
    records are formatted without needing to implement a full Layout subclass.

    Thread-safe if the provided formatting function is thread-safe.
*/
class CustomLayout : public Layout
{
public:
    //! Formatter function type definition
    using FormatterFn = std::function<std::string(const Record&)>;

    //! Initialize custom logging layout with a given formatter function
    /*!
         \param formatter - Function that formats the record into a string
    */
    explicit CustomLayout(FormatterFn formatter);
    CustomLayout(const CustomLayout&) = delete;
    CustomLayout(CustomLayout&&) = delete;
    virtual ~CustomLayout() = default;

    CustomLayout& operator=(const CustomLayout&) = delete;
    CustomLayout& operator=(CustomLayout&&) = delete;

    // Implementation of Layout
    void LayoutRecord(Record& record) override;

private:
    FormatterFn _formatter;
};

} // namespace Logging

#endif // LOGGING_LAYOUTS_CUSTOM_LAYOUT_H 