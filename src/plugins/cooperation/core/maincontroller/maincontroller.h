// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include "global_defines.h"
#include "info/deviceinfo.h"

#include <QObject>
#include <QTimer>
#include <QFuture>

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
    void deviceOnline(const QList<DeviceInfoPointer> &infoList);
    void deviceOffline(const QString &ip);
    void discoveryFinished(bool hasFound);

private Q_SLOTS:
    void checkNetworkState();
    void updateDeviceList(const QString &ip, const QString &info, bool isOnline);
    void onDiscoveryFinished(const QList<DeviceInfoPointer> &infoList);

private:
    explicit MainController(QObject *parent = nullptr);
    ~MainController();

    void initConnect();

private:
    QTimer *networkMonitorTimer { nullptr };

    bool isRunning { false };
    bool isOnline { true };
};

}   // namespace cooperation_core

#endif   // MAINCONTROLLER_H
