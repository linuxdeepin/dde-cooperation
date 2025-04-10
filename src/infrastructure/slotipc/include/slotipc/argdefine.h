// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ARG_DEFINE_H
#define GLOBAARG_DEFINE_HL_H

#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    #define METHOD_ARG QMetaMethodArgument
    #define METHOD_RE_ARG QMetaMethodReturnArgument
#else
    #define METHOD_ARG QGenericArgument
    #define METHOD_RE_ARG QGenericReturnArgument
#endif

#endif // ARG_DEFINE_H