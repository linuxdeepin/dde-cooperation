// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "errors/fatal.h"

#include <cstdlib>
#include <iostream>

namespace BaseKit {

void fatal(const SourceLocation& location, const StackTrace& trace, const std::string& message, int error) noexcept
{
    std::cerr << "Fatal error: " << message << std::endl;
    std::cerr << "System error: " << error << std::endl;
    std::cerr << "System message: " << SystemError::Description(error) << std::endl;
    std::cerr << "Source location: " << location.string() << std::endl;
    std::cerr << "Stack trace: " << std::endl << trace.string() << std::endl;
    std::abort();
}

void fatal(const SourceLocation& location, const StackTrace& trace, const std::exception& fatal) noexcept
{
    std::cerr << fatal.what() << std::endl;
    std::abort();
}

} // namespace BaseKit
