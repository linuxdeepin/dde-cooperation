// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_LAYOUT_H
#define LOGGING_LAYOUT_H

#include "logging/element.h"
#include "logging/record.h"

namespace Logging {

//! Logging layout interface
/*!
    Logging layout takes an instance of a single logging record
    and convert it into a raw buffer (raw filed will be updated).

    \see NullLayout
    \see EmptyLayout
    \see BinaryLayout
    \see TextLayout
*/
class Layout : public Element
{
public:
    //! Layout the given logging record into a raw buffer
    /*!
         This method will update the raw filed of the given logging record.

         \param record - Logging record
    */
    virtual void LayoutRecord(Record& record) = 0;
};

} // namespace Logging

#endif // LOGGING_LAYOUT_H
