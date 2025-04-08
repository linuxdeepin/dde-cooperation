// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_LAYOUTS_BINARY_LAYOUT_H
#define LOGGING_LAYOUTS_BINARY_LAYOUT_H

#include "logging/layout.h"

namespace Logging {

//! Binary layout
/*!
    Binary layout performs simple memory copy operation to convert
    the given logging record into the plane raw buffer.

    Thread-safe.
*/
class BinaryLayout : public Layout
{
public:
    BinaryLayout() = default;
    BinaryLayout(const BinaryLayout&) = delete;
    BinaryLayout(BinaryLayout&&) = delete;
    virtual ~BinaryLayout() = default;

    BinaryLayout& operator=(const BinaryLayout&) = delete;
    BinaryLayout& operator=(BinaryLayout&&) = delete;

    // Implementation of Layout
    void LayoutRecord(Record& record) override;
};

} // namespace Logging

#endif // LOGGING_LAYOUTS_BINARY_LAYOUT_H
