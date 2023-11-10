// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "maincontroller.h"
#include "utils/cooperationutil.h"
#include "config/configmanager.h"
#include "common/constant.h"

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
}

void MainController::initConnect()
{
    connect(networkMonitorTimer, &QTimer::timeout, this, &MainController::checkNetworkState);
    connect(ConfigManager::instance(), &ConfigManager::appAttributeChanged, this, &MainController::onAppAttributeChanged);
    connect(CooperationUtil::instance(), &CooperationUtil::discoveryFinished, this, &MainController::onDiscoveryFinished, Qt::QueuedConnection);
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

        map.insert("IPAddress", ip);

        auto devInfo = DeviceInfo::fromVariantMap(map);
        Q_EMIT deviceOnline({ devInfo });
    } else {
        Q_EMIT deviceOffline(ip);
    }
}

void MainController::onDiscoveryFinished(const QList<DeviceInfoPointer> &infoList)
{
    if (infoList.isEmpty()) {
        Q_EMIT discoveryFinished(false);
        isRunning = false;
        return;
    }

    Q_EMIT deviceOnline(infoList);
    Q_EMIT discoveryFinished(!infoList.isEmpty());
    isRunning = false;
}

void MainController::onAppAttributeChanged(const QString &group, const QString &key, const QVariant &value)
{
    if (group != AppSettings::GenericGroup)
        return;

    if (key == AppSettings::StoragePathKey)
        CooperationUtil::instance()->setAppConfig(KEY_APP_STORAGE_DIR, value.toString());

    regist();
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

    isOnline = true;
    networkMonitorTimer->start();

    Q_EMIT startDiscoveryDevice();
    isRunning = true;

    // 延迟1s，为了展示发现界面
    QTimer::singleShot(1000, this, [] { CooperationUtil::instance()->asyncDiscoveryDevice(); });
}

void MainController::stop()
{
}

void MainController::regist()
{
    QVariantMap info;
    auto value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::DiscoveryModeKey);
    info.insert(AppSettings::DiscoveryModeKey, value.isValid() ? value.toInt() : 0);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::DeviceNameKey);
    info.insert(AppSettings::DeviceNameKey,
                value.isValid()
                        ? value.toString()
                        : QStandardPaths::writableLocation(QStandardPaths::HomeLocation).section(QDir::separator(), -1));

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::PeripheralShareKey);
    info.insert(AppSettings::PeripheralShareKey, value.isValid() ? value.toBool() : false);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::LinkDirectionKey);
    info.insert(AppSettings::LinkDirectionKey, value.isValid() ? value.toInt() : 0);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::TransferModeKey);
    info.insert(AppSettings::TransferModeKey, value.isValid() ? value.toInt() : 0);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::StoragePathKey);
    auto storagePath = value.isValid() ? value.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    info.insert(AppSettings::StoragePathKey, storagePath);
    static std::once_flag flag;
    std::call_once(flag, [&storagePath] { CooperationUtil::instance()->setAppConfig(KEY_APP_STORAGE_DIR, storagePath); });

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::ClipboardShareKey);
    info.insert(AppSettings::ClipboardShareKey, value.isValid() ? value.toBool() : false);

    auto doc = QJsonDocument::fromVariant(info);
    CooperationUtil::instance()->registAppInfo(doc.toJson());
}

void MainController::unregist()
{
    CooperationUtil::instance()->unregistAppInfo();
}
