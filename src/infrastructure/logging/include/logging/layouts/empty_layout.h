// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_LAYOUTS_EMPTY_LAYOUT_H
#define LOGGING_LAYOUTS_EMPTY_LAYOUT_H

#include "logging/layout.h"

namespace Logging {

//! Empty layout
/*!
    Empty layout performs zero memory operation to convert
    the given logging record into the empty raw buffer.

    Thread-safe.
*/
class EmptyLayout : public Layout
{
public:
    EmptyLayout() = default;
    EmptyLayout(const EmptyLayout&) = delete;
    EmptyLayout(EmptyLayout&&) = delete;
    virtual ~EmptyLayout() = default;

    EmptyLayout& operator=(const EmptyLayout&) = delete;
    EmptyLayout& operator=(EmptyLayout&&) = delete;

    // Implementation of Layout
    void LayoutRecord(Record& record) override { record.raw.clear(); }
};

} // namespace Logging

#endif // LOGGING_LAYOUTS_EMPTY_LAYOUT_H
