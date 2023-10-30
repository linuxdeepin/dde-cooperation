// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationcoreplugin.h"
#include "base/baseutils.h"
#include "events/cooperationcoreeventreceiver.h"
#include "utils/cooperationutil.h"
#include "maincontroller/maincontroller.h"

#include <QDebug>

using namespace cooperation_core;

void CooperaionCorePlugin::initialize()
{
    bindEvents();
}

bool CooperaionCorePlugin::start()
{
    CooperationUtil::instance()->mainWindow()->show();
    MainController::instance()->regist();
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
