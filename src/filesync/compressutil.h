// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COMPRESSUTIL_H
#define COMPRESSUTIL_H

#include "utility/singleton.h"

#include <iostream>
#include <fstream>

class CompressUtil : public CppCommon::Singleton<CompressUtil>
{
    friend CppCommon::Singleton<CompressUtil>;

public:
    void compressFolder(const std::string &folderPath, std::ostream &outputStream, int cLevel);
    void decompressFolder(std::ifstream &compressedStream, const std::string &outputFolder);

private:
    void *malloc_buffer(size_t size);
};

#endif // COMPRESSUTIL_H
