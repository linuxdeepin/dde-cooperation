// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COMMONUTILS_H
#define COMMONUTILS_H

#include <QString>

#include <co/flag.h>
#include <co/log.h>

namespace deepin_cross {
class CommonUitls
{
public:
    static std::string getFirstIp();

    static void loadTranslator();

    static void initLog();

private:
    static QString logDir();
    static bool detailLog();
};
}

#endif   // COMMONUTILS_H
