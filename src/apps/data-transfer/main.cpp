// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "singleapplication.h"
#include "base/baseutils.h"
#include "config.h"

#include <dde-cooperation-framework/dpf.h>

#include <QDir>
#include <QIcon>
#include <QTranslator>

static constexpr char kPluginInterface[] { "org.deepin.plugin.datatransfer" };
static constexpr char kPluginCore[] { "data-transfer-core" };
#ifdef WIN32
#define LIB_FILE_NAME(lib_name) QString("%1.dll").arg(#lib_name)
#else
#define LIB_FILE_NAME(lib_name) QString("lib%1.so").arg(#lib_name)
#endif

static bool loadPlugins()
{
    QStringList pluginsDirs;
#ifdef QT_DEBUG
    const QString &pluginsDir { DDE_COOPERATION_PLUGIN_ROOT_DEBUG_DIR };
    qInfo() << QString("Load plugins path : %1").arg(pluginsDir);
    pluginsDirs.push_back(pluginsDir);
    pluginsDirs.push_back(pluginsDir + "/data-transfer");
    pluginsDirs.push_back(pluginsDir + "/data-transfer/core");
#else
    pluginsDirs << QString(DDE_COOPERATION_PLUGIN_ROOT_DIR);
    pluginsDirs << QString(DEEPIN_DATA_TRANS_PLUGIN_DIR);
    pluginsDirs << QDir::currentPath() + "/plugins";
    pluginsDirs << QDir::currentPath() + "/plugins/data-transfer";
    pluginsDirs << QDir::currentPath() + "/plugins/data-transfer/core";
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
    if (!corePlugin->fileName().contains(LIB_FILE_NAME(data-transfer-core)))
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

    deepin_cross::SingleApplication app(argc, argv);
#ifndef WIN32
    app.setOrganizationName("deepin");
    app.loadTranslator();
    app.setApplicationName("deepin-data-transfer");
    app.setApplicationDisplayName(app.translate("Application", "UOS data transfer"));
    app.setApplicationVersion(APP_VERSION);
    QIcon icon(":/icons/icon_256.svg");
    app.setProductIcon(icon);
    app.setApplicationAcknowledgementPage("https://www.deepin.org/acknowledgments/" );
    app.setApplicationDescription(app.translate("Application", "UOS transfer tool enables one click migration of your files, personal data, and applications to UOS, helping you seamlessly replace your system."));
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif


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

#ifdef WIN32
    // FIXME: windows上使用socket，即使线程资源全释放，进程也无法正常退出
    abort();
#endif
    return ret;
}
