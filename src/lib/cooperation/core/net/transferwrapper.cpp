// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferwrapper.h"
#include "networkutil.h"
#include "helper/transferhelper.h"
#include "discover/discovercontroller.h"

#include <QJsonDocument>
#include <QJsonObject>

using namespace cooperation_core;

TransferWrapper::TransferWrapper(QObject *parent)
    : SlotIPCService(parent)
{
    DLOG << "TransferWrapper constructor";
    connect(DiscoverController::instance(), &DiscoverController::deviceOnline, this, &TransferWrapper::onDeviceOnline);
    connect(DiscoverController::instance(), &DiscoverController::deviceOffline, this, &TransferWrapper::onDeviceOffline);
    connect(DiscoverController::instance(), &DiscoverController::discoveryFinished, this, &TransferWrapper::onFinishedDiscovery);
    DLOG << "TransferWrapper initialized with signal connections for device discovery";
}

TransferWrapper::~TransferWrapper()
{
    DLOG << "TransferWrapper destructor";
}

TransferWrapper *TransferWrapper::instance()
{
    DLOG << "Getting TransferWrapper instance";
    static TransferWrapper ins;
    return &ins;
}

void TransferWrapper::onRefreshDevice()
{
    DLOG << "Refreshing device list";
    DiscoverController::instance()->startDiscover();
}

void TransferWrapper::onSearchDevice(const QString &ip)
{
    DLOG << "Searching device with IP:" << ip.toStdString();
    NetworkUtil::instance()->trySearchDevice(ip);
}

void TransferWrapper::onSendFiles(const QString &ip, const QString &name, const QStringList &files)
{
    TransferHelper::instance()->sendFiles(ip, name, files);
    DLOG << "File transfer request sent to TransferHelper";
}

void TransferWrapper::onDeviceOnline(const QList<DeviceInfoPointer> &infoList)
{
    DLOG << "Device online event, count:" << infoList.size();
    QStringList devList;
    for (auto info : infoList) {
        auto infomap = info->toVariantMap();
        devList << variantMapToQString(infomap);
    }
    if (devList.isEmpty()) {
        DLOG << "No valid device info found";
        return;
    }

    if (devList.size() < 2) {
        DLOG << "Emitting searched signal for single device";
        Q_EMIT searched(devList.first());
    } else {
        DLOG << "Emitting refreshed signal for multiple devices";
        Q_EMIT refreshed(devList);
    }
}

void TransferWrapper::onDeviceOffline(const QString &ip)
{
    DLOG << "Device offline event, IP:" << ip.toStdString();
    Q_EMIT deviceChanged(false, ip);
}

void TransferWrapper::onFinishedDiscovery(bool hasFound)
{
    DLOG << "Discovery finished, hasFound:" << hasFound;
    if (!hasFound) {
        DLOG << "Emitting empty searched signal";
        Q_EMIT searched("");
    }
}


QString TransferWrapper::variantMapToQString(const QVariantMap &variantMap)
{
    DLOG << "Converting variant map to JSON string";
    QJsonObject jsonObject = QJsonObject::fromVariantMap(variantMap);
    QJsonDocument jsonDoc(jsonObject);
    return jsonDoc.toJson(QJsonDocument::Compact);
}
