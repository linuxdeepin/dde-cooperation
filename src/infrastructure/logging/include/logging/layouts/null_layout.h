// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_LAYOUTS_NULL_LAYOUT_H
#define LOGGING_LAYOUTS_NULL_LAYOUT_H

#include "logging/layout.h"

namespace Logging {

//! Null layout
/*!
    Null layout does nothing with a given logging record.

    Thread-safe.
*/
class NullLayout : public Layout
{
public:
    NullLayout() = default;
    NullLayout(const NullLayout&) = delete;
    NullLayout(NullLayout&&) = delete;
    virtual ~NullLayout() = default;

    NullLayout& operator=(const NullLayout&) = delete;
    NullLayout& operator=(NullLayout&&) = delete;

    // Implementation of Layout
    void LayoutRecord(Record& record) override {}
};

} // namespace Logging

#endif // LOGGING_LAYOUTS_NULL_LAYOUT_H
