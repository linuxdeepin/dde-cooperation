// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "baseutils.h"

#include <QProcessEnvironment>
#include <QDebug>

using namespace deepin_cross;

bool BaseUtils::isWayland()
{
    qInfo() << "Checking if running under Wayland";
    if (osType() != kLinux) {
        qInfo() << "Not Linux system, Wayland not supported";
        return false;
    }

    auto e = QProcessEnvironment::systemEnvironment();
    QString XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));
    QString WAYLAND_DISPLAY = e.value(QStringLiteral("WAYLAND_DISPLAY"));

    bool isWayland = (XDG_SESSION_TYPE == QLatin1String("wayland")
            || WAYLAND_DISPLAY.contains(QLatin1String("wayland"), Qt::CaseInsensitive));
    qInfo() << "Wayland detection result: " << isWayland;
    return isWayland;
}

bool BaseUtils::portInUse(int port)
{
    qInfo() << "Checking if port " << port << " is in use";
    QProcess process;
    process.start("netstat -ano");
    process.waitForFinished(3000);

    // 获取命令输出
    QString output = process.readAllStandardOutput();
    bool portUsed = output.contains("0.0.0.0:" + QString::number(port));
    qInfo() << "Port " << port << (portUsed ? " is in use" : " is available");
    return portUsed;
}


BaseUtils::OS_TYPE BaseUtils::osType()
{
#ifdef _WIN32
    qInfo() << "Detected Windows system";
    return kWindows;
#elif __linux__
    qInfo() << "Detected Linux system";
    return kLinux;
#elif __APPLE__
    qInfo() << "Detected macOS system";
    return kMacOS;
#endif
    qInfo() << "Unknown system type";
    return kOther;
}
