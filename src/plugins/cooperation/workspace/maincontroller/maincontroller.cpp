// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "maincontroller.h"
#include "gui/workspacewidget.h"

using namespace cooperation_workspace;

MainController::MainController(QObject *parent)
    : QObject(parent),
      w(new WorkspaceWidget),
      networkMgr(new QNetworkConfigurationManager(this))
{
    initConnect();
}

void MainController::initConnect()
{
    connect(networkMgr, &QNetworkConfigurationManager::onlineStateChanged, this, &MainController::onlineStateChanged);
}

MainController *MainController::instance()
{
    static MainController ins;
    return &ins;
}

QWidget *MainController::widget()
{
    return w;
}

void MainController::start()
{
    if (isRuning)
        return;

    if (!networkMgr->isOnline()) {
        w->switchWidget(WorkspaceWidget::kNoNetworkWidget);
        return;
    }

    isRuning = true;
    w->switchWidget(WorkspaceWidget::kLookignForDeviceWidget);

    DeviceInfo info { "设备测试名称", "192.168.122.30", ConnectState::kConnectable };
    DeviceInfo info1 { "设备测试名称1", "10.2.3.3", ConnectState::kConnected };
    DeviceInfo info2 { "设备测试名称2", "10.2.3.31", ConnectState::kOffline };
    w->addDeviceInfo(info);
    w->addDeviceInfo(info1);
    w->addDeviceInfo(info2);
    w->switchWidget(WorkspaceWidget::kDeviceListWidget);
}

void MainController::stop()
{
}

void MainController::registerDeviceOperation(const QVariantMap &map)
{
    w->addDeviceOperation(map);
}

void MainController::onlineStateChanged(bool isOnline)
{
    if (!isOnline) {
        w->clear();
        w->switchWidget(WorkspaceWidget::kNoNetworkWidget);
        return;
    }
}
