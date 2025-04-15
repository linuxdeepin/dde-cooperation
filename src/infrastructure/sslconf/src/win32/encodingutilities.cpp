// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "encodingutilities.h"
#include <stringapiset.h>

std::string win_wchar_to_utf8(const WCHAR* utfStr)
{
    int utfLength = lstrlenW(utfStr);
    int mbLength = WideCharToMultiByte(CP_UTF8, 0, utfStr, utfLength, NULL, 0, NULL, NULL);
    std::string mbStr(mbLength, 0);
    WideCharToMultiByte(CP_UTF8, 0, utfStr, utfLength, &mbStr[0], mbLength, NULL, NULL);
    return mbStr;
}

std::vector<WCHAR> utf8_to_win_char(const std::string& str)
{
    int result_len = MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), NULL, 0);
    std::vector<WCHAR> result;
    result.resize(result_len + 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), result.data(), result_len);
    return result;
}
