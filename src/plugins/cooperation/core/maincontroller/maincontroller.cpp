// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "maincontroller.h"
#include "utils/cooperationutil.h"
#include "config/configmanager.h"
#include "common/constant.h"
#include "common/commonutils.h"

#include <QNetworkInterface>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QtConcurrent>
#include <QHostInfo>
#include <QDir>

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
        // 不允许被发现，作为下线处理
        if (devInfo->discoveryMode() == DeviceInfo::DiscoveryMode::NotAllow) {
            Q_EMIT deviceOffline(ip);
            return;
        }
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
