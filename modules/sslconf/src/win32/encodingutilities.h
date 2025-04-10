// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SSLCONF_WIN32_ENCODING_UTILITIES_H
#define SSLCONF_WIN32_ENCODING_UTILITIES_H

#include <windows.h>
#include <string>
#include <vector>

std::string win_wchar_to_utf8(const WCHAR* utfStr);
std::vector<WCHAR> utf8_to_win_char(const std::string& str);

#endif
