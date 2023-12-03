// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "daemoncooperationplugin.h"
#include "global_defines.h"
#include "maincontroller/maincontroller.h"
#include "utils/cooperationutil.h"
#include "common/commonutils.h"

#include "config/configmanager.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QTranslator>
#include <QTimer>
#include <QDir>

using namespace daemon_cooperation;
using namespace deepin_cross;

void DaemonCooperationPlugin::initialize()
{
    auto appName = qApp->applicationName();
    qApp->setApplicationName(MainAppName);
    ConfigManager::instance();
    qApp->setApplicationName(appName);

    CommonUitls::loadTranslator();

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

void DaemonCooperationPlugin::onAllPluginsStarted()
{
    CooperationUtil::instance();
    // 延时，确保服务已启动
    QTimer::singleShot(1000, this, [] {
        MainController::instance()->regist();
    });
}
