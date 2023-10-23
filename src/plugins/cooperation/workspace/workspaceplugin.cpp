// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "workspaceplugin.h"
#include "events/workspaceeventreceiver.h"
#include "maincontroller/maincontroller.h"

using CreateWorkspaceWidgetCallback = std::function<QWidget *()>;
Q_DECLARE_METATYPE(CreateWorkspaceWidgetCallback);

using namespace cooperation_workspace;

void WorkspacePlugin::initialize()
{
    bindEvents();
}

bool WorkspacePlugin::start()
{
    CreateWorkspaceWidgetCallback creatWidget { []() { return MainController::instance()->workspaceWidget(); } };
    dpfSlotChannel->push("cooperation_core", "slot_Register_Workspace", QVariant::fromValue(creatWidget));

    MainController::instance()->start();
    return true;
}

void WorkspacePlugin::stop()
{
    MainController::instance()->stop();
}

void WorkspacePlugin::bindEvents()
{
    dpfSignalDispatcher->subscribe("cooperation_core", "signal_Request_Refresh",
                                   WorkspaceEventReceiver::instance(), &WorkspaceEventReceiver::handleRequestRefresh);

    dpfSlotChannel->connect("cooperation_workspace", "slot_Register_Operation",
                            WorkspaceEventReceiver::instance(), &WorkspaceEventReceiver::handleRegisterOperation);
}
