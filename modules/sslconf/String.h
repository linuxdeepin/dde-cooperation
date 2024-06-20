// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef STRING_H
#define STRING_H

#include <string>
#include <vector>
#include <cstdint>

// use standard C++ string class for our string class
typedef std::string String;

namespace barrier {

//! std::string utilities
/*!
Provides functions for string manipulation.
*/
namespace string {

//! Convert into hexdecimal
/*!
Convert each character in \c subject into hexdecimal form with \c width
*/
std::string to_hex(const std::vector<std::uint8_t>& subject, int width, const char fill = '0');

/// Convert binary data from hexadecimal
std::vector<std::uint8_t> from_hex(const std::string& data);

//! Convert to all uppercase
/*!
Convert each character in \c subject to uppercase
*/
void uppercase(std::string& subject);

}
}

#endif // STRING_H
