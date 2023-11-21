// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationcoreplugin.h"
#include "base/baseutils.h"
#include "events/cooperationcoreeventreceiver.h"
#include "utils/cooperationutil.h"
#include "maincontroller/maincontroller.h"
#include "transfer/transferhelper.h"
#include "config/configmanager.h"
#include "singleton/singleapplication.h"

#include <co/flag.h>

using namespace cooperation_core;
using namespace deepin_cross;

void CooperaionCorePlugin::initialize()
{
    if (qApp->property("onlyTransfer").toBool()) {
        auto appName = qApp->applicationName();
        qApp->setApplicationName(kMainAppName);
        ConfigManager::instance();
        qApp->setApplicationName(appName);
    } else {
        connect(qApp, &SingleApplication::raiseWindow, this, [] { CooperationUtil::instance()->mainWindow()->activateWindow(); });
    }

    CooperationUtil::instance();
    bindEvents();
    initLog();
}

bool CooperaionCorePlugin::start()
{
    CooperationUtil::instance()->mainWindow()->show();
    MainController::instance()->regist();
    TransferHelper::instance()->regist();
    MainController::instance()->start();
    return true;
}

void CooperaionCorePlugin::stop()
{
    CooperationUtil::instance()->destroyMainWindow();
    MainController::instance()->unregist();
    MainController::instance()->stop();
}

void CooperaionCorePlugin::bindEvents()
{
    dpfSlotChannel->connect("cooperation_core", "slot_Register_Operation",
                            CooperationCoreEventReceiver::instance(), &CooperationCoreEventReceiver::handleRegisterOperation);
}

void CooperaionCorePlugin::initLog()
{
    flag::set_value("rpc_log", "false");   //rpc日志关闭

#if defined(QT_DEBUG) || defined(WIN32)
    flag::set_value("cout", "true");   //终端日志输出
#else
    fastring logdir = deepin_cross::BaseUtils::logDir().toStdString();
    qInfo() << "set logdir: " << logdir.c_str();
    flag::set_value("log_dir", logdir);   //日志保存目录
#endif
}
