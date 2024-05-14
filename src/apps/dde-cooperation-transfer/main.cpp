// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "singleapplication.h"
#include "commandparser.h"
#include "base/baseutils.h"
#include "config.h"

#include "core/cooperationcoreplugin.h"

#include <QDir>

using namespace cooperation_core;

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    SingleApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setProperty("onlyTransfer", true);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    {
        // 加载翻译
        auto appName = app.applicationName();
        app.setApplicationName("dde-cooperation");
        app.loadTranslator();
        app.setApplicationName(appName);
    }

    bool isSingleInstance = app.setSingleInstance(app.applicationName());
    if (!isSingleInstance) {
        qInfo() << "new client";
        app.handleNewClient(app.applicationName());
        return 0;
    }

    if (deepin_cross::BaseUtils::isWayland()) {
        // do something
    }

    CooperaionCorePlugin *core =  new CooperaionCorePlugin();
    core->start();

    CommandParser::instance().process();
    CommandParser::instance().processCommand();
    int ret = app.exec();

    core->stop();
    app.closeServer();
    return ret;
}
