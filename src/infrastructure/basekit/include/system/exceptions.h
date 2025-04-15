// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


#ifndef BASEKIT_SYSTEM_EXCEPTIONS_H
#define BASEKIT_SYSTEM_EXCEPTIONS_H

#include "filesystem/exceptions.h"

namespace BaseKit {

//! Dynamic link library exception
class DLLException : public FileSystemException
{
public:
    using FileSystemException::FileSystemException;
};

} // namespace BaseKit

#endif // BASEKIT_SYSTEM_EXCEPTIONS_H
