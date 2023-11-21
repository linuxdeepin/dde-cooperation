// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "daemoncooperationplugin.h"
#include "global_defines.h"
#include "maincontroller/maincontroller.h"
#include "utils/cooperationutil.h"

#include "config/configmanager.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QTranslator>
#include <QTimer>
#include <QDir>

using namespace daemon_cooperation;

void DaemonCooperationPlugin::initialize()
{
    auto appName = qApp->applicationName();
    qApp->setApplicationName(MainAppName);
    ConfigManager::instance();
    loadTranslator();
    qApp->setApplicationName(appName);

    if (DPF_NAMESPACE::LifeCycle::isAllPluginsStarted())
        onAllPluginsStarted();
    else
        connect(dpfListener, &DPF_NAMESPACE::Listener::pluginsStarted, this, &DaemonCooperationPlugin::onAllPluginsStarted, Qt::DirectConnection);
}

bool DaemonCooperationPlugin::start()
{
    return true;
}

void DaemonCooperationPlugin::stop()
{
    MainController::instance()->unregist();
}

void DaemonCooperationPlugin::loadTranslator()
{
    QStringList translateDirs;
    auto dataDirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (auto &dir : dataDirs) {
        translateDirs << dir + QDir::separator() + qApp->applicationName() + QDir::separator() + "translations";
    }

    auto locale = QLocale::system();
    QStringList missingQmfiles;
    QStringList translateFilenames { QString("%1_%2").arg(qApp->applicationName()).arg(QLocale::system().name()) };
    const QStringList parseLocalNameList = locale.name().split("_", QString::SkipEmptyParts);
    if (parseLocalNameList.length() > 0)
        translateFilenames << QString("%1_%2").arg(qApp->applicationName()).arg(parseLocalNameList.at(0));

    for (const auto &translateFilename : translateFilenames) {
        for (const auto &dir : translateDirs) {
            QString translatePath = dir + QDir::separator() + translateFilename;
            if (QFile::exists(translatePath + ".qm")) {
                qDebug() << "load translate" << translatePath;
                auto translator = new QTranslator(qApp);
                translator->load(translatePath);
                qApp->installTranslator(translator);
                qApp->setProperty("dapp_locale", locale.name());
                return;
            }
        }

        if (locale.language() != QLocale::English)
            missingQmfiles << translateFilename + ".qm";
    }

    if (missingQmfiles.size() > 0) {
        qWarning() << qApp->applicationName() << "can not find qm files" << missingQmfiles;
    }
}

void DaemonCooperationPlugin::onAllPluginsStarted()
{
    CooperationUtil::instance();
    // 延时，确保服务已启动
    QTimer::singleShot(1000, this, [] {
        MainController::instance()->regist();
    });
}
