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

MainController::~MainController()
{
    if (future.isRunning()) {
        stop();
        future.waitForFinished();
    }
}

void MainController::initConnect()
{
    connect(networkMonitorTimer, &QTimer::timeout, this, &MainController::checkNetworkState);
    connect(ConfigManager::instance(), &ConfigManager::appAttributeChanged, this, &MainController::regist);
}

void MainController::handleDiscoveryDevice()
{
    if (!isOnline) {
        networkMonitorTimer->stop();
        return;
    }

    const auto &infoList = CooperationUtil::instance()->onlineDeviceInfo();
    if (infoList.isEmpty()) {
        Q_EMIT discoveryFinished(false);
        return;
    }

    Q_EMIT deviceOnline(infoList);
    Q_EMIT discoveryFinished(!infoList.isEmpty());
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

void MainController::updateDeviceList(const QString &ip, const QString &info, bool isOnline)
{
    if (isOnline) {
        QJsonParseError error;
        auto doc = QJsonDocument::fromJson(info.toLocal8Bit(), &error);
        if (error.error != QJsonParseError::NoError)
            return;

        auto map = doc.toVariant().toMap();
        if (!map.contains("DeviceName"))
            return;

        DeviceInfo devInfo { map.value("DeviceName").toString(),
                             ip,
                             ConnectState::kConnectable };
        Q_EMIT deviceOnline({ devInfo });
    } else {
        Q_EMIT deviceOffline(ip);
    }
}

MainController *MainController::instance()
{
    static MainController ins;
    return &ins;
}

void MainController::start()
{
    if (future.isRunning())
        return;

    isOnline = true;
    networkMonitorTimer->start();

    Q_EMIT startDiscoveryDevice();
    QTimer::singleShot(1000, this, [this] {
        future = QtConcurrent::run(this, &MainController::handleDiscoveryDevice);
    });
}

void MainController::stop()
{
    isStoped = true;
}

void MainController::regist()
{
    QVariantMap info;
    auto value = ConfigManager::instance()->appAttribute(AppSettings::kGenericGroup, AppSettings::kDiscoveryModeKey);
    info.insert(AppSettings::kDiscoveryModeKey, value.isValid() ? value.toInt() : 0);

    value = ConfigManager::instance()->appAttribute(AppSettings::kGenericGroup, AppSettings::kDeviceNameKey);
    info.insert(AppSettings::kDeviceNameKey,
                value.isValid()
                        ? value.toString()
                        : QStandardPaths::writableLocation(QStandardPaths::HomeLocation).section(QDir::separator(), -1));

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
    CooperationUtil::instance()->unregistAppInfo();
}
