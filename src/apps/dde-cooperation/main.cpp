// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "singleton/singleapplication.h"
#include "base/baseutils.h"
#include "config.h"

#include <deepin-cross-framework/dpf.h>

// static constexpr char kPluginInterface[] { "org.deepin.plugin.deepincross" };
// static constexpr char kPluginCore[] { "data-transfer-core" };
// static constexpr char kLibCore[] { "libdata-transfer-core.so" };

static bool loadPlugins()
{ /*
     QStringList pluginsDirs;
 #ifdef QT_DEBUG
     const QString &pluginsDir { DEEPIN_CROSS_BUILD_PLUGIN_DIR };
     qInfo() << QString("Load plugins path : %1").arg(pluginsDir);
     pluginsDirs.push_back(pluginsDir);
     pluginsDirs.push_back(pluginsDir + "/data-transfer");
 #else
     pluginsDirs << QString(DEEPIN_DATA_TRANS_PLUGIN_DIR);
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
     if (!corePlugin->fileName().contains(kLibCore))
         return false;
     if (!DPF_NAMESPACE::LifeCycle::loadPlugin(corePlugin))
         return false;

     // load plugins without core
     if (!DPF_NAMESPACE::LifeCycle::loadPlugins())
         return false;*/

    return true;
}

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    deepin_cross::SingleApplication app(argc, argv);
    bool canSetSingle = app.setSingleInstance(app.applicationName());
    if (!canSetSingle) {
        qInfo() << "single application is already running.";
        return 0;
    }

    if (deepin_cross::BaseUtils::isWayland()) {
        // do something
    }

    if (!loadPlugins()) {
        qCritical() << "load plugin failed";
        return -1;
    }

    int ret = app.exec();

    app.closeServer();
    return ret;
}
