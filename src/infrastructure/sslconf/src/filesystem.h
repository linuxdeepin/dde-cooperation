// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SSLCONF_FILESYSTEM_H
#define SSLCONF_FILESYSTEM_H

#include <cstdio>
#include <iosfwd>
#include <ios>
#include <ghc/fs_fwd.hpp>

namespace sslconf {

namespace fs = ghc::filesystem;

void open_utf8_path(std::ifstream& stream, const fs::path& path,
                    std::ios_base::openmode mode = std::ios_base::in);
void open_utf8_path(std::ofstream& stream, const fs::path& path,
                    std::ios_base::openmode mode = std::ios_base::out);
void open_utf8_path(std::fstream& stream, const fs::path& path,
                    std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out);

std::FILE* fopen_utf8_path(const fs::path& path, const std::string& mode);

} // namespace sslconf

#endif
