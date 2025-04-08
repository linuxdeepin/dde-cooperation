// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


#ifndef BASEKIT_FILESYSTEM_EXCEPTIONS_H
#define BASEKIT_FILESYSTEM_EXCEPTIONS_H

#include "errors/exceptions.h"
#include "filesystem/path.h"

namespace BaseKit {

//! File system exception
class FileSystemException : public SystemException
{
public:
    using SystemException::SystemException;

    //! Get exception path
    const Path& path() const noexcept { return _path; }

    //! Get string from the current exception
    std::string string() const override;

    //! Attach the given path to the exception
    /*!
        \param path - Exception path
    */
    FileSystemException& Attach(const Path& path)
    { _path = path; return *this; }
    //! Attach the given source and destination paths to the exception
    /*!
        \param src - Exception source path
        \param dst - Exception destination path
    */
    FileSystemException& Attach(const Path& src, const Path& dst)
    { _src = src; _dst = dst; return *this; }

protected:
    //! Filesystem exception path
    Path _path;
    //! Filesystem exception source path
    Path _src;
    //! Filesystem exception destination path
    Path _dst;
};

} // namespace BaseKit

#endif // BASEKIT_FILESYSTEM_EXCEPTIONS_H
