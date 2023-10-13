// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationcoreeventreceiver.h"
#include "gui/mainwindow.h"

using CreateWorkspaceWidgetCallback = std::function<QWidget *()>;
Q_DECLARE_METATYPE(CreateWorkspaceWidgetCallback);

using namespace cooperation_core;

CooperationCoreEventReceiver::CooperationCoreEventReceiver(QObject *parent)
    : QObject(parent)
{
}

CooperationCoreEventReceiver *CooperationCoreEventReceiver::instance()
{
    static CooperationCoreEventReceiver ins;
    return &ins;
}

void CooperationCoreEventReceiver::handleRegisterWorkspace(QVariant param)
{
    auto func = param.value<CreateWorkspaceWidgetCallback>();
    if (!func)
        return;

    auto widget = func();
    if (widget && !widget->parent())
        widget->setParent(MainWindow::instance());

    MainWindow::instance()->setCentralWidget(widget);
}

void CooperationCoreEventReceiver::handleRegisterSettings()
{
}
