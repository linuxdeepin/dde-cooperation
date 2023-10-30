// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include "global_defines.h"

#include <QObject>
#include <QTimer>
#include <QFutureWatcher>

namespace cooperation_core {

class MainController : public QObject
{
    Q_OBJECT
public:
    static MainController *instance();

    void start();
    void stop();
    void regist();
    void unregist();

Q_SIGNALS:
    void onlineStateChanged(bool isOnline);
    void startDiscoveryDevice();
    void deviceOnline(const QList<DeviceInfo> &infoList);
    void deviceOffline(const QList<DeviceInfo> &infoList);
    void discoveryFinished();

private Q_SLOTS:
    void checkNetworkState();
    void updateDeviceStatus(const QString &ip, const QString &info, bool isOnline);

private:
    explicit MainController(QObject *parent = nullptr);
    void initConnect();
    void handleDiscoveryDevice();

private:
    QTimer *networkMonitorTimer { nullptr };

    QFutureWatcher<void> futureWatcher;
    bool isOnline = true;
};

}   // namespace cooperation_core

#endif   // MAINCONTROLLER_H
