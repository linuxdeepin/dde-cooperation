// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "maincontroller.h"
#include "utils/cooperationutil.h"
#include "config/configmanager.h"

#include <QNetworkInterface>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QtConcurrent>

using namespace cooperation_core;

MainController::MainController(QObject *parent)
    : QObject(parent)
{
    networkMonitorTimer = new QTimer(this);
    networkMonitorTimer->setInterval(1000);

    initConnect();
}

void MainController::initConnect()
{
    connect(networkMonitorTimer, &QTimer::timeout, this, &MainController::checkNetworkState);
    connect(ConfigManager::instance(), &ConfigManager::appAttributeChanged, this, &MainController::regist);
    connect(&futureWatcher, &QFutureWatcherBase::finished, this, &MainController::discoveryFinished);
}

void MainController::handleDiscoveryDevice()
{
    QTimer::singleShot(1000, this, [this] {
        if (!isOnline) {
            networkMonitorTimer->stop();
            return;
        }

        CooperationUtil::instance()->onlineDeviceInfo();

        DeviceInfo info { "设备测试名称", "10.8.11.193", ConnectState::kConnectable };
        DeviceInfo info11 { "sdf", "10.8.11.193", ConnectState::kConnected };
        DeviceInfo info22 { "xxxx", "192.168.122.30", ConnectState::kConnectable };
        DeviceInfo info33 { "cvbc", "192.168.122.30", ConnectState::kOffline };
        DeviceInfo info1 { "设备测试名称1", "10.2.3.3", ConnectState::kConnected };
        DeviceInfo info2 { "设备测试名称2", "10.2.3.31", ConnectState::kOffline };
        QList<DeviceInfo> infoList;
        for (int i = 0; i < 20; ++i) {
            infoList.append(info);
            infoList.append(info1);
            infoList.append(info2);
            infoList.append(info11);
            infoList.append(info33);
            infoList.append(info22);
        }

        Q_EMIT deviceOnline(infoList);
    });
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
        networkMonitorTimer->stop();
        Q_EMIT onlineStateChanged(isOnlineTemp);
    }
}

void MainController::updateDeviceStatus(const QString &ip, const QString &info, bool isOnline)
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(info.toLocal8Bit(), &error);
    if (error.error != QJsonParseError::NoError)
        return;

    QVariantMap map = doc.toVariant().toMap();
    DeviceInfo devInfo { map.value("DeviceName").toString(),
                         ip,
                         ConnectState::kConnectable };

    if (isOnline)
        Q_EMIT deviceOnline({ devInfo });
    else
        Q_EMIT deviceOffline({ devInfo });
}

MainController *MainController::instance()
{
    static MainController ins;
    return &ins;
}

void MainController::start()
{
    if (futureWatcher.isRunning())
        return;

    isOnline = true;
    networkMonitorTimer->start();

    Q_EMIT startDiscoveryDevice();
    futureWatcher.setFuture(QtConcurrent::run(this, &MainController::handleDiscoveryDevice));
}

void MainController::stop()
{
}

void MainController::regist()
{
    QVariantMap info;
    auto value = ConfigManager::instance()->appAttribute(AppSettings::kGenericGroup, AppSettings::kDiscoveryModeKey);
    info.insert(AppSettings::kDiscoveryModeKey, value.isValid() ? value.toInt() : 0);

    value = ConfigManager::instance()->appAttribute(AppSettings::kGenericGroup, AppSettings::kDeviceNameKey);
    info.insert(AppSettings::kDeviceNameKey, value.isValid() ? value.toString() : QStandardPaths::displayName(QStandardPaths::HomeLocation));

    value = ConfigManager::instance()->appAttribute(AppSettings::kGenericGroup, AppSettings::kPeripheralShareKey);
    info.insert(AppSettings::kPeripheralShareKey, value.isValid() ? value.toBool() : false);

    value = ConfigManager::instance()->appAttribute(AppSettings::kGenericGroup, AppSettings::kLinkDirectionKey);
    info.insert(AppSettings::kLinkDirectionKey, value.isValid() ? value.toInt() : 0);

    value = ConfigManager::instance()->appAttribute(AppSettings::kGenericGroup, AppSettings::kTransferModeKey);
    info.insert(AppSettings::kTransferModeKey, value.isValid() ? value.toInt() : 0);

    value = ConfigManager::instance()->appAttribute(AppSettings::kGenericGroup, AppSettings::kStoragePathKey);
    info.insert(AppSettings::kStoragePathKey, value.isValid() ? value.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));

    value = ConfigManager::instance()->appAttribute(AppSettings::kGenericGroup, AppSettings::kClipboardShareKey);
    info.insert(AppSettings::kClipboardShareKey, value.isValid() ? value.toBool() : false);

    auto doc = QJsonDocument::fromVariant(info);
    CooperationUtil::instance()->registAppInfo(doc.toJson());
}

void MainController::unregist()
{
}
