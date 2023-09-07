// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "baseutils.h"

#include <QProcessEnvironment>
#include <QFile>

using namespace deepin_cross;

bool BaseUtils::isWayland()
{
    if (osType() != kLinux)
        return false;

    auto e = QProcessEnvironment::systemEnvironment();
    QString XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));
    QString WAYLAND_DISPLAY = e.value(QStringLiteral("WAYLAND_DISPLAY"));

    return (XDG_SESSION_TYPE == QLatin1String("wayland")
            || WAYLAND_DISPLAY.contains(QLatin1String("wayland"), Qt::CaseInsensitive));
}

BaseUtils::OS_TYPE BaseUtils::osType()
{
#ifdef _WIN32
    return kWindows;
#elif __linux__
    return kLinux;
#elif __APPLE__
    return kMacOS;
#endif
    return kOther;
}
