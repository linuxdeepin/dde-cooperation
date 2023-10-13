// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "workspaceeventreceiver.h"
#include "maincontroller/maincontroller.h"

using namespace cooperation_workspace;

WorkspaceEventReceiver::WorkspaceEventReceiver(QObject *parent)
    : QObject(parent)
{
}

WorkspaceEventReceiver *WorkspaceEventReceiver::instance()
{
    static WorkspaceEventReceiver ins;
    return &ins;
}

void WorkspaceEventReceiver::handleRequestRefresh()
{
    MainController::instance()->start();
}

void WorkspaceEventReceiver::handleRegisterOperation(const QVariantMap &map)
{
    MainController::instance()->registerDeviceOperation(map);
}
