// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONMANAGER_H
#define COOPERATIONMANAGER_H

#include "info/deviceinfo.h"

namespace cooperation_core {

class CooperationManagerPrivate;
class CooperationManager : public QObject
{
    Q_OBJECT

public:
    static CooperationManager *instance();

    void regist();

    void checkAndProcessShare(const DeviceInfoPointer info);

    static void buttonClicked(const QString &id, const DeviceInfoPointer info);
    static bool buttonVisible(const QString &id, const DeviceInfoPointer info);

public Q_SLOTS:
    void connectToDevice(const DeviceInfoPointer info);
    void disconnectToDevice(const DeviceInfoPointer info);
    void notifyConnectRequest(const QString &info);
    void handleConnectResult(int result);
    void handleDisConnectResult(const QString &devName);
    void onVerifyTimeout();
    void handleCancelCooperApply();
    void handleNetworkDismiss();

private:
    explicit CooperationManager(QObject *parent = nullptr);
    ~CooperationManager();

    QSharedPointer<CooperationManagerPrivate> d { nullptr };
};

}   // namespace cooperation_core

#endif   // COOPERATIONMANAGER_H
