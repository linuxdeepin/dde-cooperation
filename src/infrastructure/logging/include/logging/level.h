// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_LEVEL_H
#define LOGGING_LEVEL_H

#include <cstdint>

namespace Logging {

//! Logging level
enum class Level : uint8_t
{
    NONE  = 0x00,   //!< Log nothing
    FATAL = 0x1F,   //!< Log fatal errors
    ERROR = 0x3F,   //!< Log errors
    WARN  = 0x7F,   //!< Log warnings
    INFO  = 0x9F,   //!< Log information
    DEBUG = 0xBF,   //!< Log debug
    ALL   = 0xFF    //!< Log everything
};

//! Stream output: Logging level
/*!
    \param stream - Output stream
    \param level - Logging level
    \return Output stream
*/
template <class TOutputStream>
TOutputStream& operator<<(TOutputStream& stream, Level level);

} // namespace Logging

#include "level.inl"

#endif // LOGGING_LEVEL_H
