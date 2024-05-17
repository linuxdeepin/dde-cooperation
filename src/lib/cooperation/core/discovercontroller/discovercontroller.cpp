// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "discovercontroller.h"
#include "utils/cooperationutil.h"
#include "common/log.h"

#include <QProcess>
#include <QTimer>
#include <common/constant.h>
#include <configs/dconfig/dconfigmanager.h>
#include <configs/settings/configmanager.h>

using namespace cooperation_core;

DiscoverController::DiscoverController(QObject *parent)
    : QObject(parent)
{
    if (!zeroConf.browserExists())
        zeroConf.startBrowser(UNI_CHANNEL);

    if (!isZeroConfDaemonActive())
        openZeroConfDaemonDailog();
    initConnect();
    publish();
}

DiscoverController::~DiscoverController()
{
}

void DiscoverController::initConnect()
{
#ifdef linux
    connect(DConfigManager::instance(), &DConfigManager::valueChanged, this, &DiscoverController::onDConfigValueChanged);
#endif
    connect(ConfigManager::instance(), &ConfigManager::appAttributeChanged, this, &DiscoverController::onAppAttributeChanged);
    connect(&zeroConf, &QZeroConf::serviceAdded, this, &DiscoverController::addService);
    connect(&zeroConf, &QZeroConf::serviceRemoved, this, &DiscoverController::removeService);
    connect(&zeroConf, &QZeroConf::serviceUpdated, this, &DiscoverController::updateService);
}

QList<DeviceInfoPointer> DiscoverController::getOnlineDeviceList() const
{
    return onlineDeviceList;
}

void DiscoverController::openZeroConfDaemonDailog()
{
#ifdef __linux__
    CooperationDialog dlg;
    dlg.setIcon(QIcon::fromTheme("dialog-warning"));
    dlg.addButton(tr("Confirm"), true, CooperationDialog::ButtonRecommend);
    dlg.addButton(tr("Close"), false, CooperationDialog::ButtonWarning);

    dlg.setTitle(tr("Please click to confirm to enable the LAN discovery service!"));
    dlg.setMessage(tr("Unable to discover and be discovered by other devices when LAN discovery service is not turned on"));

    int code = dlg.exec();
    if (code == 0)
        QProcess::startDetached("systemctl start avahi-daemon.service");
#endif
}

bool DiscoverController::isZeroConfDaemonActive()
{
#ifdef __linux__
    QProcess process;
    process.start("systemctl", QStringList() << "is-active"
                                             << "avahi-daemon.service");
    process.waitForFinished();

    QString output = process.readAllStandardOutput();
    QString error = process.readAllStandardError();

    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
        if (output.contains("active")) {
            LOG << "Avahi service is running";
            return true;
        } else {
            WLOG << "Avahi service is not running";
            return false;
        }
    } else {
        ELOG << "Error: " << error.toStdString();
        return false;
    }
#endif
    return true;
}

DeviceInfoPointer DiscoverController::findDeviceByIP(const QString &ip)
{
    for (auto info : onlineDeviceList)
        if (info->ipAddress() == ip)
            return info;
    return nullptr;
}

void DiscoverController::onDConfigValueChanged(const QString &config, const QString &key)
{
    Q_UNUSED(key);
    if (config != kDefaultCfgPath)
        return;

    updatePublish();
}

void DiscoverController::onAppAttributeChanged(const QString &group, const QString &key, const QVariant &value)
{
    if (group != AppSettings::GenericGroup)
        return;

    if (key == AppSettings::StoragePathKey)
        CooperationUtil::instance()->setAppConfig("storagedir", value.toString());

    updatePublish();
}

void DiscoverController::addService(QZeroConfService zcs)
{
    QVariantMap infomap;
    for (const auto &key : zcs->txt().keys())
        infomap.insert(key, zcs->txt().value(key));

    auto devInfo = DeviceInfo::fromVariantMap(infomap);
    if (devInfo->ipAddress().isEmpty())
        return;

    onlineDeviceList.append(devInfo);
    Q_EMIT deviceOnline({ devInfo });
}

void DiscoverController::updateService(QZeroConfService zcs)
{
    QVariantMap infomap;
    for (const auto &key : zcs->txt().keys())
        infomap.insert(key, zcs->txt().value(key));

    auto devInfo = DeviceInfo::fromVariantMap(infomap);
    if (devInfo->ipAddress().isEmpty())
        return;

    auto oldinfo = findDeviceByIP(devInfo->ipAddress());
    if (oldinfo)
        onlineDeviceList.removeOne(oldinfo);
    onlineDeviceList.append(devInfo);
    Q_EMIT deviceOnline({ devInfo });
}

void DiscoverController::removeService(QZeroConfService zcs)
{
    QVariantMap infomap;
    for (const auto &key : zcs->txt().keys())
        infomap.insert(key, zcs->txt().value(key));

    auto devInfo = DeviceInfo::fromVariantMap(infomap);
    if (devInfo->ipAddress().isEmpty())
        return;

    auto oldinfo = findDeviceByIP(devInfo->ipAddress());
    if (oldinfo)
        onlineDeviceList.removeOne(oldinfo);
    Q_EMIT deviceOffline(devInfo->ipAddress());
}

DiscoverController *DiscoverController::instance()
{
    static DiscoverController ins;
    return &ins;
}

void DiscoverController::publish()
{
    zeroConf.clearServiceTxtRecords();

    QVariantMap deviceInfo = CooperationUtil::deviceInfo();
    //设置为局域网不发现
    if (deviceInfo.value(AppSettings::DiscoveryModeKey) == 1)
        return;

    deviceInfo.insert(AppSettings::IPAddress, CooperationUtil::localIPAddress());
    qWarning() << "publish:-------" << deviceInfo;
    for (const auto &key : deviceInfo.keys())
        zeroConf.addServiceTxtRecord(key, deviceInfo.value(key).toString());

    zeroConf.startServicePublish(CooperationUtil::localIPAddress().toUtf8(), UNI_CHANNEL, "local", UNI_RPC_PORT_UDP);
}

void DiscoverController::unpublish()
{
    zeroConf.stopServicePublish();
}

void DiscoverController::updatePublish()
{
    unpublish();
    publish();
}

void DiscoverController::refresh()
{
    onlineDeviceList.clear();
    auto allServices = zeroConf.getServices();

    for (const auto &key : allServices.keys()) {
        QVariantMap infomap;
        QZeroConfService zcs = allServices.value(key);
        for (const auto &key : zcs->txt().keys())
            infomap.insert(key, zcs->txt().value(key));

        auto devInfo = DeviceInfo::fromVariantMap(infomap);
        onlineDeviceList.append(devInfo);
    }
    Q_EMIT deviceOnline({ onlineDeviceList });
}

void DiscoverController::startDiscover()
{
    Q_EMIT startDiscoveryDevice();

    // 延迟1s，为了展示发现界面
    QTimer::singleShot(1000, this, &DiscoverController::refresh);
}
