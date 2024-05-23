// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SHAREHELPER_H
#define SHAREHELPER_H

#include "discover/deviceinfo.h"

namespace cooperation_core {

class ShareHelperPrivate;
class ShareHelper : public QObject
{
    Q_OBJECT

public:
    static ShareHelper *instance();

    void registConnectBtn();

    void checkAndProcessShare(const DeviceInfoPointer info);

    static void buttonClicked(const QString &id, const DeviceInfoPointer info);
    static bool buttonVisible(const QString &id, const DeviceInfoPointer info);

public Q_SLOTS:
    void connectToDevice(const DeviceInfoPointer info);
    void disconnectToDevice(const DeviceInfoPointer info);
    void notifyConnectRequest(const QString &info);
    void onVerifyTimeout();

    void handleConnectResult(int result);
    void handleDisConnectResult(const QString &devName);
    void handleCancelCooperApply();
    void handleNetworkDismiss(const QString &msg);
    void handleSearchDeviceResult(bool res);

private:
    explicit ShareHelper(QObject *parent = nullptr);
    ~ShareHelper();

    QSharedPointer<ShareHelperPrivate> d { nullptr };
};

}   // namespace cooperation_core

#endif   // SHAREHELPER_H
