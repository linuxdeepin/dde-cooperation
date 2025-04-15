// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// this header must come first so that it picks up the filesystem implementation
#include <ghc/fs_impl.hpp>

#include "filesystem.h"
#if SYSAPI_WIN32
#include "win32/encodingutilities.h"
#endif
#include <fstream>

namespace sslconf {

namespace {

template<class Stream>
void open_utf8_path_impl(Stream& stream, const fs::path& path, std::ios_base::openmode mode)
{
#if SYSAPI_WIN32
    // on Windows we need to use a non-standard constructor from wchar_t* string
    // which fs::path::native() returns
    stream.open(path.native().c_str(), mode);
#else
    stream.open(path.native().c_str(), mode);
#endif
}

} // namespace

void open_utf8_path(std::ifstream& stream, const fs::path& path, std::ios_base::openmode mode)
{
    open_utf8_path_impl(stream, path, mode);
}

void open_utf8_path(std::ofstream& stream, const fs::path& path, std::ios_base::openmode mode)
{
    open_utf8_path_impl(stream, path, mode);
}

void open_utf8_path(std::fstream& stream, const fs::path& path, std::ios_base::openmode mode)
{
    open_utf8_path_impl(stream, path, mode);
}

std::FILE* fopen_utf8_path(const fs::path& path, const std::string& mode)
{
#if SYSAPI_WIN32
    auto wchar_mode = utf8_to_win_char(mode);
    return _wfopen(path.native().c_str(),
                   reinterpret_cast<wchar_t*>(wchar_mode.data()));
#else
    return std::fopen(path.native().c_str(), mode.c_str());
#endif
}

} // namespace sslconf
