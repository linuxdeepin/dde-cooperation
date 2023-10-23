// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "maincontroller.h"
#include "gui/workspacewidget.h"

#include <QNetworkInterface>

using namespace cooperation_workspace;

MainController::MainController(QObject *parent)
    : QObject(parent),
      w(new WorkspaceWidget)
{
    networkMonitorTimer = new QTimer(this);
    networkMonitorTimer->setInterval(1000);

    initConnect();
}

void MainController::initConnect()
{
    connect(networkMonitorTimer, &QTimer::timeout, this, &MainController::checkNetworkState);
}

void MainController::checkNetworkState()
{
    // 网络状态检测
    bool isOnlineTemp = isOnline;
    QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
    for (int i = 0; i < ifaces.count(); i++) {
        QNetworkInterface iface = ifaces.at(i);
        if (iface.flags().testFlag(QNetworkInterface::IsUp)
            && iface.flags().testFlag(QNetworkInterface::IsRunning)
            && !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            if (!iface.addressEntries().isEmpty()) {
                isOnlineTemp = true;
                break;
            }
        } else if (!iface.addressEntries().isEmpty()) {
            isOnlineTemp = false;
        }
    }

    if (isOnlineTemp != isOnline) {
        isOnline = isOnlineTemp;
        onlineStateChanged();
    }
}

MainController *MainController::instance()
{
    static MainController ins;
    return &ins;
}

QWidget *MainController::workspaceWidget()
{
    return w;
}

void MainController::start()
{
    if (isRuning)
        return;

    networkMonitorTimer->start();
    isRuning = true;
    w->clear();
    w->switchWidget(WorkspaceWidget::kLookignForDeviceWidget);

    QTimer::singleShot(1000, this, [this] {
        if (!isOnline) {
            w->switchWidget(WorkspaceWidget::kNoNetworkWidget);
            isRuning = false;
            networkMonitorTimer->stop();
            return;
        }

        w->switchWidget(WorkspaceWidget::kDeviceListWidget);

        DeviceInfo info { "设备测试名称", "10.8.11.193", ConnectState::kConnectable };
        DeviceInfo info11 { "sdf", "10.8.11.193", ConnectState::kConnected };
        DeviceInfo info22 { "xxxx", "192.168.122.30", ConnectState::kConnectable };
        DeviceInfo info33 { "cvbc", "192.168.122.30", ConnectState::kOffline };
        DeviceInfo info1 { "设备测试名称1", "10.2.3.3", ConnectState::kConnected };
        DeviceInfo info2 { "设备测试名称2", "10.2.3.31", ConnectState::kOffline };
        for (int i = 0; i < 20; ++i) {
            w->addDeviceInfo(info);
            w->addDeviceInfo(info1);
            w->addDeviceInfo(info2);
            w->addDeviceInfo(info11);
            w->addDeviceInfo(info33);
            w->addDeviceInfo(info22);
        }

        isRuning = false;
    });
}

void MainController::stop()
{
}

void MainController::registerDeviceOperation(const QVariantMap &map)
{
    w->addDeviceOperation(map);
}

void MainController::onlineStateChanged()
{
    if (!isOnline) {
        w->clear();
        networkMonitorTimer->stop();
        w->switchWidget(WorkspaceWidget::kNoNetworkWidget);
        return;
    }
}
