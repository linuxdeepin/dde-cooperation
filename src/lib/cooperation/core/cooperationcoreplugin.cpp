// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationcoreplugin.h"
#include "events/cooperationcoreeventreceiver.h"
#include "utils/cooperationutil.h"
#include "maincontroller/maincontroller.h"
#include "discovercontroller/discovercontroller.h"
#include "transfer/transferhelper.h"
#include "cooperation/cooperationmanager.h"
#include "info/deviceinfo.h"

#include "common/commonutils.h"
#include "configs/settings/configmanager.h"
#include "singleton/singleapplication.h"

#ifdef __linux__
#    include "base/reportlog/reportlogmanager.h"
#    include <DFeatureDisplayDialog>
#    include <QFile>
DWIDGET_USE_NAMESPACE
#endif

using namespace cooperation_core;
using namespace deepin_cross;

CooperaionCorePlugin::CooperaionCorePlugin(QObject *parent)
    : QObject(parent)
{
    initialize();
}

CooperaionCorePlugin::~CooperaionCorePlugin()
{
}

void CooperaionCorePlugin::initialize()
{
    if (qApp->property("onlyTransfer").toBool()) {
        auto appName = qApp->applicationName();
        qApp->setApplicationName(MainAppName);
        ConfigManager::instance();
        qApp->setApplicationName(appName);
    } else {
        connect(qApp, &SingleApplication::raiseWindow, this, [] { CooperationUtil::instance()->mainWindow()->activateWindow(); });
    }

#ifdef linux
    ReportLogManager::instance()->init();
#endif

    CooperationUtil::instance();

    CommonUitls::initLog();
    CommonUitls::loadTranslator();
}

bool CooperaionCorePlugin::start()
{
    CooperationUtil::instance()->mainWindow()->show();
    DiscoverController::instance();
    MainController::instance()->regist();
    TransferHelper::instance()->regist();
    CooperationManager::instance()->regist();
    MainController::instance()->start();

    if (qApp->property("onlyTransfer").toBool() || !QFile(CommonUitls::tipConfPath()).exists())
        emit MainController::instance()->firstStart();

    if (CommonUitls::isFirstStart() && !qApp->property("onlyTransfer").toBool()) {
#ifdef linux
        DFeatureDisplayDialog *dlg = qApp->featureDisplayDialog();
        auto btn = dlg->getButton(0);
        connect(btn, &QAbstractButton::clicked, qApp, &SingleApplication::helpActionTriggered);
        CooperationUtil::instance()->showFeatureDisplayDialog(dlg);
#endif
    }

    return true;
}

void CooperaionCorePlugin::stop()
{
    CooperationUtil::instance()->destroyMainWindow();
    MainController::instance()->unregist();
    MainController::instance()->stop();
}
