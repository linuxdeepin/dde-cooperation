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

    void regist();
    void unregist();
    void updateDeviceState(const DeviceInfoPointer info);

public Q_SLOTS:
    void start();
    void stop();

Q_SIGNALS:
    void onlineStateChanged(bool isOnline);
    void startDiscoveryDevice();
    void deviceOnline(const QList<DeviceInfoPointer> &infoList);
    void deviceOffline(const QString &ip);
    void discoveryFinished(bool hasFound);

private Q_SLOTS:
    void checkNetworkState();
    void updateDeviceList(const QString &ip, const QString &connectedIp, int osType, const QString &info, bool isOnline);
    void onDiscoveryFinished(const QList<DeviceInfoPointer> &infoList);
    void onAppAttributeChanged(const QString &group, const QString &key, const QVariant &value);

private:
    explicit MainController(QObject *parent = nullptr);
    ~MainController();

    void initConnect();
    void registDeviceInfo();
    void discoveryDevice();

private:
    QTimer *networkMonitorTimer { nullptr };

    bool isRunning { false };
    bool isOnline { true };
};

}   // namespace cooperation_core

#endif   // MAINCONTROLLER_H
