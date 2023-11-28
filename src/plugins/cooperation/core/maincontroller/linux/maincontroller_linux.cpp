// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../maincontroller.h"
#include "utils/cooperationutil.h"
#include "config/configmanager.h"

using namespace cooperation_core;

void MainController::initConnect()
{
    connect(networkMonitorTimer, &QTimer::timeout, this, &MainController::checkNetworkState);
    connect(CooperationUtil::instance(), &CooperationUtil::discoveryFinished, this, &MainController::onDiscoveryFinished, Qt::QueuedConnection);
}

void MainController::onAppAttributeChanged(const QString &group, const QString &key, const QVariant &value)
{
    Q_UNUSED(group)
    Q_UNUSED(key)
    Q_UNUSED(value)
    return;
}

void MainController::regist()
{
    // TODO: 外设共享？
}

void MainController::unregist()
{
    // TODO:
}
