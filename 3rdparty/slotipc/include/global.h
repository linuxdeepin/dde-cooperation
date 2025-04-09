// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    #define METHOD_ARG QMetaMethodArgument
    #define METHOD_RE_ARG QMetaMethodReturnArgument
#else
    #define METHOD_ARG QGenericArgument
    #define METHOD_RE_ARG QGenericReturnArgument
#endif

#endif // GLOBAL_H 