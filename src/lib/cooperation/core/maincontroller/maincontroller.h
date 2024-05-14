// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include "global_defines.h"
#include "info/deviceinfo.h"

#if defined(_WIN32) || defined(_WIN64)
#include "gui/win/cooperationdialog.h"
#endif

#include <QObject>
#include <QTimer>
#include <QFuture>
#ifdef __linux__
#include <QDBusInterface>
#endif

namespace cooperation_core {

#if defined(_WIN32) || defined(_WIN64)
#define MID_FRONT 15
#else
#define MID_FRONT 25
#endif
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
    void firstStart();

private Q_SLOTS:
    void checkNetworkState();
    void updateDeviceList(const QString &ip, const QString &connectedIp, int osType, const QString &info, bool isOnline);
    void onDiscoveryFinished(const QList<DeviceInfoPointer> &infoList);
    void onAppAttributeChanged(const QString &group, const QString &key, const QVariant &value);

    void onTransJobStatusChanged(int id, int result, const QString &msg);
    void onFileTransStatusChanged(const QString &status);
    void waitForConfirm(const QString &name);
    void onConfirmTimeout();
    void showToast(bool result, const QString &msg);
    void onNetworkMiss();

    // do something for user actions
    void onAccepted();
    void onRejected();
    void onCanceled();
    void onCompleted();
    void onViewed();

#ifdef __linux__
    void onDConfigValueChanged(const QString &config, const QString &key);
    void onActionTriggered(uint replacesId, const QString &action);
#endif

private:
    explicit MainController(QObject *parent = nullptr);
    ~MainController();

    void initConnect();
    void registDeviceInfo();
    void discoveryDevice();

    void initNotifyConnect();
    void registApp();

    void updateProgress(int value, const QString &remainTime);

#ifdef __linux__
    uint notifyMessage(uint replacesId, const QString &body,
                       const QStringList &actions, QVariantMap hitMap, int expireTimeout);
#endif

private:
    QTimer *networkMonitorTimer { nullptr };

    bool isRunning { false };
    bool isOnline { true };

    QString recvFilesSavePath;
    QString requestFrom;
    QTimer transTimer;
    bool isReplied { false };
    bool isRequestTimeout { false };
#ifdef __linux__
    QDBusInterface *notifyIfc { nullptr };
    uint recvNotifyId { 0 };
    
#elif defined(_WIN32) || defined(_WIN64)
    // Windows support
    CooperationTransDialog *cooperationDlg { nullptr };
#endif
};

}   // namespace cooperation_core

#endif   // MAINCONTROLLER_H
