// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/reader.h"

#include "utility/countof.h"

namespace BaseKit {

std::vector<uint8_t> Reader::ReadAllBytes()
{
    const size_t PAGE = 8192;

    uint8_t buffer[PAGE];
    std::vector<uint8_t> result;
    size_t size = 0;

    do
    {
        size = Read(buffer, countof(buffer));
        result.insert(result.end(), buffer, buffer + size);
    } while (size == countof(buffer));

    return result;
}

std::string Reader::ReadAllText()
{
    std::vector<uint8_t> bytes = ReadAllBytes();
    return std::string(bytes.begin(), bytes.end());
}

std::vector<std::string> Reader::ReadAllLines()
{
    std::string temp;
    std::vector<std::string> result;
    std::vector<uint8_t> bytes = ReadAllBytes();

    for (auto ch : bytes)
    {
        if ((ch == '\r') || (ch == '\n'))
        {
            if (!temp.empty())
            {
                result.push_back(temp);
                temp.clear();
            }
        }
        else
            temp += ch;
    }

    return result;
}

} // namespace BaseKit
