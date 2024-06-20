// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "String.h"

#include <algorithm>
#include <iomanip>

namespace barrier {
namespace string {

namespace {

// returns negative in case of non-matching character
int hex_to_number(char ch)
{
    switch (ch) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;

        case 'a': return 10;
        case 'b': return 11;
        case 'c': return 12;
        case 'd': return 13;
        case 'e': return 14;
        case 'f': return 15;

        case 'A': return 10;
        case 'B': return 11;
        case 'C': return 12;
        case 'D': return 13;
        case 'E': return 14;
        case 'F': return 15;
    }
    return -1;
}

} // namespace


std::string to_hex(const std::vector<std::uint8_t>& subject, int width, const char fill)
{
    std::stringstream ss;
    ss << std::hex;
    for (unsigned int i = 0; i < subject.size(); i++) {
        ss << std::setw(width) << std::setfill(fill) << static_cast<int>(subject[i]);
    }

    return ss.str();
}

std::vector<std::uint8_t> from_hex(const std::string& data)
{
    std::vector<std::uint8_t> result;
    result.reserve(data.size() / 2);

    std::size_t i = 0;
    while (i < data.size()) {
        if (data[i] == ':') {
            i++;
            continue;
        }

        if (i + 2 > data.size()) {
            return {}; // uneven character count follows, it's unclear how to interpret it
        }

        auto high = hex_to_number(data[i]);
        auto low = hex_to_number(data[i + 1]);
        if (high < 0 || low < 0) {
            return {};
        }
        result.push_back(high * 16 + low);
        i += 2;
    }
    return result;
}

void
uppercase(std::string& subject)
{
    std::transform(subject.begin(), subject.end(), subject.begin(), ::toupper);
}

}
}
