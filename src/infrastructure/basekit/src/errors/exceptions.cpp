// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "errors/exceptions.h"

#include <sstream>

namespace BaseKit {

const char* Exception::what() const noexcept
{
    try
    {
        if (_cache.empty())
            string();
        return _cache.c_str();
    }
    catch (...)
    {
        return "Out of memory!";
    }
}

std::string Exception::string() const
{
    if (_cache.empty())
    {
        std::stringstream stream;
        stream << "Exception: " << _message << std::endl;
        std::string location = _location.string();
        if (!location.empty())
            stream << "Source location: " << location << std::endl;
        _cache = stream.str();
    }
    return _cache;
}

std::string SystemException::string() const
{
    if (_cache.empty())
    {
        std::stringstream stream;
        stream << "System exception: " << _message << std::endl;
        stream << "System error: " << _system_error << std::endl;
        stream << "System message: " << _system_message << std::endl;
        std::string location = _location.string();
        if (!location.empty())
            stream << "Source location: " << location << std::endl;
        _cache = stream.str();
    }
    return _cache;
}

} // namespace BaseKit
