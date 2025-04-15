// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_ELEMENT_H
#define LOGGING_ELEMENT_H

namespace Logging {

//! Logging element interface
/*!
    Logging filter takes an instance of a single logging record and
    performs some checks to detect if the record should be filered
    out and not processed anymore.

    \see Appender
    \see Filter
    \see Layout
    \see Processor
*/
class Element
{
public:
    //! Is the logging element started?
    virtual bool IsStarted() const noexcept { return true; }

    //! Start the logging element
    /*!
         \return 'true' if the logging element was successfully started, 'false' if the logging element failed to start
    */
    virtual bool Start() { return true; }
    //! Stop the logging element
    /*!
         \return 'true' if the logging element was successfully stopped, 'false' if the logging element failed to stop
    */
    virtual bool Stop() { return true; }
};

} // namespace Logging

#endif // LOGGING_ELEMENT_H
