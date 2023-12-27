// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "maincontroller.h"
#include "utils/cooperationutil.h"
#include "utils/historymanager.h"
#include "configs/settings/configmanager.h"
#include "common/constant.h"
#include "common/commonutils.h"
#include "cooperation/cooperationmanager.h"

#include <QNetworkInterface>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QtConcurrent>
#include <QHostInfo>
#include <QDir>

using ConnectHistory = QMap<QString, QString>;
Q_GLOBAL_STATIC(ConnectHistory, connectHistory)

using namespace cooperation_core;

MainController::MainController(QObject *parent)
    : QObject(parent)
{
    networkMonitorTimer = new QTimer(this);
    networkMonitorTimer->setInterval(1000);

    *connectHistory = HistoryManager::instance()->getConnectHistory();
    connect(HistoryManager::instance(), &HistoryManager::connectHistoryUpdated, this, [] {
        *connectHistory = HistoryManager::instance()->getConnectHistory();
    });

    initConnect();
}

MainController::~MainController()
{
}

void MainController::checkNetworkState()
{
    // 网络状态检测
    bool isConnected = deepin_cross::CommonUitls::getFirstIp().size() > 0;

    if (isConnected != isOnline) {
        isOnline = isConnected;
        networkMonitorTimer->stop();
        Q_EMIT onlineStateChanged(isConnected);
    }
}

void MainController::updateDeviceList(const QString &ip, const QString &connectedIp, int osType, const QString &info, bool isOnline)
{
    if (!this->isOnline)
        return;

    if (isOnline) {
        QJsonParseError error;
        auto doc = QJsonDocument::fromJson(info.toLocal8Bit(), &error);
        if (error.error != QJsonParseError::NoError)
            return;

        auto map = doc.toVariant().toMap();
        if (!map.contains("DeviceName"))
            return;

        map.insert("IPAddress", ip);
        map.insert("OSType", osType);
        auto devInfo = DeviceInfo::fromVariantMap(map);
        if (devInfo->discoveryMode() == DeviceInfo::DiscoveryMode::Everyone) {
            if (connectedIp == CooperationUtil::localIPAddress())
                devInfo->setConnectStatus(DeviceInfo::Connected);

            // 处理设备的共享属性发生变化情况
            CooperationManager::instance()->checkAndProcessShare(devInfo);
            Q_EMIT deviceOnline({ devInfo });
            return;
        }
    }

    // 更新设备状态为离线状态
    if (connectHistory->contains(ip)) {
        DeviceInfoPointer info(new DeviceInfo(ip, connectHistory->value(ip)));
        info->setConnectStatus(DeviceInfo::Offline);
        updateDeviceState(info);
        return;
    }

    Q_EMIT deviceOffline(ip);
}

void MainController::onDiscoveryFinished(const QList<DeviceInfoPointer> &infoList)
{
    if (infoList.isEmpty() && connectHistory->isEmpty()) {
        Q_EMIT discoveryFinished(false);
        isRunning = false;
        return;
    }

    Q_EMIT deviceOnline(infoList);
    Q_EMIT discoveryFinished(true);
    isRunning = false;
}

MainController *MainController::instance()
{
    static MainController ins;
    return &ins;
}

void MainController::start()
{
    if (isRunning)
        return;

    isOnline = deepin_cross::CommonUitls::getFirstIp().size() > 0;
    networkMonitorTimer->start();

    Q_EMIT startDiscoveryDevice();
    isRunning = true;

    // 延迟1s，为了展示发现界面
    QTimer::singleShot(1000, this, &MainController::discoveryDevice);
}

void MainController::stop()
{
}

void MainController::updateDeviceState(const DeviceInfoPointer info)
{
    Q_EMIT deviceOnline({ info });
}

void MainController::discoveryDevice()
{
    if (!isOnline) {
        isRunning = false;
        Q_EMIT onlineStateChanged(isOnline);
        return;
    }

    QList<DeviceInfoPointer> offlineDevList;
    auto iter = connectHistory->begin();
    for (; iter != connectHistory->end(); ++iter) {
        DeviceInfoPointer info(new DeviceInfo(iter.key(), iter.value()));
        info->setConnectStatus(DeviceInfo::Offline);
        offlineDevList << info;
    }

    if (!offlineDevList.isEmpty())
        deviceOnline(offlineDevList);

    CooperationUtil::instance()->asyncDiscoveryDevice();
}
