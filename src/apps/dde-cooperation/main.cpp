// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "singleton/singleapplication.h"
#include "base/baseutils.h"
#include "config.h"

#include "core/cooperationcoreplugin.h"

#include <QDir>
#include <QIcon>
#include <QDebug>

#include <signal.h>

#if defined(_WIN32) || defined(_WIN64)
// Fix no OPENSSL_Applink crash issue.
extern "C"
{
#include <openssl/applink.c>
}
#endif

#define BASEPROTO_PORT 51597

using namespace deepin_cross;
using namespace cooperation_core;

static void appExitHandler(int sig)
{
    qInfo() << "break with !SIGTERM! " << sig;
    qApp->quit();
}

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    deepin_cross::SingleApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

#ifdef linux
    app.loadTranslator();
    app.setApplicationDisplayName(app.translate("Application", "Cooperation"));
    app.setApplicationVersion(APP_VERSION);
    app.setProductIcon(QIcon::fromTheme("dde-cooperation"));
    app.setApplicationAcknowledgementPage("https://www.deepin.org/acknowledgments/");
    app.setApplicationDescription(app.translate("Application", "Cooperation is a powerful cross-terminal "
                                                               "office tool that helps you deliver files, "
                                                               "share keys and mice, and share clipboards "
                                                               "between different devices."));
#endif

    bool inUse = BaseUtils::portInUse(BASEPROTO_PORT);
    if (inUse) {
        qCritical() << "exit, network port (" << BASEPROTO_PORT << ") is busing........";
        return 1;
    }

    bool canSetSingle = app.setSingleInstance(app.applicationName());
    if (!canSetSingle) {
        qInfo() << "single application is already running.";
        return 0;
    }

    if (deepin_cross::BaseUtils::isWayland()) {
        // do something
    }

    CooperaionCorePlugin *core =  new CooperaionCorePlugin();
    core->start();

    // 安全退出
#ifndef _WIN32
    signal(SIGQUIT, appExitHandler);
#endif
    signal(SIGINT, appExitHandler);
    signal(SIGTERM, appExitHandler);
    int ret = app.exec();

    core->stop();
    app.closeServer();

    return ret;
}
