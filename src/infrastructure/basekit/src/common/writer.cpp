// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/writer.h"

#include "system/environment.h"

namespace BaseKit {

size_t Writer::Write(const std::string& text)
{
    return Write(text.data(), text.size());
}

size_t Writer::Write(const std::vector<std::string>& lines)
{
    static std::string endline = Environment::EndLine();

    size_t result = 0;
    for (const auto& line : lines)
    {
        if (Write(line.data(), line.size()) != line.size())
            break;
        if (Write(endline.data(), endline.size()) != endline.size())
            break;
        ++result;
    }
    return result;
}

} // namespace BaseKit
