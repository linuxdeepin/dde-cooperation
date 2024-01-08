// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BASEUTILS_H
#define BASEUTILS_H
#include <QString>

namespace deepin_cross {
class BaseUtils
{
public:
    static bool isWayland();

    enum OS_TYPE {
        kLinux,
        kWindows,
        kMacOS,
        kOther
    };
    static OS_TYPE osType();

    static QString logDir();
};
}

#endif   // BASEUTILS_H
