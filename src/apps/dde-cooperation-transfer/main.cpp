// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "singleapplication.h"
#include "commandparser.h"
#include "base/baseutils.h"
#include "config.h"

#include <dde-cooperation-framework/dpf.h>

#include <QDir>

static constexpr char kPluginInterface[] { "org.deepin.plugin.cooperation" };
static constexpr char kPluginCore[] { "cooperation-core" };

static bool loadPlugins()
{
    QStringList pluginsDirs;
#ifdef QT_DEBUG
    const QString &pluginsDir { DDE_COOPERATION_PLUGIN_ROOT_DEBUG_DIR };
    qInfo() << QString("Load plugins path : %1").arg(pluginsDir);
    pluginsDirs.push_back(pluginsDir);
    pluginsDirs.push_back(pluginsDir + "/cooperation");
    pluginsDirs.push_back(pluginsDir + "/cooperation/core");
#else
    pluginsDirs << QString(DDE_COOPERATION_PLUGIN_ROOT_DIR);
    pluginsDirs << QString(DEEPIN_COOPERATION_PLUGIN_DIR);
    pluginsDirs << QDir::currentPath() + "/plugins";
    pluginsDirs << QDir::currentPath() + "/plugins/cooperation";
    pluginsDirs << QDir::currentPath() + "/plugins/cooperation/core";
#endif
#if defined(WIN32)
    pluginsDirs << QCoreApplication::applicationDirPath();
#endif

    qInfo() << "Using plugins dir:" << pluginsDirs;
    // TODO(zhangs): use config
    static const QStringList kLazyLoadPluginNames {};
    QStringList blackNames;

    DPF_NAMESPACE::LifeCycle::initialize({ kPluginInterface }, pluginsDirs, blackNames, kLazyLoadPluginNames);

    qInfo() << "Depend library paths:" << QCoreApplication::libraryPaths();
    qInfo() << "Load plugin paths: " << dpf::LifeCycle::pluginPaths();

    // read all plugins in setting paths
    if (!DPF_NAMESPACE::LifeCycle::readPlugins())
        return false;

    // We should make sure that the core plugin is loaded first
    auto corePlugin = DPF_NAMESPACE::LifeCycle::pluginMetaObj(kPluginCore);
    if (corePlugin.isNull())
        return false;
    if (!corePlugin->fileName().contains(kPluginCore))
        return false;
    if (!DPF_NAMESPACE::LifeCycle::loadPlugin(corePlugin))
        return false;

    // load plugins without core
    if (!DPF_NAMESPACE::LifeCycle::loadPlugins())
        return false;

    return true;
}

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    SingleApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setProperty("onlyTransfer", true);

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

    if (!loadPlugins()) {
        qCritical() << "load plugin failed";
        return -1;
    }

    CommandParser::instance().process();
    CommandParser::instance().processCommand();
    int ret = app.exec();
    DPF_NAMESPACE::LifeCycle::shutdownPlugins();

    app.closeServer();
    return ret;
}
