// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include "global_defines.h"

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
    void deviceOnline(const QList<DeviceInfo> &infoList);
    void deviceOffline(const QString &ip);
    void discoveryFinished(bool hasFound);

private Q_SLOTS:
    void checkNetworkState();
    void updateDeviceList(const QString &ip, const QString &info, bool isOnline);

private:
    explicit MainController(QObject *parent = nullptr);
    ~MainController();

    void initConnect();
    void handleDiscoveryDevice();

private:
    QTimer *networkMonitorTimer { nullptr };

    QFuture<void> future;
    bool isOnline { true };
    bool isStoped { false };
};

}   // namespace cooperation_core

#endif   // MAINCONTROLLER_H
